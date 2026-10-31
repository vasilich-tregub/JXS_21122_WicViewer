/* TERMS OF USE
 * This source code is subject to the terms of the MIT License.
 * Copyright(c) 2026 Vladimir Vasilich Tregub
*/
#include "framework.h"
#include <wincodec.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include "JXS_21122_WicViewer.h"

#include <shlwapi.h>
#pragma comment(lib, "shlwapi")
#include <shobjidl.h>

template <typename T>
inline void SafeRelease(T*& p)
{
    if (NULL != p)
    {
        p->Release();
        p = NULL;
    }
}

/******************************************************************
*  Application entrypoint                                         *
******************************************************************/

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_JXS21122WICVIEWER));

    if (SUCCEEDED(hr))
    {
        {
            DemoApp app;
            hr = app.Initialize(hInstance);

            if (SUCCEEDED(hr))
            {
                BOOL fRet;
                MSG msg;

                // Main message loop:
                while ((fRet = GetMessage(&msg, NULL, 0, 0)) != 0)
                {
                    if (fRet == -1)
                    {
                        break;
                    }
                    else
                        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
                        {
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                }
            }
        }
        CoUninitialize();
    }

    return 0;
}

/******************************************************************
*  Initialize member data                                         *
******************************************************************/

DemoApp::DemoApp()
    :
    m_pD2DBitmap(NULL),
    m_pBitmapClipper(NULL),
    m_pIWICFactory(NULL),
    m_pD2DFactory(NULL),
    m_pRT(NULL),
    m_hInst(0)
{
}

/******************************************************************
*  Tear down resources                                            *
******************************************************************/

DemoApp::~DemoApp()
{
    SafeRelease(m_pD2DBitmap);
    SafeRelease(m_pBitmapClipper);
    SafeRelease(m_pIWICFactory);
    SafeRelease(m_pD2DFactory);
    SafeRelease(m_pRT);
}

/******************************************************************
*  Create application window and resources                        *
******************************************************************/

HRESULT DemoApp::Initialize(HINSTANCE hInstance)
{

    HRESULT hr = S_OK;

    // Create WIC factory
    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_pIWICFactory)
    );

    if (SUCCEEDED(hr))
    {
        // Create D2D factory
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
    }

    if (SUCCEEDED(hr))
    {
        WNDCLASSEX wcex;

        // Register window class
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = DemoApp::s_WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = sizeof(LONG_PTR);
        wcex.hInstance = hInstance;
        wcex.hIcon = NULL;
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = NULL;
        wcex.lpszMenuName = MAKEINTRESOURCE(IDC_JXS21122WICVIEWER);
        wcex.lpszClassName = L"JXS_21122_WicViewer";
        wcex.hIconSm = NULL;

        m_hInst = hInstance;

        hr = RegisterClassEx(&wcex) ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // Create window
        HWND hWnd = CreateWindow(
            L"JXS_21122_WicViewer",
            L"JXS 21122 REF SFTWR WIC Viewer D2D Extended Formats",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            640,
            480,
            NULL,
            NULL,
            hInstance,
            this
        );

        hr = hWnd ? S_OK : E_FAIL;
    }

    return hr;
}

/******************************************************************
*  Load an image file and create an D2DBitmap                     *
******************************************************************/

