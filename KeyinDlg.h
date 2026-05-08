#pragma once
#include "resource.h"
#include "ModernUI.h"

#ifndef IDD_KEYIN_DIALOG
#define IDD_KEYIN_DIALOG 193
#endif

#define RET_MSR    3
#define RET_CANCEL 4
#define WM_KEYIN_HOOK_KEY  (WM_APP + 200)

class CKeyinDlg : public CDialog
{
public:
    enum { IDD = IDD_KEYIN_DIALOG };

    int     m_keyinKind;   // 1=카드번호, 2=휴대폰/사업자번호, 3=비밀번호
    CString m_cardnum;     // 순수 숫자+= (하이픈 없음) -- 최종 출력값

    CKeyinDlg(int nKind, CWnd* pParent = NULL);
    virtual ~CKeyinDlg();

protected:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void DoDataExchange(CDataExchange* pDX);

    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnBtnClicked(UINT nID);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg LRESULT OnHookKey(WPARAM wParam, LPARAM lParam);
    afx_msg void OnDestroy();
    afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);

    DECLARE_MESSAGE_MAP()

private:
    CString m_strInputData;     // raw data (digits + '=' for card mode)
    CString m_strCancelHotkey;  // from registry SERIALPORT/CANCEL_HOTKEY
    CString m_strMsrHotkey;     // from registry SERIALPORT/MSR_HOTKEY
    BOOL    m_bCursorVisible;
    BOOL    m_bInputEnabled;

    CModernButton m_btnDigit[10]; // IDC_KEYIN_BTN_0..9
    CModernButton m_btnDel;       // IDC_KEYIN_BTN_DEL
    CModernButton m_btnSpec;      // IDC_KEYIN_BTN_SPEC (= or MSR 전환 or hidden)
    CModernButton m_btnOk;        // IDC_KEYIN_BTN_OK
    CModernButton m_btnCancel;    // IDC_KEYIN_BTN_CANCEL

    CFont  m_fontTitle;
    CFont  m_fontBtn;
    CBrush m_brDlgBg;

    void    EnsureFonts();
    int     SX(int v) const;
    void    LayoutControls();
    void    AppendDigit(TCHAR ch);
    void    DeleteLast();
    void    UpdateDisplay();
    CString FormatDisplay() const;
    void    LoadHotkeySettings();
    BOOL    HotkeyMatchesVK(const CString& strKey, UINT nVK) const;
    int     GetMaxInputLen() const;
    int     CountDigits() const;
    void    UpdateOkButtonState();
    void    DrawDisplayText(HDC hdc, const CRect& rcDisplay, const CString& strFormatted);

    static HHOOK   s_hKbHook;
    static HWND    s_hWndTarget;
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
};