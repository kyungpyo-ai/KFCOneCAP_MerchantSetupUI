#pragma once
#include "resource.h"
#include "ModernUI.h"

class CShopDownDlg : public CDialog
{
    DECLARE_DYNAMIC(CShopDownDlg)

public:
    enum { IDD = IDD_SHOP_DOWN_DIALOG };

    CShopDownDlg(CWnd* pParent = nullptr);
    virtual ~CShopDownDlg();

protected:
    virtual BOOL OnInitDialog();

    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnTimer(UINT_PTR nIDEvent);    virtual BOOL PreTranslateMessage(MSG* pMsg);  // 자식 컨트롤 위 휠 캡처
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    void CreateControlsOnce();
    void LayoutControls();
    void ApplyFonts();
    void UpdateScrollBar();
    void ApplyScroll(int newPos);
    void QueueScroll(int newPos);
    void OnDownloadClick(int index);

    // ---------- Column layout -------------------------------------------------
    // Single source of truth - used by LayoutControls AND OnPaint.
    // All pixel values are 96-DPI base; call Compute() after DPI is known.
    struct CardLayout
    {
        int cardM;      // section card inner margin (same as kSecM)
        int cardPad;    // padding inside each item card
        int colGap;     // gap between left/right columns

        int idxW;      // row number gutter width

        int leftW;      // left column width (inputs)
        int rightW;     // right column width (info)

        int editGap;    // gap between edits (within left col)
        int labelH;     // label text height
        int labelGap;   // gap between label and control/text
        int infoLineH;  // height of each info value line (right col)

        int btnW;       // download button width
        int btnH;       // download button height

        void Compute(int clientWidth, HWND hwnd);
    };
    CardLayout m_card;
// ---------- Layout constants (96-DPI base, Scale() at runtime) -------------
    static const int kHdrAreaH = 44;   // 타이틀 헤더 영역 높이
    static const int kSecGapT  = 6;    // 헤더~섹션카드 상단 간격
    static const int kSecM     = 18;   // 섹션카드 좌우하단 마진 (다른 탭과 동일)
    static const int kColHdrH  = 0;   // 컬럼 헤더 행 높이
        static const int kRowH     = 190;  // data row height (approx) - show 2 cards in view
    static const int kRowGap   = 12;   // gap between cards
            static const int kCtrlH    = 42;   // control height inside row (match other tabs, taller)   // control height inside row (match other tabs, slightly taller)   // control height inside row (match other tabs)

    // contentStartY = kHdrAreaH + kSecGapT + Scale(kColHdrH)
    // kHdrAreaH/kSecGapT는 fixed px, kColHdrH만 DPI 스케일

    static const int kRowCount = 25;
    static const int kBtnBase  = 61001;

private:
    // ---------- Brushes -------------------------------------------------------
    CBrush m_brushBg;    // dialog background RGB(249,250,252)
    CBrush m_brushCard;  // row card / control background RGB(255,255,255)
    CBrush m_brushCardDisabled; // disabled edit background RGB(245,246,248)

    // ---------- Fonts ---------------------------------------------------------
    CFont m_fontHeader;   // column header labels
    CFont m_fontCell;     // edit / button text
    CFont m_fontBadge;    // row number badge

    // ---------- Data strings --------------------------------------------------
    CString m_prdid1,  m_prdid2,  m_prdid3,  m_prdid4,  m_prdid5;
    CString m_prdid6,  m_prdid7,  m_prdid8,  m_prdid9,  m_prdid10;
    CString m_prdid11, m_prdid12, m_prdid13, m_prdid14, m_prdid15;
    CString m_prdid16, m_prdid17, m_prdid18, m_prdid19, m_prdid20;
    CString m_prdid21, m_prdid22, m_prdid23, m_prdid24, m_prdid25;

    CString m_regno1,  m_regno2,  m_regno3,  m_regno4,  m_regno5;
    CString m_regno6,  m_regno7,  m_regno8,  m_regno9,  m_regno10;
    CString m_regno11, m_regno12, m_regno13, m_regno14, m_regno15;
    CString m_regno16, m_regno17, m_regno18, m_regno19, m_regno20;
    CString m_regno21, m_regno22, m_regno23, m_regno24, m_regno25;

    CString m_passwd1,  m_passwd2,  m_passwd3,  m_passwd4,  m_passwd5;
    CString m_passwd6,  m_passwd7,  m_passwd8,  m_passwd9,  m_passwd10;
    CString m_passwd11, m_passwd12, m_passwd13, m_passwd14, m_passwd15;
    CString m_passwd16, m_passwd17, m_passwd18, m_passwd19, m_passwd20;
    CString m_passwd21, m_passwd22, m_passwd23, m_passwd24, m_passwd25;

    CString m_retail_name1,  m_retail_name2,  m_retail_name3,  m_retail_name4,  m_retail_name5;
    CString m_retail_name6,  m_retail_name7,  m_retail_name8,  m_retail_name9,  m_retail_name10;
    CString m_retail_name11, m_retail_name12, m_retail_name13, m_retail_name14, m_retail_name15;
    CString m_retail_name16, m_retail_name17, m_retail_name18, m_retail_name19, m_retail_name20;
    CString m_retail_name21, m_retail_name22, m_retail_name23, m_retail_name24, m_retail_name25;

    CString m_second_name1,  m_second_name2,  m_second_name3,  m_second_name4,  m_second_name5;
    CString m_second_name6,  m_second_name7,  m_second_name8,  m_second_name9,  m_second_name10;
    CString m_second_name11, m_second_name12, m_second_name13, m_second_name14, m_second_name15;
    CString m_second_name16, m_second_name17, m_second_name18, m_second_name19, m_second_name20;
    CString m_second_name21, m_second_name22, m_second_name23, m_second_name24, m_second_name25;

    // ---------- Controls ------------------------------------------------------
    CSkinnedEdit  m_editProd[kRowCount];
    CSkinnedEdit  m_editBiz[kRowCount];
    CSkinnedEdit  m_editPwd[kRowCount];
    CModernButton m_btnDownload[kRowCount];
    CSkinnedEdit  m_editMerchantName[kRowCount];
    CSkinnedEdit  m_editEtc[kRowCount];

    // ---------- Pointer arrays for loop access --------------------------------
    CString* m_pPrdid[kRowCount];
    CString* m_pRegno[kRowCount];
    CString* m_pPasswd[kRowCount];
    CString* m_pRetailName[kRowCount];
    CString* m_pSecondName[kRowCount];
    void InitPointerArrays();

    // ---------- Layout cache --------------------------------------------------
    CRect m_rcRow[kRowCount];   // row card rects (window coords, updated by Layout)
    CRect m_rcProd[kRowCount];
    CRect m_rcBiz[kRowCount];
    CRect m_rcPwd[kRowCount];
    CRect m_rcBtn[kRowCount];
    CRect m_rcInfoRep[kRowCount];
    CRect m_rcInfoTerm[kRowCount];

    // ---------- Scroll state --------------------------------------------------
    int  m_nScrollPos;
    int  m_nTotalContentH;
    int  m_nViewH;

    // SB_THUMBTRACK coalescing timer
    int      m_nPendingScrollPos;
    BOOL     m_bScrollTimerActive;
    static const UINT_PTR kScrollTimerId = 1001;
    static const UINT     kScrollTimerMs = 16;
};