HRESULT DemoApp::CreateD2DBitmapFromFile(HWND hWnd)
{
    IWICFormatConverter* convertedSourceBitmap = nullptr;

    HRESULT hr = S_OK;

    // Step 1: Create the open dialog box and locate the image file
    std::vector<std::wstring> paths;
    if (LocateImageFile(paths))
    {
        if (paths.size() == 0)
        {
            MessageBox(hWnd, L"No image files selected", L"Selecting images", MB_OK);
            return S_OK;
        }
        // Step 2: Decode the source image

        // Create a decoder
        IWICBitmapDecoder* pDecoder = NULL;

        hr = m_pIWICFactory->CreateDecoderFromFilename(
            paths[0].data(),                      // Image to be decoded
            NULL,                            // Do not prefer a particular vendor
            GENERIC_READ,                    // Desired read access to the file
            WICDecodeMetadataCacheOnDemand,  // Cache metadata when needed
            &pDecoder                        // Pointer to the decoder
        );

        // Retrieve the first frame of the image from the decoder
        IWICBitmapFrameDecode* pFrame = NULL;

        if (SUCCEEDED(hr))
        {
            hr = pDecoder->GetFrame(0, &pFrame);
        }

        unsigned int frame_width = 0, frame_height = 0;
        if (SUCCEEDED(hr))
        {
            hr = pFrame->GetSize(&frame_width, &frame_height);
        }

        m_width = (frame_width > 8192) ? 8192 : frame_width;
        m_height = (frame_height > 8192) ? 8192 : frame_height;

        //Step 3: Format convert the frame to 32bppPBGRA
        if (SUCCEEDED(hr))
        {
            SafeRelease(convertedSourceBitmap);
            hr = m_pIWICFactory->CreateFormatConverter(&convertedSourceBitmap);
        }

        if (SUCCEEDED(hr))
        {
            hr = convertedSourceBitmap->Initialize(
                pFrame,                          // Input bitmap to convert
                GUID_WICPixelFormat32bppPBGRA,   // Destination pixel format
                WICBitmapDitherTypeNone,         // Specified dither pattern
                NULL,                            // Specify a particular palette 
                0.f,                             // Alpha threshold
                WICBitmapPaletteTypeCustom       // Palette translation type
            );
        }

        // if no clipping required, this adds an unnecessary overhead, will see to it later
        if (SUCCEEDED(hr))
        {
            SafeRelease(m_pBitmapClipper);
            hr = m_pIWICFactory->CreateBitmapClipper(&m_pBitmapClipper);
        }

        WICRect cliprect = { 0, 0, (int)m_width, (int)m_height };
        if (SUCCEEDED(hr))
        {
            m_pBitmapClipper->Initialize(convertedSourceBitmap, &cliprect);
        }

        //Step 4: Create render target and D2D bitmap from IWICBitmapSource
        if (SUCCEEDED(hr))
        {
            hr = CreateDeviceResources(hWnd);
        }

        if (SUCCEEDED(hr))
        {
            // Need to release the previous D2DBitmap if there is one
            SafeRelease(m_pD2DBitmap);
            hr = m_pRT->CreateBitmapFromWicBitmap(m_pBitmapClipper, NULL, &m_pD2DBitmap);
            std::wstring title = L"Image File ";
            title += paths[0].data();
            SetWindowTextW(hWnd, title.c_str());
        }

        SafeRelease(pDecoder);
        SafeRelease(pFrame);
    }

    return hr;
}

/******************************************************************
* Creates an open file dialog box and locate the image to decode. *
******************************************************************/

BOOL DemoApp::LocateImageFile(std::vector<std::wstring>& paths)
{
    IFileOpenDialog* p_file_open = nullptr;
    bool are_all_operation_success = false;
    while (!are_all_operation_success)
    {
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&p_file_open));
        if (FAILED(hr))
            break;

        FILEOPENDIALOGOPTIONS options = 0;
        hr = p_file_open->GetOptions(&options);
        if (FAILED(hr))
            break;

        //options |= FOS_PICKFOLDERS;
        options |= FOS_ALLOWMULTISELECT;

        hr = p_file_open->SetOptions(options);
        if (FAILED(hr))
            break;

        COMDLG_FILTERSPEC filters[2];
        filters[0].pszName = L"All Image files.";
        filters[0].pszSpec = L"*.bmp;*.dib;*.wdp;*.mdp;*.hdp;*.gif;*.png;*.jpg;*.jpeg;*.tif;*.ico";
        filters[1].pszName = L"All Files.";
        filters[1].pszSpec = L"*.*";

        p_file_open->SetFileTypes(2, filters);
        hr = p_file_open->Show(NULL);
        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) // No items were selected.
        {
            are_all_operation_success = true;
            break;
        }
        else if (FAILED(hr))
            break;

        IShellItemArray* p_items;
        hr = p_file_open->GetResults(&p_items);
        if (FAILED(hr))
            break;
        DWORD total_items = 0;
        hr = p_items->GetCount(&total_items);
        if (FAILED(hr))
            break;

        for (int i = 0; i < static_cast<int>(total_items); ++i)
        {
            IShellItem* p_item;
            p_items->GetItemAt(i, &p_item);
            if (SUCCEEDED(hr))
            {
                PWSTR path;
                hr = p_item->GetDisplayName(SIGDN_FILESYSPATH, &path);
                if (SUCCEEDED(hr))
                {
                    paths.push_back(path);
                    CoTaskMemFree(path);
                }
                p_item->Release();
            }
        }

        p_items->Release();
        are_all_operation_success = true;
    }

    if (p_file_open)
        p_file_open->Release();
    return are_all_operation_success;
}

/******************************************************************
*  This method creates resources which are bound to a particular  *
*  D2D device. It's all centralized here, in case the resources   *
*  need to be recreated in the event of D2D device loss           *
* (e.g. display change, remoting, removal of video card, etc).    *
******************************************************************/

HRESULT DemoApp::CreateDeviceResources(HWND hWnd)
{
    HRESULT hr = S_OK;


    if (!m_pRT)
    {
        RECT rc;
        hr = GetClientRect(hWnd, &rc) ? S_OK : E_FAIL;

        if (SUCCEEDED(hr))
        {
            // Create a D2D render target properties
            D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties();

            // Set the DPI to be the default system DPI to allow direct mapping
            // between image pixels and desktop pixels in different system DPI settings
            renderTargetProperties.dpiX = DEFAULT_DPI;
            renderTargetProperties.dpiY = DEFAULT_DPI;

            // Create a D2D render target
            D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
            if (m_width > 0 && m_height > 0)
            {
                size = D2D1::SizeU(m_width, m_height);
            }

            hr = m_pD2DFactory->CreateHwndRenderTarget(
                renderTargetProperties,
                D2D1::HwndRenderTargetProperties(hWnd, size),
                &m_pRT
            );
        }
    }

    return hr;
}

