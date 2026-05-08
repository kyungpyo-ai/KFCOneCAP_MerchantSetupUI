#pragma once

class CDimDlg
{
public:
    CDimDlg();
    ~CDimDlg();
    BOOL Create();
    void Destroy();
    HWND GetSafeHwnd() const { return m_hWnd; }

private:
    HWND        m_hWnd;
    static ATOM s_atom;
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};