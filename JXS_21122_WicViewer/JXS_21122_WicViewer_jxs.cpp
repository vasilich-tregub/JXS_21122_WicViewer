/* TERMS OF USE
 * This source code is subject to the terms of the MIT License.
 * Copyright(c) 2026 Vladimir Vasilich Tregub
*/
/* Code for function DecodeJPEGXSSequence is borrowed from 
 * ISO_IEC_21122-5_ED-3, compilation unit xs_dec_main.c, 
 * copyrigtht holders intoPIX SA, Fraunhofer IIS and Canon Inc.
*/
#include "framework.h"
#include <wincodec.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include "JXS_21122_WicViewer.h"

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

uint16_t byteswap(uint16_t val)
{
    uint8_t hi = (val & 0xff00) >> 8;
    uint8_t lo = (val & 0xff);
    return val;// (lo << 8 | hi);
}

xs_image_t copy_xs_image(xs_image_t* xs_image)
{
    xs_image_t ret_image;
    ret_image.width = xs_image->width;
    ret_image.height = xs_image->height;
    ret_image.depth = xs_image->depth;
    ret_image.ncomps = xs_image->ncomps;
    std::copy(std::begin(xs_image->sx), std::end(xs_image->sx), std::begin(ret_image.sx));
    std::copy(std::begin(xs_image->sy), std::end(xs_image->sy), std::begin(ret_image.sy));
    for (int i = 0; i < xs_image->ncomps; ++i)
    {
        const size_t sample_count = (size_t)(xs_image->width / xs_image->sx[i]) * (size_t)(xs_image->height / xs_image->sy[i]);
        ret_image.comps_array[i] = (xs_data_in_t*)malloc(sample_count * sizeof(xs_data_in_t));
        memcpy_s(ret_image.comps_array[i], sizeof(xs_data_in_t) * sample_count, xs_image->comps_array[i], sizeof(xs_data_in_t) * sample_count);
    }
    return ret_image;
}