/******************************************************************
*  Regsitered Window message handler                              *
******************************************************************/

LRESULT CALLBACK DemoApp::s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DemoApp* pThis;
    LRESULT lRet = 0;

    if (uMsg == WM_NCCREATE)
    {
        LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT> (lParam);
        pThis = reinterpret_cast<DemoApp*> (pcs->lpCreateParams);

        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    else
    {
        pThis = reinterpret_cast<DemoApp*> (GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if (pThis)
        {
            lRet = pThis->WndProc(hWnd, uMsg, wParam, lParam);
        }
        else
        {
            lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
    return lRet;
}

/******************************************************************
*  Internal Window message handler                                *
******************************************************************/

LRESULT DemoApp::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        // Parse the menu selections:
        switch (LOWORD(wParam))
        {
        case ID_FILE_OPEN:
        {
            if (SUCCEEDED(CreateD2DBitmapFromFile(hWnd)))
            {
                InvalidateRect(hWnd, NULL, TRUE);
            }
            else
            {
                MessageBox(hWnd, L"Failed to load image, select a new one.", L"Application Error", MB_ICONEXCLAMATION | MB_OK);
            }
            break;
        }
        case ID_FILE_JPEGXSFILES:
        {
            if (SUCCEEDED(CreateD2DBitmapFromJPEGXSFile(hWnd)))
            {
                InvalidateRect(hWnd, NULL, TRUE);
            }
            else
            {
                MessageBox(hWnd, L"Failed to load image, select a new one.", L"Application Error", MB_ICONEXCLAMATION | MB_OK);
            }
            break;
        }
        case ID_VIEW_ACTUALSIZE:
        {
            m_zoom = 1.0;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        case ID_VIEW_ZOOMIN:
        {
            m_zoom *= 2.0;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        case ID_VIEW_ZOOMOUT:
        {
            m_zoom /= 2.0;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        case IDM_EXIT:
        {
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        }
        }
        break;
    }
    case WM_SIZE:
    {
        D2D1_SIZE_U size = D2D1::SizeU(LOWORD(lParam), HIWORD(lParam));

        if (m_pRT)
        {
            // If we couldn't resize, release the device and we'll recreate it
            // during the next render pass.
            if (FAILED(m_pRT->Resize(size)))
            {
                SafeRelease(m_pRT);
                SafeRelease(m_pD2DBitmap);
            }
        }
        break;
    }
    case WM_PAINT:
    {
        return OnPaint(hWnd);
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

/******************************************************************
* Rendering routine using D2D                                     *
******************************************************************/
LRESULT DemoApp::OnPaint(HWND hWnd)
{
    HRESULT hr = S_OK;
    PAINTSTRUCT ps;

    if (BeginPaint(hWnd, &ps))
    {
        // Create render target if not yet created
        hr = CreateDeviceResources(hWnd);

        if (SUCCEEDED(hr) && !(m_pRT->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
        {
            m_pRT->BeginDraw();

            m_pRT->SetTransform(D2D1::Matrix3x2F::Identity());

            // Clear the background
            m_pRT->Clear(D2D1::ColorF(D2D1::ColorF::White));

            D2D1_SIZE_F rtSize = m_pRT->GetSize();

            // Create a rectangle same size of current window
            D2D1_RECT_F rectangle = D2D1::RectF(0.0f, 0.0f, rtSize.width, rtSize.height);

            // D2DBitmap may have been released due to device loss. 
            // If so, re-create it from the source bitmap
            if (m_pBitmapClipper && !m_pD2DBitmap)
            {
                m_pRT->CreateBitmapFromWicBitmap(m_pBitmapClipper, NULL, &m_pD2DBitmap);
                // or re-create it from the ppm data, if a ppm file has to be drawn
            }

            // Draws an image /*and scales it to the current window size*/
            if (m_pD2DBitmap)
            {
                m_pRT->SetTransform(
                    D2D1::Matrix3x2F::Scale(
                        D2D1::Size(m_zoom, m_zoom),
                        D2D1::Point2F(0.0f, 0.0f))
                );
                m_pRT->DrawBitmap(m_pD2DBitmap);
            }

            hr = m_pRT->EndDraw();

            // In case of device loss, discard D2D render target and D2DBitmap
            // They will be re-create in the next rendering pass
            if (hr == D2DERR_RECREATE_TARGET)
            {
                SafeRelease(m_pD2DBitmap);
                SafeRelease(m_pRT);
                // Force a re-render
                hr = InvalidateRect(hWnd, NULL, TRUE) ? S_OK : E_FAIL;
            }
        }

        EndPaint(hWnd, &ps);
    }

    return SUCCEEDED(hr) ? 0 : 1;
}
