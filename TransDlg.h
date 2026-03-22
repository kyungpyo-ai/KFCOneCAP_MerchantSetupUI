#pragma once
#include "resource.h"
#include "ModernUI.h"
#include <vector>

#ifndef IDD_TRANS_DIALOG
#define IDD_TRANS_DIALOG 190
#endif

// ============================================================
// CSegmentCtrl - 세그먼트 컨트롤 (결제창.png / transDlg.html 스타일)
//   회색 pill 배경 + 선택된 탭은 흰색 pill + 파란 굵은 텍스트
//   TCN_SELCHANGE 알림을 부모에게 WM_NOTIFY로 전송
// ============================================================
class CSegmentCtrl : public CWnd
{
public:
    static const int kBarH = 46;

    CSegmentCtrl() : m_nSel(0), m_crUnderlay(RGB(255,255,255)) {}

    BOOL Create(CWnd* pParent, UINT nID, const CRect& rc);
    void AddTab(LPCTSTR text);
    int  GetCurSel() const { return m_nSel; }
    void SetCurSel(int n);   // 알림 포함 (사용자 클릭과 동일)
    void SetCurSelSilent(int n); // 알림 없이 선택만 변경
    int  GetCount()  const { return (int)m_tabs.size(); }
    void SetUnderlayColor(COLORREF cr) { m_crUnderlay = cr; }

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
    DECLARE_MESSAGE_MAP()

private:
    int Scale(int v) const { return ModernUIDpi::Scale(m_hWnd, v); }
    void NotifyParent();

    std::vector<CString> m_tabs;
    int      m_nSel;
    COLORREF m_crUnderlay;
};

// ============================================================
// CTransDlg
// ============================================================
class CTransDlg : public CDialog
{
public:
    enum { IDD = IDD_TRANS_DIALOG };
    CTransDlg(CWnd* pParent = NULL);
    virtual ~CTransDlg();

protected:
    enum ETransMode { MODE_CREDIT_APPROVAL=0, MODE_CREDIT_CANCEL, MODE_CASH_APPROVAL, MODE_CASH_CANCEL };

    struct ResultPair { LPCTSTR pszLabel; CString value; BOOL bBlue; BOOL bRed; };
    struct FieldPair  { CWnd* pCtrl; CString caption; BOOL bFullRow; UINT ctrlType; };

    void EnsureFonts();
    int  SX(int v) const;
    void LayoutControls();
    void CreateSegmentControl();
    void CreateInputControls();
    void CreateResultControls();
    void CreateBottomButtons();
    void ApplyFonts();
    void SetMode(ETransMode mode);
    void ShowFieldsForMode();
    void UpdateResultControls();
    void ResetSampleResult();
    void ResizeForCurrentMode();
    BOOL ValidateCurrentMode(CString& e);
    CString BuildSampleMessage() const;
    CString GetCurrentModeName() const;
    void SetResultValue(int i, LPCTSTR v, BOOL bBlue=FALSE, BOOL bRed=FALSE);
    void DrawSectionTitle(CDC& dc, const CRect& rc, LPCTSTR text);
    void DrawRoundedCard(Gdiplus::Graphics& g, const CRect& rc, int radius,
                         COLORREF fill, COLORREF border, int shadowAlpha=8);
    int  GetVisibleFieldRowCount() const;
    int  GetSetupCardHeight() const;
    int  GetResultCardHeight() const;

    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    virtual void OnOK();
    virtual void OnCancel();

    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()

private:
    enum {
        IDC_TRANS_SEG = 49999,
        IDC_TRANS_EDIT_AMOUNT, IDC_TRANS_EDIT_TAX, IDC_TRANS_EDIT_TIP,
        IDC_TRANS_EDIT_TAXFREE, IDC_TRANS_COMBO_INSTALLMENT,
        IDC_TRANS_EDIT_ORG_DATE, IDC_TRANS_EDIT_ORG_APPROVAL,
        IDC_TRANS_EDIT_CASH_NO, IDC_TRANS_COMBO_CASH_TYPE,
        IDC_TRANS_COMBO_CANCEL_REASON,
        IDC_TRANS_BTN_CLOSE, IDC_TRANS_BTN_RUN,
        IDC_TRANS_LABEL_BASE      = 51000,
        IDC_TRANS_RESULT_LBL_BASE = 51100,
        IDC_TRANS_VALUE_BASE      = 51200
    };

    ETransMode m_eMode;
    BOOL       m_bUiBuilt;
    CBrush     m_brBack;
    CFont      m_fontTitle, m_fontSub, m_fontSection;
    CFont      m_fontLabel, m_fontEdit, m_fontAmount;
    CFont      m_fontResultLabel, m_fontResultValue;
    CFont      m_fontResultBlue,  m_fontResultRed;

    CSegmentCtrl     m_segCtrl;
    CModernButton    m_btnClose, m_btnRun;
    CSkinnedEdit     m_edtAmount, m_edtTax, m_edtTip, m_edtTaxFree;
    CSkinnedEdit     m_edtOrgDate, m_edtOrgApproval, m_edtCashNo;
    CSkinnedComboBox m_cmbInstallment, m_cmbCashType, m_cmbCancelReason;

    std::vector<FieldPair>  m_fields;
    std::vector<ResultPair> m_results;
    CStatic m_fieldLabels[10];
    CStatic m_resultLabels[10];
    CStatic m_resultValues[10];
};
