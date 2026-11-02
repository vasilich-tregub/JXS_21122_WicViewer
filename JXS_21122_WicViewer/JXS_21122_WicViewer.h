/* TERMS OF USE
 * This source code is subject to the terms of the MIT License.
 * Copyright(c) 2026 Vladimir Vasilich Tregub
*/
#pragma once

#include "Resource.h"
#include <string>
#include <vector>
#include "libjxs.h"

const FLOAT DEFAULT_DPI = 96.f;   // Default DPI that maps image resolution directly to screen resoltuion

/******************************************************************
*                                                                 *
*  DemoApp                                                        *
*                                                                 *
******************************************************************/

class DemoApp
{
public:
    DemoApp();
    ~DemoApp();

    HRESULT Initialize(HINSTANCE hInstance);

private:
    HRESULT CreateD2DBitmapFromFile(HWND hWnd);
    BOOL LocateImageFile(std::vector<std::wstring>& paths);
    HRESULT DecodeJPEGXSSequence(HWND hWnd);
    HRESULT CreateD2DBitmapFromJPEGXSFile(HWND hWnd, xs_config_t* xs_config, xs_image_t* image);
    BOOL LocateJXSFile(std::vector<std::wstring>& paths);
    HRESULT CreateDeviceResources(HWND hWnd);

    LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT OnPaint(HWND hWnd);

    static LRESULT CALLBACK s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:

    HINSTANCE               m_hInst;
    IWICImagingFactory* m_pIWICFactory;

    ID2D1Factory* m_pD2DFactory;
    ID2D1HwndRenderTarget* m_pRT;
    ID2D1Bitmap* m_pD2DBitmap;
    uint8_t* m_pPxlData;
    IWICBitmapClipper* m_pBitmapClipper;
    float m_zoom = 1.0f;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    xs_config_t m_xsConfig = { 0 };
    std::vector<xs_image_t> m_jxsimages;
    int m_xsImageIx = 0;
};
