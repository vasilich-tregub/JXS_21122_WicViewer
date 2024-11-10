#pragma once

#include "resource.h"
#include <vector>

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
    HRESULT CreateD2DBitmapFromNETPBMFile(HWND hWnd);
    HRESULT CreateD2DBitmapFromJPEGXSFile(HWND hWnd);
    BOOL LocateImageFile(HWND hWnd, LPWSTR pszFileName, DWORD cbFileName);
    BOOL LocateNETPBMFile(HWND hWnd, LPWSTR pszFileName, DWORD cbFileName);
    BOOL LocateJXSFile(HWND hWnd, LPWSTR pszFileName, DWORD cbFileName);
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
    std::vector<uint8_t> m_pPxlData;
    IWICBitmapClipper* m_pBitmapClipper;
    float m_zoom = 1.0f;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};
