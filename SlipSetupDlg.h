// SlipSetupDlg.h
#pragma once
#include "resource.h"
#include "ModernUI.h"
#include <gdiplus.h>

#ifndef IDC_SLIP_PRINT_ENABLE
#define IDC_SLIP_PRINT_ENABLE       6001
#define IDC_SLIP_COMBO_PRINT_COUNT  6002
#define IDC_SLIP_EDIT_PORT          6003
#define IDC_SLIP_COMBO_SPEED        6004
#define IDC_SLIP_EDIT_MSG1          6010
#define IDC_SLIP_EDIT_MSG2          6011
#define IDC_SLIP_EDIT_MSG3          6012
#define IDC_SLIP_EDIT_MSG4          6013
#define IDC_SLIP_EDIT_MSG5          6014
#define IDC_SLIP_EDIT_MSG6          6015
#define IDC_SLIP_BTN_LAST_PRINT     6020
#define IDC_SLIP_BTN_OK             6021
#define IDC_SLIP_BTN_CANCEL         6022
#define IDC_SLIP_BTN_CLOSE          6023
#define IDC_SLIP_BTN_PRINT_INFO     6024
#define IDC_SLIP_BTN_PORT_INFO      6025
#define IDC_SLIP_BTN_SPEED_INFO     6026
#endif

class CSlipSetupDlg : public CDialog
{
    DECLARE_DYNAMIC(CSlipSetupDlg)
public:
    CSlipSetupDlg(CWnd* pParent = nullptr);
    virtual ~CSlipSetupDlg();
    enum { IDD = IDD_SLIP_SETUP_DLG };

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnDestroy();
    afx_msg void OnBtnOk();
    afx_msg void OnBtnCancel();
    afx_msg void OnBtnClose();
    afx_msg void OnBtnLastPrint();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg LRESULT OnNcHitTest(CPoint point);
    afx_msg void OnInfoBtnClicked(UINT nID);
    DECLARE_MESSAGE_MAP()

private:
    void EnsureFonts();
    void LayoutControls();
    void SetClientSize(int cx, int cy);
    void LoadFromRegistry();
    void SaveToRegistry();
    int  SX(int px) const;
    void ShowInfoPopover(CInfoIconButton& btn, LPCTSTR szTitle, LPCTSTR szBody);
    void DrawBackground(CDC* pDC);

    void UpdateInputHoverByCursor();

    CModernToggleSwitch  m_chkPrintEnable;
    CSkinnedComboBox     m_comboPrintCount;
    CSkinnedEdit         m_editPort;
    CSkinnedComboBox     m_comboSpeed;
    CSkinnedEdit         m_editMsg[6];
    CModernButton        m_btnLastPrint;
    CModernButton        m_btnOk;
    CModernButton        m_btnCancel;
    CModernButton        m_btnClose;
    CInfoIconButton      m_btnPrintInfo;
    CInfoIconButton      m_btnPortInfo;
    CInfoIconButton      m_btnSpeedInfo;
    CModernPopover       m_popover;

    CFont  m_fontTitle;
    CFont  m_fontSubtitle;
    CFont  m_fontSection;
    CFont  m_fontLabel;
    CFont  m_fontCombo;
    HFONT  m_hFontCardTitle;
    HFONT  m_hFontHdrTitle;
    HFONT  m_hFontHdrSub;

    CBrush m_brBg;
    CBrush m_brWhite;

    enum { TIMER_HOVER = 0x5C01 };
    UINT_PTR m_uHoverTimer;
    BOOL     m_bUiInitialized;
};
