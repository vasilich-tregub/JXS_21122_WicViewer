// JXS_21122_WicViewer.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <wincodec.h>
#include <commdlg.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include "JXS_21122_WicViewer.h"

#include "libjxs.h"
#include "PlanarToPackaged.h"
extern "C" {
#include "file_io.h"
}
#ifdef _DEBUG
#pragma comment(lib, "jxsd.lib")
#else
#pragma comment(lib, "jxs.lib")
#endif // DEBUG

#include <shlwapi.h>
#include <string>
#pragma comment(lib, "shlwapi")

template <typename T>
inline void SafeRelease(T*& p)
{
    if (NULL != p)
    {
        p->Release();
        p = NULL;
    }
}

uint16_t byteswap(uint16_t val)
{
    uint8_t hi = (val & 0xff00) >> 8;
    uint8_t lo = (val & 0xff);
    return val;// (lo << 8 | hi);
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

    WCHAR szFileName[MAX_PATH];

    // Step 1: Create the open dialog box and locate the image file
    if (LocateImageFile(hWnd, szFileName, ARRAYSIZE(szFileName)))
    {
        // Step 2: Decode the source image

        // Create a decoder
        IWICBitmapDecoder* pDecoder = NULL;

        hr = m_pIWICFactory->CreateDecoderFromFilename(
            szFileName,                      // Image to be decoded
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
            title += std::wstring(szFileName);
            SetWindowTextW(hWnd, title.c_str());
        }

        SafeRelease(pDecoder);
        SafeRelease(pFrame);
    }

    return hr;
}

