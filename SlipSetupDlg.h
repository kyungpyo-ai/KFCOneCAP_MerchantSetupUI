#pragma once

#include "ModernUI.h" // Modern UI 관련 유틸리티 및 컨트롤 클래스 포함

class CSlipSetupDlg : public CDialog
{
    DECLARE_DYNAMIC(CSlipSetupDlg)

public:
    CSlipSetupDlg(CWnd* pParent = nullptr);
    virtual ~CSlipSetupDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SLIP_SETUP_DLG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnBtnOk();
    afx_msg void OnBtnCancel();
    afx_msg void OnBtnLastPrint();
    afx_msg void OnInfoBtnClicked(UINT nID);
    DECLARE_MESSAGE_MAP()

private:
    // UI 컨트롤
    CModernToggleSwitch m_chkPrintEnable;
    CSkinnedComboBox    m_comboPrintCount;
    CSkinnedComboBox    m_comboSpeed;
    CSkinnedEdit        m_editPort;
    CSkinnedEdit        m_editMsg[6];

    CInfoIconButton     m_btnPrintInfo;
    CInfoIconButton     m_btnPortInfo;
    CInfoIconButton     m_btnSpeedInfo;

    CModernButton       m_btnLastPrint;
    CModernButton       m_btnOk;
    CModernButton       m_btnCancel;

    CModernPopover      m_popover;

    // 리소스 (폰트/브러시)
    CFont   m_fontTitle, m_fontSubtitle, m_fontSection, m_fontLabel, m_fontCombo;
    HFONT   m_hFontCardTitle, m_hFontHdrTitle, m_hFontHdrSub;
    CBrush  m_brBg, m_brWhite;

    UINT_PTR m_uHoverTimer;
    BOOL     m_bUiInitialized;

    // 유틸리티 함수
    void EnsureFonts();
    void LayoutControls();
    void DrawBackground(CDC* pDC);
    void UpdateInputHoverByCursor();
    void ShowInfoPopover(CInfoIconButton& btn, LPCTSTR szTitle, LPCTSTR szBody);
    void LoadFromRegistry();
    void SaveToRegistry();
    int  SX(int px) const;

    virtual void OnOK() override;
    virtual void OnCancel() override;
};