HRESULT DemoApp::DecodeJPEGXSSequence(HWND hWnd)
{
    HRESULT hr = S_OK;

    for (auto xs_image : m_jxsimages)
    {
        xs_free_image(&xs_image);
    }
    m_jxsimages.clear();
    m_jxsimages.shrink_to_fit();
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
    
    // Step 1: Create the open dialog box and locate the image file
    std::vector<std::wstring> paths;
    if (LocateJXSFile(paths))
    {
        SetCursor(LoadCursor(nullptr, IDC_WAIT));
    
        if (paths.size() == 0)
        {
            MessageBox(hWnd, L"No image files selected", L"Selecting images", MB_OK);
            return S_OK;
        }

        wchar_t shortPath[260];
        long retlen = GetShortPathName(paths[0].data(), shortPath, 260);
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

        if (!xs_dec_probe(bitstream_buf, bitstream_buf_size, &m_xsConfig, &image))
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

        ctx = xs_dec_init(&m_xsConfig, &image);
        if (!ctx)
        {
            fprintf(stderr, "Error parsing the codestream\n");
            ret = -1;
            return ret;
        }

        if (!xs_dec_bitstream(ctx, 0, bitstream_buf, bitstream_buf_max_size, &image))
        {
            fprintf(stderr, "Error while decoding the codestream\n");
            ret = -1;
            return ret;
        }

        if (!xs_dec_postprocess_image(&m_xsConfig, &image))
        {
            fprintf(stderr, "Error postprocessing the image\n");
            ret = -1;
            return ret;
        }

        m_jxsimages.push_back(copy_xs_image(&image));

        for (int ix = 1; ix < paths.size(); ++ix)
        {
            long retlen = GetShortPathName(paths[ix].data(), shortPath, 260);
            errno_t err = wcstombs_s(&retSize, filename, 260, shortPath, 259);
            fileio_read(input_fn, bitstream_buf, &bitstream_buf_size, bitstream_buf_max_size);

            if (!xs_dec_bitstream(ctx, 0, bitstream_buf, bitstream_buf_max_size, &image))
            {
                fprintf(stderr, "Error while decoding the codestream\n");
                ret = -1;
                return ret;
            }

            if (!xs_dec_postprocess_image(&m_xsConfig, &image))
            {
                fprintf(stderr, "Error postprocessing the image\n");
                ret = -1;
                return ret;
            }

            m_jxsimages.push_back(copy_xs_image(&image));
        }

        if (input_fn)
        {
            //free(input_fn);
        }
        if (output_fn)
        {
            free(output_fn);
        }
        xs_free_image(&image);
        if (ctx)
        {
            xs_dec_close(ctx);
        }
        if (bitstream_buf)
        {
            free(bitstream_buf);
        }
        
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
    }
    
    m_xsImageIx = 0;
    CreateD2DBitmapFromJPEGXSFile(hWnd, &m_xsConfig, &m_jxsimages[m_xsImageIx]);
    
    return hr;
}
HRESULT DemoApp::CreateD2DBitmapFromJPEGXSFile(HWND hWnd, xs_config_t* xs_config, xs_image_t* image)
{
    HRESULT hr = S_OK;
    IWICBitmap* jxsBitmap = nullptr;
    IWICFormatConverter* convertedSourceBitmap = nullptr;
    //Step 3: Copy aligned image memory data to buffer (TODO: consider big endian case) and create render target
    if (SUCCEEDED(hr))
    {
        if (image->ncomps == 3 || image->ncomps == 4)
        {
            if (image->sx[0] != 1 || image->sx[1] != 1 || image->sx[2] != 1 ||
                image->sy[0] != 1 || image->sy[1] != 1 || image->sy[2] != 1)
            {
                if (!((image->sx[1] == 2 && image->sx[2] == 2) && (image->sy[1] == 1 && image->sy[2] == 1)) &&
                    !((image->sx[1] == 2 && image->sx[2] == 2) && (image->sy[1] == 2 && image->sy[2] == 2)))
                {
                    MessageBox(hWnd, L"This subsampling pattern has yet to be implemented in viewer, come back later.", L"Application Capabilities", MB_ICONEXCLAMATION | MB_OK);
                    return E_NOTIMPL;
                }
            }
            m_pPxlData = (uint8_t*)malloc(4 * image->width * image->height);// .resize(4 * image->width * image->height);
        }
        else
        {
            if (image->ncomps == 1)
            {
                m_pPxlData = (uint8_t*)malloc(4 * image->width * image->height);//.resize(4 * image->width * image->height);
            }
            else
            {
                MessageBox(hWnd, L"Viewing of other than 1, 3 or 4 component images has yet to be implemented, come back later.", L"Application Capabilities", MB_ICONEXCLAMATION | MB_OK);
                return E_NOTIMPL;
            }
        }
        PlanarToPackaged(*xs_config, *image, m_pPxlData);

    }

    if (SUCCEEDED(hr))
    {
        hr = m_pIWICFactory->CreateBitmapFromMemory(image->width, image->height,
            GUID_WICPixelFormat32bppPRGBA, 4 * image->width,
            (unsigned int)(4 * image->width * image->height), m_pPxlData,
            &jxsBitmap);
    }

    m_width = (image->width > 8192) ? 8192 : image->width;
    m_height = (image->height > 8192) ? 8192 : image->height;

    if (SUCCEEDED(hr))
    {
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
        //title += std::wstring(paths[0].data());
        //SetWindowTextW(hWnd, title.c_str());
    }

    if (m_pPxlData)
    {
        free(m_pPxlData);
    }
    SafeRelease(m_pBitmapClipper);
    SafeRelease(convertedSourceBitmap);
    SafeRelease(jxsBitmap);

    return hr;
}
BOOL DemoApp::LocateJXSFile(std::vector<std::wstring>& paths)
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
        filters[0].pszName = L"All JPEG XS Files.";
        filters[0].pszSpec = L"*.jxs;*.jxc";
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

