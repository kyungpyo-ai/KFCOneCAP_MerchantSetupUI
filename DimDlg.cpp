#include "stdafx.h"
#include "DimDlg.h"

ATOM CDimDlg::s_atom = 0;

CDimDlg::CDimDlg() : m_hWnd(NULL) {}

CDimDlg::~CDimDlg() { Destroy(); }

BOOL CDimDlg::Create()
{
    if (m_hWnd) return TRUE;

    if (!s_atom)
    {
        WNDCLASSEX wc    = {};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = WndProc;
        wc.hInstance     = ::GetModuleHandle(NULL);
        wc.hCursor       = ::LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = _T("CDimDlgWnd");
        s_atom = ::RegisterClassEx(&wc);
        if (!s_atom) return FALSE;
    }

    int x = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
    int w = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int h = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);

    m_hWnd = ::CreateWindowEx(
        WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        _T("CDimDlgWnd"), NULL,
        WS_POPUP | WS_VISIBLE,
        x, y, w, h,
        NULL, NULL, ::GetModuleHandle(NULL), NULL
    );

    if (!m_hWnd) return FALSE;

    ::SetLayeredWindowAttributes(m_hWnd, 0, 5, LWA_ALPHA);
    ::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    return TRUE;
}

void CDimDlg::Destroy()
{
    if (m_hWnd)
    {
        ::DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }
}

LRESULT CALLBACK CDimDlg::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(hWnd, &ps);
            RECT rc; ::GetClientRect(hWnd, &rc);
            ::FillRect(hdc, &rc, (HBRUSH)::GetStockObject(BLACK_BRUSH));
            ::EndPaint(hWnd, &ps);
            return 0;
        }
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}