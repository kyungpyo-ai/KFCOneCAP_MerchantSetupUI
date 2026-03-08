#pragma once
#include "resource.h"
#include "ModernUI.h"

// ==============================================================
// [ShopDownDlg.h]
//  - 가맹점 다운로드(조회/다운로드/삭제) 화면을 담당
//  - 리스트/카드 UI를 ModernUI 스타일로 그리며, 스크롤/선택/버튼 이벤트 처리
// ==============================================================


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
    afx_msg void OnDestroy();
    afx_msg BOOL OnNcActivate(BOOL bActive);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg void OnTimer(UINT_PTR nIDEvent);  // [FIX] xxxSaveDlgFocus O(N^2) 차단
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    void CreateControlsOnce();
    void LayoutControls();
    void ApplyFonts();
    void OnDownloadClick(int slot, int rowIdx);
    void OnDeleteClick(int slot, int rowIdx);
    void RebindSlots();

    // ---------- Card/Control background sync ---------------------------------
     // 카드(행) 배경색이 상태에 따라 바뀌는 경우, 각 컨트롤의 UnderlayColor도
    // 즉시 동일한 색으로 맞춰 라운드 모서리 halo(흰색 테두리) / 배경 불일치를 제거한다.
    COLORREF GetRowCardBg(int slot) const;
    void ApplyRowUnderlay(int slot, BOOL bRedraw = TRUE);
    void ApplyAllRowUnderlays();


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
    
    static const int kRowH     = 190;  // data row height (approx) - show 2 cards in view
    static const int kRowGap   = 12;   // gap between cards
    static const int kCtrlH    = 42;   // control height inside row (match other tabs, taller)   // control height inside row (match other tabs, slightly taller)   // control height inside row (match other tabs)

    static const int kRowCount    = 20;
    static const int kRowsPerPage = 2;
    static const int kTotalPages  = kRowCount / kRowsPerPage;  // 10
    static const int kBtnBase  = 61001;
    static const int kDelBase  = 61101;

private:
    // ---------- Brushes -------------------------------------------------------
    CBrush m_brushBg;    // dialog background RGB(249,250,252)
    CBrush m_brushCard;  // row card / control background RGB(255,255,255)
    CBrush m_brushCardDisabled; // disabled edit background RGB(245,246,248)

    // ---------- Fonts ---------------------------------------------------------
    CFont m_fontHeader;   // column header labels
    CFont m_fontCell;     // edit / button text
    CFont m_fontBadge;    // row number badge

    // Cached GDI+ paint fonts (recreated on DPI change)
    int                  m_nCachedPaintDpi;
    Gdiplus::Font*       m_pFontLbl;
    Gdiplus::Font*       m_pFontVal;
    Gdiplus::Font*       m_pFontValBold;
    Gdiplus::FontFamily* m_pFontFamily;

    // ---------- Data strings --------------------------------------------------
    // ---------- Per-row data (all 20 rows) ------------------------------------
    // 2-row slot reuse: only kRowsPerPage rows visible at a time.
    // Data for all pages lives here; UI controls (edit/buttons) are reused
    // per slot when the user navigates pages.
    struct RowData {
        CString prdid, regno, passwd, retail_name, second_name;
    };
    RowData m_rowData[kRowCount];

    // ---------- Controls ------------------------------------------------------
    CSkinnedEdit  m_editProd[kRowsPerPage];
    CSkinnedEdit  m_editBiz[kRowsPerPage];
    CSkinnedEdit  m_editPwd[kRowsPerPage];
    CModernButton m_btnDownload[kRowsPerPage];
    CModernButton m_btnDelete[kRowsPerPage];


    // ---------- Layout cache --------------------------------------------------
    CRect m_rcRow[kRowsPerPage];   // row card rects (window coords, updated by Layout)
    CRect m_rcProd[kRowsPerPage];
    CRect m_rcBiz[kRowsPerPage];
    CRect m_rcPwd[kRowsPerPage];
    CRect m_rcBtn[kRowsPerPage];
    CRect m_rcDel[kRowsPerPage];
    CRect m_rcInfoRep[kRowsPerPage];
    CRect m_rcInfoTerm[kRowsPerPage];
    CRect m_rcInfoTermVal[kRowsPerPage];

    BOOL m_bInLayout;

    // ---------- Pagination ----------------------------------------------------
    int  m_nCurrentPage;                    // 0-based current page index
    CRect m_rcNavBar;   // nav bar rect for OnPaint page indicator

    // Custom nav buttons: drawn in OnPaint, hit-tested in OnLButtonDown
    CRect m_rcPrevBtn;
    CRect m_rcNextBtn;
    bool  m_bHoverPrev;
    bool  m_bHoverNext;
    bool  m_bPressedPrev;
    bool  m_bPressedNext;
    bool  m_bMouseTracked;

    // Page-transition slide animation
    int   m_nNavAnim;       // 0=idle, 1-10=animating (counts down)
    bool  m_bNavAnimNext;   // true=forward, false=backward
    int   m_nAnimFromPage;  // page index transitioning FROM
    float m_fPillFrom;      // visual pill position at animation start (handles rapid clicks)

    void UpdatePageButtons();
    void RefreshPage();  // lightweight page switch (no control reposition)
    void OnPrevPageClick();
    void OnNextPageClick();
};