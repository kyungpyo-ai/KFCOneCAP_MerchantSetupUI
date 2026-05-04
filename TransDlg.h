#pragma once
#include "resource.h"
#include "ModernUI.h"
#include <vector>

#ifndef IDD_TRANS_DIALOG
#define IDD_TRANS_DIALOG 190
#endif

class CSegmentCtrl : public CWnd
{
public:
    static const int kBarH = 46;
    CSegmentCtrl() : m_nSel(0), m_nHover(-1), m_nPress(-1), m_crUnderlay(RGB(255,255,255)), m_pGdipNormal(NULL), m_pGdipBold(NULL) {}
    ~CSegmentCtrl();
    BOOL Create(CWnd* pParent, UINT nID, const CRect& rc);
    void AddTab(LPCTSTR text);
    int  GetCurSel() const { return m_nSel; }
    void SetCurSel(int n);
    void SetCurSelSilent(int n);
    int  GetCount() const { return (int)m_tabs.size(); }
    void SetUnderlayColor(COLORREF cr) { m_crUnderlay = cr; }
protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint pt);
    afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
    afx_msg void OnMouseLeave();
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    DECLARE_MESSAGE_MAP()
private:
    int Scale(int v) const { return ModernUIDpi::Scale(m_hWnd, v); }
    int HitTab(CPoint pt) const;
    void NotifyParent();
    std::vector<CString> m_tabs;
    int      m_nSel;
    int      m_nHover;
    int      m_nPress;
    COLORREF m_crUnderlay;
    CFont    m_fontNormal, m_fontBold;
    Gdiplus::Font* m_pGdipNormal;
    Gdiplus::Font* m_pGdipBold;
};

class CTransDlg : public CDialog
{
public:
    enum { IDD = IDD_TRANS_DIALOG };
    CTransDlg(CWnd* pParent = NULL);
    virtual ~CTransDlg();

protected:
    enum EFieldIndex {
        F_SUPPLY=0,F_TAX,F_TIP,F_TAXFREE,F_INSTALL,F_QR,F_ORGDATE,F_ORGAPPNO,F_CASHTYPE,F_CASHNO
    };
    enum ETransMode {
        MODE_CREDIT_APPROVAL = 0, MODE_CREDIT_CANCEL,
        MODE_CASH_APPROVAL,       MODE_CASH_CANCEL
    };
    struct ResultPair { LPCTSTR pszLabel; CString value; BOOL bBlue; BOOL bRed; };
    struct FieldPair  { CWnd* pCtrl; CString caption; BOOL bFullRow; UINT ctrlType; };

    void EnsureFonts();
    int  SX(int v) const;
    void LayoutControls();
    void CreateSegmentControl();
    void CreateInputControls();
    void CreateResultControls();
    void CreateBottomButton();
    void ApplyFonts();
    void SetMode(ETransMode mode);
    void ShowFieldsForMode();
    void UpdateResultControls();
    void ClearResult();
    void ResetSampleResult();
    void ResizeWindow();
    BOOL ValidateCurrentMode(CString& e);
    void OnRunCreditApproval();
    void OnRunCreditCancel();
    void OnRunCashApproval();
    void OnRunCashCancel();
    CString GetModeButtonText() const;
    CString GetCurrentModeName() const;
    void SetResultValue(int i, LPCTSTR v, BOOL bBlue=FALSE, BOOL bRed=FALSE);
    void ApplyResultColoring();
    void DrawRoundedCard(Gdiplus::Graphics& g, const CRect& rc, int radius,
                         COLORREF fill, COLORREF border, int shadowAlpha=0);
    void GetContentRects(CRect& rcForm, CRect& rcResult) const;

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

    static const int kNumFields  = 10;
    static const int kNumResults = 15;
    static const int kResRowH    = 30;

    ETransMode m_eMode;
    BOOL       m_bUiBuilt;
    CBrush     m_brBack;
    CFont      m_fontTitle, m_fontSub, m_fontSection;
    CFont      m_fontLabel, m_fontEdit, m_fontAmount;
    CFont      m_fontResultLabel, m_fontResultValue;
    CFont      m_fontResultBlue,  m_fontResultRed;
    CFont      m_fontBadge;
    Gdiplus::Font* m_pGdiFontSection;
    Gdiplus::Font* m_pGdiFontTitle;
    Gdiplus::Font* m_pGdiFontSub;
    Gdiplus::Font* m_pGdiFontResultLabel;
    Gdiplus::Font* m_pGdiFontResultValue;
    Gdiplus::Font* m_pGdiFontResultBlue;
    Gdiplus::Font* m_pGdiFontResultRed;
    Gdiplus::Font* m_pGdiFontBadge;
    Gdiplus::Font* m_pGdiFontLabel;
    Gdiplus::Font* m_pGdiFontAmount;
    CString    m_strBadge;
    BOOL       m_bBadgeOk;

    CSegmentCtrl     m_segCtrl;
    CModernButton    m_btnClose, m_btnRun;

    CSkinnedEdit     m_edtSupply;
    CSkinnedEdit     m_edtTax;
    CSkinnedEdit     m_edtTip;
    CSkinnedEdit     m_edtTaxFree;
    CSkinnedEdit     m_edtInstall; 
    CSkinnedEdit     m_edtQr;
    CSkinnedEdit     m_edtOrgDate;
    CSkinnedEdit     m_edtOrgAppNo;
    CSkinnedComboBox m_cmbCashType;
    CSkinnedEdit     m_edtCashNo;

    std::vector<FieldPair>  m_fields;
    std::vector<ResultPair> m_results;
    CString m_tabValues[4][kNumFields]; // per-tab saved values [mode][fieldIdx]
    CStatic m_fieldLabels[kNumFields];
    CStatic m_resultLabels[kNumResults];
    CEdit   m_resultValues[kNumResults];
};