HRESULT DemoApp::CreateD2DBitmapFromJPEGXSFile(HWND hWnd)
{
    HRESULT hr = S_OK;

    WCHAR szFileName[MAX_PATH];

    xs_config_t xs_config{ 0 };
    char* input_seq_n = NULL;
    char* output_seq_n = NULL;
    char* input_fn = NULL;
    char* output_fn = NULL;
    xs_image_t image = { 0 };
    uint8_t* bitstream_buf = NULL;
    size_t bitstream_buf_size, bitstream_buf_max_size;
    xs_dec_context_t* ctx = NULL;
    int ret = 0;
    int file_idx = 0;

    IWICBitmap* jxsBitmap = nullptr;
    IWICFormatConverter* convertedSourceBitmap = nullptr;

    // Step 1: Create the open dialog box and locate the image file
    if (LocateJXSFile(hWnd, szFileName, ARRAYSIZE(szFileName)))
    {
        SetCursor(LoadCursor(nullptr, IDC_WAIT));

        wchar_t shortPath[260];
        long retlen = GetShortPathName(szFileName, shortPath, 260);
        char filename[260];
        size_t retSize = 0;
        errno_t err = wcstombs_s(&retSize, filename, 260, shortPath, 259);

        input_fn = &filename[0];
        bitstream_buf_max_size = fileio_getsize(input_fn);
        if (bitstream_buf_max_size <= 0)
        {
            fprintf(stderr, "Unable to open file %s\n", input_seq_n);
            ret = -1;
            return ret;
        }

        bitstream_buf = (uint8_t*)malloc((bitstream_buf_max_size + 8) & (~0x7));
        if (!bitstream_buf)
        {
            fprintf(stderr, "Unable to allocate memory for codestream\n");
            ret = -1;
            return ret;
        }
        fileio_read(input_fn, bitstream_buf, &bitstream_buf_size, bitstream_buf_max_size);

        if (!xs_dec_probe(bitstream_buf, bitstream_buf_size, &xs_config, &image))
        {
            fprintf(stderr, "Unable to parse input codestream (%s)\n", input_fn);
            ret = -1;
            return ret;
        }

        if (!xs_allocate_image(&image, false))
        {
            fprintf(stderr, "Image memory allocation error\n");
            ret = -1;
            return ret;
        }

        ctx = xs_dec_init(&xs_config, &image);
        if (!ctx)
        {
            fprintf(stderr, "Error parsing the codestream\n");
            ret = -1;
            return ret;
        }

        if (!xs_dec_bitstream(ctx, bitstream_buf, bitstream_buf_max_size, &image))
        {
            fprintf(stderr, "Error while decoding the codestream\n");
            ret = -1;
            return ret;
        }

        if (!xs_dec_postprocess_image(&xs_config, &image))
        {
            fprintf(stderr, "Error postprocessing the image\n");
            ret = -1;
            return ret;
        }

        //Step 3: Copy aligned image memory data to buffer (TODO: consider big endian case) and create render target
        if (SUCCEEDED(hr))
        {
            if (image.ncomps == 3 || image.ncomps == 4)
            {
                if (image.sx[0] != 1 || image.sx[1] != 1 || image.sx[2] != 1 ||
                    image.sy[0] != 1 || image.sy[1] != 1 || image.sy[2] != 1)
                {
                    if (!((image.sx[1] == 2 && image.sx[2] == 2) && (image.sy[1] == 1 && image.sy[2] == 1)) &&
                        !((image.sx[1] == 2 && image.sx[2] == 2) && (image.sy[1] == 2 && image.sy[2] == 2)))
                    {
                        MessageBox(hWnd, L"This subsampling pattern has yet to be implemented in viewer, come back later.", L"Application Capabilities", MB_ICONEXCLAMATION | MB_OK);
                        return E_NOTIMPL;
                    }
                }
                m_pPxlData.resize(4 * image.width * image.height);
            }
            else 
                if (image.ncomps == 1)
                {
                    m_pPxlData.resize(4 * image.width * image.height);
                }
            else
            {
                MessageBox(hWnd, L"Viewing of other than 1, 3 or 4 component images has yet to be implemented, come back later.", L"Application Capabilities", MB_ICONEXCLAMATION | MB_OK);
                return E_NOTIMPL;
            }
            PlanarToPackaged(xs_config, image, m_pPxlData);
        }

        if (SUCCEEDED(hr))
        {
            SafeRelease(jxsBitmap);
            hr = m_pIWICFactory->CreateBitmapFromMemory(image.width, image.height,
                GUID_WICPixelFormat32bppPRGBA, 4 * image.width,
                (unsigned int)m_pPxlData.size(), m_pPxlData.data(),
                &jxsBitmap);
        }

        m_width = (image.width > 8192) ? 8192 : image.width;
        m_height = (image.height > 8192) ? 8192 : image.height;

        if (SUCCEEDED(hr))
        {
            SafeRelease(convertedSourceBitmap);
            hr = m_pIWICFactory->CreateFormatConverter(&convertedSourceBitmap);
        }

        if (SUCCEEDED(hr))
        {
            hr = convertedSourceBitmap->Initialize(
                jxsBitmap,                          // Input bitmap to convert
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

        //Step 4: Create render target and D2D bitmap from IWICBitmap
        if (SUCCEEDED(hr))
        {
            hr = CreateDeviceResources(hWnd);
        }

        if (SUCCEEDED(hr))
        {
            // Need to release the previous D2DBitmap if there is one
            SafeRelease(m_pD2DBitmap);
            hr = m_pRT->CreateBitmapFromWicBitmap(m_pBitmapClipper, NULL, &m_pD2DBitmap);
            std::wstring title = L"JPEG XS File ";
            title += std::wstring(szFileName);
            SetWindowTextW(hWnd, title.c_str());
        }

        SetCursor(LoadCursor(nullptr, IDC_ARROW));
    }
    xs_free_image(&image);
    return hr;
}

/******************************************************************
* Creates an open file dialog box and locate the image to decode. *
******************************************************************/

BOOL DemoApp::LocateImageFile(HWND hWnd, LPWSTR pszFileName, DWORD cchFileName)
{
    pszFileName[0] = L'\0';

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"All Image Files\0"              L"*.bmp;*.dib;*.wdp;*.mdp;*.hdp;*.gif;*.png;*.jpg;*.jpeg;*.tif;*.ico\0"
        L"Windows Bitmap\0"               L"*.bmp;*.dib\0"
        L"High Definition Photo\0"        L"*.wdp;*.mdp;*.hdp\0"
        L"Graphics Interchange Format\0"  L"*.gif\0"
        L"Portable Network Graphics\0"    L"*.png\0"
        L"JPEG File Interchange Format\0" L"*.jpg;*.jpeg\0"
        L"Tiff File\0"                    L"*.tif\0"
        L"Icon\0"                         L"*.ico\0"
        L"All Files\0"                    L"*.*\0"
        L"\0";
    ofn.lpstrFile = pszFileName;
    ofn.nMaxFile = cchFileName;
    ofn.lpstrTitle = L"Open Image";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    // Display the Open dialog box. 
    return GetOpenFileName(&ofn);
}

BOOL DemoApp::LocateJXSFile(HWND hWnd, LPWSTR pszFileName, DWORD cchFileName)
{
    pszFileName[0] = L'\0';

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"All JPEG XS Files\0"              L"*.jxs;*.jxc\0"
        L"All Files\0"                    L"*.*\0"
        L"\0";
    ofn.lpstrFile = pszFileName;
    ofn.nMaxFile = cchFileName;
    ofn.lpstrTitle = L"Open JPEG XS Image";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    // Display the Open dialog box. 
    return GetOpenFileName(&ofn);
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
