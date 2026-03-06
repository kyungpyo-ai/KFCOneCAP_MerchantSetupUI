#include "stdafx.h"
#include "ShopDownDlg.h"
#include <string>

// ==============================================================
// [ShopDownDlg.cpp]
//  - 가맹점 다운로드 다이얼로그 구현부
//
// 화면 동작 개요
//  1) OnInitDialog()에서 컨트롤/버튼/리스트(또는 카드 영역) 초기화
//  2) 스크롤/마우스 입력에 따라 선택/호버 상태를 갱신하고 Invalidate()로 재그림
//  3) 다운로드/삭제 버튼 클릭 시 대상 단말/가맹점 정보를 기준으로 동작
//
// 유지보수 포인트
//  - 스크롤 영역 높이/행 높이/패딩 등은 한 곳(상수/헬퍼)에서 관리 권장
//  - WM_VSCROLL/마우스휠 처리 시 범위(clamp) 누락이 크래시/깜빡임 원인이 되기 쉬움
// ==============================================================


IMPLEMENT_DYNAMIC(CShopDownDlg, CDialog)

BEGIN_MESSAGE_MAP(CShopDownDlg, CDialog)
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_WM_NCACTIVATE()    // [FIX] xxxSaveDlgFocus O(N^2) 차단
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_WM_TIMER()
END_MESSAGE_MAP()

// ============================================================================
// ColLayout::Compute
// ============================================================================
void CShopDownDlg::CardLayout::Compute(int clientWidth, HWND hwnd)
{
        cardM    = ModernUIDpi::Scale(hwnd, 0); // host is already inset by parent card padding
    cardPad  = ModernUIDpi::Scale(hwnd, 16);
    colGap   = ModernUIDpi::Scale(hwnd, 16);
        idxW     = ModernUIDpi::Scale(hwnd, 34); // widened for 2-digit indices
editGap  = ModernUIDpi::Scale(hwnd, 8);
labelH   = ModernUIDpi::Scale(hwnd, 28);
    labelGap = ModernUIDpi::Scale(hwnd, 6);
    infoLineH= ModernUIDpi::Scale(hwnd, 22);

        btnW = ModernUIDpi::Scale(hwnd, 140); // ensure "다운로드" fits on one line
            btnH = ModernUIDpi::Scale(hwnd, 38);

    // item card available inner width
    // (section margin is handled by caller; here we just decide the split ratio)
    int innerW = clientWidth - cardM * 2 - cardPad * 2 - idxW;

    // left column: fixed-ish, but clamp for smaller windows
    int leftMin = ModernUIDpi::Scale(hwnd, 280);
    int leftMax = ModernUIDpi::Scale(hwnd, 380);
int idealLeft = (int)(innerW * 0.48);
    if (idealLeft < leftMin) idealLeft = leftMin;
    if (idealLeft > leftMax) idealLeft = leftMax;

    leftW  = idealLeft;
    rightW = innerW - leftW - colGap;

    // Ensure the right column is wide enough to keep action buttons readable.
    // (Download/Delete should not wrap in normal window widths.)
    const int btnPadR = ModernUIDpi::Scale(hwnd, 16);
    const int btnGap  = ModernUIDpi::Scale(hwnd, 8);
    const int rightMin = max(ModernUIDpi::Scale(hwnd, 220), (btnW * 2 + btnPadR + btnGap));

    if (rightW < rightMin)
    {
        // fallback: squeeze left a bit if right is too small
        int need = rightMin - rightW;
        leftW = max(leftMin, leftW - need);
        rightW = innerW - leftW - colGap;
    }
}


// ============================================================================
// Construction / Destruction
// ============================================================================
CShopDownDlg::CShopDownDlg(CWnd* pParent)
    : CDialog(CShopDownDlg::IDD, pParent)
    , m_nCurrentPage(0)
    , m_bInLayout(FALSE)
    , m_nCachedPaintDpi(0)
    , m_pFontLbl(nullptr)
    , m_pFontVal(nullptr)
    , m_pFontValBold(nullptr)
    , m_pFontFamily(nullptr)
    , m_bHoverPrev(false)
    , m_bHoverNext(false)
    , m_bPressedPrev(false)
    , m_bPressedNext(false)
    , m_bMouseTracked(false)
    , m_nNavAnim(0)
    , m_bNavAnimNext(false)
{

}

CShopDownDlg::~CShopDownDlg()
{
    // Font cleanup is handled in OnDestroy; pointers are null by the time the destructor runs.
    if (m_brushBg.GetSafeHandle())   m_brushBg.DeleteObject();
    if (m_brushCard.GetSafeHandle()) m_brushCard.DeleteObject();
}

void CShopDownDlg::OnDestroy()
{
    delete m_pFontLbl;     m_pFontLbl     = nullptr;
    delete m_pFontVal;     m_pFontVal     = nullptr;
    delete m_pFontValBold; m_pFontValBold = nullptr;
    delete m_pFontFamily;  m_pFontFamily  = nullptr;
    CDialog::OnDestroy();
}


// ============================================================================
// OnInitDialog
// ============================================================================
// --------------------------------------------------------------
// 다이얼로그 초기화
//  - 리스트 데이터/버튼/스킨 적용
// --------------------------------------------------------------
BOOL CShopDownDlg::OnInitDialog()
{
    /* [UI-STEP] 초기 UI 구성(다운로드 화면 컨트롤/레이아웃/스크롤 초기화)
     * 1) 베이스 다이얼로그 초기화 후, CreateControlsOnce()로 자식 컨트롤을 1회 생성한다.
     * 2) ApplyFonts()로 리스트/카드 텍스트 폰트를 적용한다.
     * 3) LayoutControls()로 카드/리스트 영역, 버튼 영역, 스크롤바 위치를 계산/배치한다.
     * 4) UpdateScrollBar()로 데이터 길이에 맞게 스크롤 범위/페이지 크기를 설정한다.
     * 5) 초기 표시 상태를 위해 Invalidate()로 첫 렌더링을 트리거한다.
     *
     * [참고]
     * - 컨트롤 생성(Create)과 배치(SetWindowPos)는 반드시 분리(리사이즈 시 재생성 금지).
     */

    CDialog::OnInitDialog();
    ModernUIGfx::EnsureGdiplusStartup();

    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    m_brushBg.CreateSolidBrush(KFTC_DLG_CONTENT_BG);
    m_brushCard.CreateSolidBrush(RGB(255, 255, 255));
    m_brushCardDisabled.CreateSolidBrush(KFTC_CARD_DISABLED_BG);

    CreateControlsOnce();


    ApplyFonts();
    LayoutControls();
    return TRUE;
}

// ============================================================================
// CreateControlsOnce
// ============================================================================
void CShopDownDlg::CreateControlsOnce()
{
    // Data-only init. Win32 controls created lazily via CreateRow(i) in LayoutControls.
    struct { LPCTSTR prod; LPCTSTR biz; LPCTSTR name; } data[25] = {
        {_T("K074404214"), _T("1050844729"), _T("금융결제원 테스트")},

    };

    for (int i = 0; i < kRowCount; ++i)
    {
        m_rowData[i].prdid       = (data[i].prod ? data[i].prod : _T(""));
        m_rowData[i].regno       = (data[i].biz  ? data[i].biz  : _T(""));
        m_rowData[i].retail_name = (data[i].name ? data[i].name : _T(""));
        m_rowData[i].second_name = _T("");
    }

    // Create kRowsPerPage slot controls (reused across page navigation)
    const DWORD edtBase  = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS
                         | ES_AUTOHSCROLL | ES_CENTER;
    const DWORD pwdStyle = edtBase | ES_PASSWORD;
    for (int slot = 0; slot < kRowsPerPage; ++slot)
    {
        m_editProd[slot].CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
            edtBase,  CRect(0,0,10,10), this, 62000+(slot*10)+1);
        m_editBiz[slot].CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
            edtBase,  CRect(0,0,10,10), this, 62000+(slot*10)+2);
        m_editPwd[slot].CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
            pwdStyle, CRect(0,0,10,10), this, 62000+(slot*10)+3);
        m_btnDownload[slot].Create(_T("다운로드"),
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS|BS_OWNERDRAW,
            CRect(0,0,10,10), this, kBtnBase+slot);
        m_btnDownload[slot].SetColors(KFTC_PRIMARY, KFTC_PRIMARY_HOVER, RGB(255,255,255));
        m_btnDelete[slot].Create(_T("삭제"),
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS|BS_OWNERDRAW,
            CRect(0,0,10,10), this, kDelBase+slot);
        m_btnDelete[slot].SetColors(KFTC_BTN_SECONDARY, KFTC_BTN_SECONDARY_HOV, RGB(40,40,40));
    }
}



// ============================================================================
// ApplyFonts
// ============================================================================
void CShopDownDlg::ApplyFonts()
{
    /* [UI-STEP] 텍스트 폰트 적용(가독성/행 높이 결정)
     * 1) DPI를 반영해 본문/캡션 폰트를 생성한다.
     * 2) 행/카드 높이 계산에 폰트 메트릭이 영향을 주므로, 폰트 적용 후 레이아웃을 다시 잡는다.
     */

    LOGFONT lf = {};
    ::GetObject((HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    lstrcpy(lf.lfFaceName, _T("Malgun Gothic"));

    // Field labels: match other tabs (13px)
    lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 13);
    lf.lfWeight = FW_NORMAL;
    m_fontHeader.DeleteObject();
    m_fontHeader.CreateFontIndirect(&lf);

    // Cell text (edit, button): match other tabs (13px)
    lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 13);
    lf.lfWeight = FW_NORMAL;
    m_fontCell.DeleteObject();
    m_fontCell.CreateFontIndirect(&lf);

    // Row number badge: semi-bold 13px
    lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 13);
    lf.lfWeight = FW_SEMIBOLD;
    m_fontBadge.DeleteObject();
    m_fontBadge.CreateFontIndirect(&lf);

    for (int slot = 0; slot < kRowsPerPage; ++slot)
    {
        m_editProd[slot].SetFont(&m_fontCell);
        m_editBiz[slot].SetFont(&m_fontCell);
        m_editPwd[slot].SetFont(&m_fontCell);
        m_btnDownload[slot].SetFont(&m_fontCell);
    }
}

// ============================================================================
// LayoutControls
// Moves all controls via DeferWindowPos with SWP_NOREDRAW, then
// redraws everything at once with RedrawWindow(RDW_ALLCHILDREN).
// ============================================================================
// ============================================================================
// LayoutControls
// Positions 2 cards for the current page, hides all others.
// Navigation bar (Prev/Next) is placed below the cards.
// ============================================================================
void CShopDownDlg::LayoutControls()
{
    if (m_bInLayout) return;
    m_bInLayout = TRUE;

    CRect rc;
    GetClientRect(&rc);

    const int rowGap = ModernUIDpi::Scale(m_hWnd, kRowGap);
    const int ctrlH  = ModernUIDpi::Scale(m_hWnd, kCtrlH);
    const int padY   = ModernUIDpi::Scale(m_hWnd, 8);
    const int padX   = ModernUIDpi::Scale(m_hWnd, 8);
    const int navH   = ModernUIDpi::Scale(m_hWnd, 44);

    const int navGap = ModernUIDpi::Scale(m_hWnd, 6);
    const int navY_bot = rc.Height() - padY - navH;  // nav bar pinned to bottom
    const int availH = max(0, navY_bot - padY - navGap);  // usable space above nav bar
    int rowH = (availH - (kRowsPerPage - 1) * rowGap) / kRowsPerPage;

    // Compute layout width and card column widths
    int layoutW = rc.Width() - padX * 2;
    if (layoutW < ModernUIDpi::Scale(m_hWnd, 320)) layoutW = ModernUIDpi::Scale(m_hWnd, 320);
    m_card.Compute(layoutW, m_hWnd);

    // Clamp row height to keep controls readable across DPI
    const int baseRowMin = ModernUIDpi::Scale(m_hWnd, 192);
    const int yInfo1Off        = m_card.labelH + m_card.labelGap;
    const int repValBottomOff  = yInfo1Off + m_card.infoLineH;
    const int yTermLabelOff    = repValBottomOff + m_card.editGap;
    const int yTermValOff      = yTermLabelOff + m_card.labelH + m_card.labelGap;
    const int termValBottomOff = yTermValOff + m_card.infoLineH;
    const int btnBottomOff     = termValBottomOff + m_card.editGap + ctrlH;
    const int rowMinNeeded = btnBottomOff + m_card.cardPad * 2;
    const int rowMin = max(baseRowMin, rowMinNeeded);
    const int rowMax = ModernUIDpi::Scale(m_hWnd, 320);
    if (rowH < rowMin) rowH = rowMin;
    if (rowH > rowMax) rowH = rowMax;
    // Hard cap: never let cards overflow into the nav bar area
    { const int rowFit = (availH - (kRowsPerPage - 1) * rowGap) / kRowsPerPage; if (rowFit > 0 && rowH > rowFit) rowH = rowFit; }

    const int pageStart = m_nCurrentPage * kRowsPerPage;

    // prod/biz/pwd/download/delete (5 per row) + 2 nav buttons
    HDWP hdwp = ::BeginDeferWindowPos(kRowsPerPage * 5);

    for (int slot = 0; slot < kRowsPerPage; ++slot)
    {

        // On-page rows get real coords; off-page rows go off-screen
        const int rowIdx = pageStart + slot;
        const bool bValid = (rowIdx < kRowCount);
        const int y = padY + slot * (rowH + rowGap);

        m_rcRow[slot].SetRect(padX, y, rc.right - padX, y + rowH);

        const int cardPad = m_card.cardPad;
        CRect inner = m_rcRow[slot];
        inner.DeflateRect(cardPad, cardPad);

        // 2-column split
        const int xL = inner.left + m_card.idxW;
        const int xR = xL + m_card.leftW + m_card.colGap;

        const int labelH = m_card.labelH;
        const int labelGap = m_card.labelGap;
        const int editGap = m_card.editGap;

        // row1 (prod/biz)
        const int y1Edit = inner.top + labelH + labelGap;
        const int halfW  = (m_card.leftW - editGap) / 2;

        m_rcProd[slot].SetRect(xL,                 y1Edit, xL + halfW,         y1Edit + ctrlH);
        m_rcBiz[slot].SetRect (xL + halfW+editGap, y1Edit, xL + m_card.leftW,  y1Edit + ctrlH);

        // row2 (pwd)
        const int y2Label = y1Edit + ctrlH + editGap;
        const int y2Edit  = y2Label + labelH + labelGap;
        m_rcPwd[slot].SetRect(xL, y2Edit, xL + m_card.leftW, y2Edit + ctrlH);

        int termRight = xR + m_card.rightW;

        // buttons row: Download/Delete
        {
            const int btnPadR = ModernUIDpi::Scale(m_hWnd, 16);
            const int btnGap  = ModernUIDpi::Scale(m_hWnd, 8);
            const int btnH    = ctrlH;

            int btnW = m_card.btnW;
            const int maxFit = (m_card.rightW - btnPadR - btnGap) / 2;
            if (maxFit > 0)
                btnW = min(btnW, maxFit);
            else
                btnW = ModernUIDpi::Scale(m_hWnd, 40);

            const int yInfo1        = inner.top + labelH + labelGap;
            const int repValBottom  = yInfo1 + m_card.infoLineH;
            const int yTermLabel    = repValBottom + editGap;
            const int yTermVal      = yTermLabel + labelH + labelGap;
            const int termValBottom = yTermVal + m_card.infoLineH;

            const int btnY  = termValBottom + editGap;
            const int right = xR + m_card.rightW - btnPadR;

            m_rcDel[slot].SetRect(right - btnW, btnY, right, btnY + btnH);
            m_rcBtn[slot].SetRect(m_rcDel[slot].left - btnGap - btnW, btnY, m_rcDel[slot].left - btnGap, btnY + btnH);

            termRight = xR + m_card.rightW;
        }

        // info text rectangles
        const int yInfo1 = inner.top + labelH + labelGap;
        m_rcInfoRep[slot].SetRect(xR, yInfo1, xR + m_card.rightW, yInfo1 + m_card.infoLineH);

        const int repValBottom = yInfo1 + m_card.infoLineH;
        const int yTermLabel   = repValBottom + editGap;
        m_rcInfoTerm[slot].SetRect(xR, yTermLabel, termRight, yTermLabel + m_card.labelH);

        const int yTermVal = yTermLabel + m_card.labelH + labelGap;
        m_rcInfoTermVal[slot].SetRect(xR, yTermVal, termRight, yTermVal + m_card.infoLineH);


        // Enabled/disabled visual states
        {
            ApplyRowUnderlay(slot, FALSE);
            const BOOL enBtn = bValid && ::IsWindowEnabled(m_btnDownload[slot].m_hWnd);
            if (enBtn)
                m_btnDownload[slot].SetColors(KFTC_PRIMARY, KFTC_PRIMARY_HOVER, RGB(255,255,255));
            else
                m_btnDownload[slot].SetColors(KFTC_BTN_DISABLED_BG, KFTC_BTN_DISABLED_BG, RGB(160,165,175));

            const BOOL enDel = bValid && ::IsWindowEnabled(m_btnDelete[slot].m_hWnd);
            if (enDel)
                m_btnDelete[slot].SetColors(KFTC_BTN_SECONDARY, KFTC_BTN_SECONDARY_HOV, RGB(40,40,40));
            else
                m_btnDelete[slot].SetColors(KFTC_CARD_DISABLED_BG, KFTC_CARD_DISABLED_BG, RGB(160,160,160));
        }

        const UINT baseFlags = SWP_NOZORDER | SWP_NOACTIVATE;
        const UINT showHide  = bValid ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;

        if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_editProd[slot].m_hWnd,    NULL, m_rcProd[slot].left, m_rcProd[slot].top, m_rcProd[slot].Width(), m_rcProd[slot].Height(), baseFlags | showHide);
        if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_editBiz[slot].m_hWnd,     NULL, m_rcBiz[slot].left,  m_rcBiz[slot].top,  m_rcBiz[slot].Width(),  m_rcBiz[slot].Height(),  baseFlags | showHide);
        if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_editPwd[slot].m_hWnd,     NULL, m_rcPwd[slot].left,  m_rcPwd[slot].top,  m_rcPwd[slot].Width(),  m_rcPwd[slot].Height(),  baseFlags | showHide);
        if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_btnDownload[slot].m_hWnd, NULL, m_rcBtn[slot].left,  m_rcBtn[slot].top,  m_rcBtn[slot].Width(),  m_rcBtn[slot].Height(),  baseFlags | showHide);
        if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_btnDelete[slot].m_hWnd,   NULL, m_rcDel[slot].left,  m_rcDel[slot].top,  m_rcDel[slot].Width(),  m_rcDel[slot].Height(),  baseFlags | showHide);
    }

    // Navigation bar: pinned to dialog bottom (rowFit hard cap above prevents card overlap)
    const int navY = navY_bot;
    m_rcNavBar.SetRect(padX, navY, rc.right - padX, navY + navH);

    // Custom nav buttons: calculate rects for OnPaint drawing and hit-test
    const int navBtnSz = ModernUIDpi::Scale(m_hWnd, 28);
    const int midX     = rc.Width() / 2;
    const int half     = ModernUIDpi::Scale(m_hWnd, 60);
    const int navBtnY  = navY + (navH - navBtnSz) / 2 + ModernUIDpi::Scale(m_hWnd, 4);
    m_rcPrevBtn.SetRect(midX - half - navBtnSz, navBtnY, midX - half, navBtnY + navBtnSz);
    m_rcNextBtn.SetRect(midX + half, navBtnY, midX + half + navBtnSz, navBtnY + navBtnSz);

    if (hdwp) ::EndDeferWindowPos(hdwp);

    RebindSlots();
    UpdatePageButtons();
    ApplyAllRowUnderlays();
    ::RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
    m_bInLayout = FALSE;
}


// ============================================================================
// OnMouseMove / OnMouseLeave  (hover row highlight)
// ============================================================================


// ============================================================================
// Commands / events
// ============================================================================
BOOL CShopDownDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    /* [UI-STEP] 컨트롤 이벤트 라우팅(버튼 클릭 등)
     * 1) 버튼 ID를 확인해 다운로드/삭제 등 클릭 이벤트를 분기한다.
     * 2) 필요한 경우 UI 상태를 갱신하고 Invalidate()로 재그림한다.
     */

    UINT nID   = LOWORD(wParam);
    UINT nCode = HIWORD(wParam);
    if (nCode == BN_CLICKED)
    {
        if (nID >= kBtnBase && nID < kBtnBase + kRowsPerPage)
        {
            int slot   = (int)(nID - kBtnBase);
            int rowIdx = m_nCurrentPage * kRowsPerPage + slot;
            OnDownloadClick(slot, rowIdx);
            return TRUE;
        }
        if (nID >= kDelBase && nID < kDelBase + kRowsPerPage)
        {
            int slot   = (int)(nID - kDelBase);
            int rowIdx = m_nCurrentPage * kRowsPerPage + slot;
            OnDeleteClick(slot, rowIdx);
            return TRUE;
        }
    }
    return CDialog::OnCommand(wParam, lParam);
}

void CShopDownDlg::OnDownloadClick(int slot, int rowIdx)
{
    /* [UI-STEP] 다운로드 버튼 처리(UI 연동)
     * 1) 선택된 단말/가맹점 정보를 확인한다.
     * 2) 다운로드 요청을 수행한 뒤 결과를 리스트/카드에 반영한다.
     * 3) 반영 후 Invalidate()로 화면 갱신한다.
     */

    if (slot < 0 || slot >= kRowsPerPage) return;

    // 테스트용: 다운로드 시 대표가맹점명(RetailName)에 "TEST"를 채워 카드 상태(배경)를 즉시 전환
    m_rowData[rowIdx].retail_name = _T("TEST");

    // 필요 시 단말기별 가맹점도 동일하게 표시(빈칸이면 보기 어색해서 같이 채움)
    if (m_rowData[rowIdx].second_name.IsEmpty())
        m_rowData[rowIdx].second_name = _T("TEST");

    // 화면/컨트롤 underlay 즉시 동기화
    // 다운로드되면 삭제 버튼 활성화
    if (m_btnDelete[slot].GetSafeHwnd()) m_btnDelete[slot].EnableWindow(TRUE);

    ApplyRowUnderlay(slot, FALSE);

    RedrawWindow(&m_rcRow[slot], nullptr,
        RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void CShopDownDlg::OnDeleteClick(int slot, int rowIdx)
{
    /* [UI-STEP] 삭제 버튼 처리(UI 연동)
     * 1) 선택 항목을 확인하고 삭제 동작을 수행한다.
     * 2) 삭제 후 스크롤 범위가 바뀌면 UpdateScrollBar()를 호출한다.
     * 3) Invalidate()로 화면 갱신한다.
     */

    if (slot < 0 || slot >= kRowsPerPage) return;

    // 해당 카드(행)의 모든 데이터 초기화
    m_rowData[rowIdx].retail_name = _T("");
    m_rowData[rowIdx].second_name = _T("");
    m_rowData[rowIdx].prdid       = _T("");
    m_rowData[rowIdx].regno       = _T("");
    m_rowData[rowIdx].passwd      = _T("");

    if (m_editProd[slot].GetSafeHwnd()) m_editProd[slot].SetWindowText(_T(""));
    if (m_editBiz[slot].GetSafeHwnd())  m_editBiz[slot].SetWindowText(_T(""));
    if (m_editPwd[slot].GetSafeHwnd())  m_editPwd[slot].SetWindowText(_T(""));

    // 삭제되면 삭제 버튼 비활성화
    if (m_btnDelete[slot].GetSafeHwnd()) m_btnDelete[slot].EnableWindow(FALSE);

    // 화면/컨트롤 underlay 즉시 동기화 (빈 상태 → 연한 톤)
    ApplyRowUnderlay(slot, FALSE);

    RedrawWindow(&m_rcRow[slot], nullptr,
        RDW_INVALIDATE | RDW_ALLCHILDREN);
}


/*
[배경 지우기]
- 깜빡임을 줄이기 위해 보통 TRUE를 반환하거나, 자체 더블버퍼와 함께 사용합니다.
- 배경을 직접 그리는 구조라면 기본 처리(DefWindowProc)를 피하는 편이 좋습니다.
*/
BOOL CShopDownDlg::OnEraseBkgnd(CDC*) {
    /* [UI-STEP] 배경 지우기(깜빡임 감소)
     * 1) OnPaint에서 배경을 전부 그리는 구조면 TRUE를 리턴하여 기본 지우기를 막는다.
     * 2) 기본 지우기를 막을 때는 항상 전체 영역을 OnPaint에서 칠해야 잔상이 남지 않는다.
     */
 return TRUE; }

HBRUSH CShopDownDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_DLG)
    {
        pDC->SetBkMode(TRANSPARENT);
        return (HBRUSH)m_brushBg.GetSafeHandle();
    }
    if (nCtlColor == CTLCOLOR_EDIT)
    {
        const BOOL bEnabled = pWnd && ::IsWindowEnabled(pWnd->GetSafeHwnd());
        pDC->SetTextColor(bEnabled ? RGB(18, 24, 40) : RGB(120, 125, 135));
        pDC->SetBkColor(bEnabled ? RGB(255, 255, 255) : RGB(245, 246, 248));
        return (HBRUSH)(bEnabled ? m_brushCard.GetSafeHandle() : m_brushCardDisabled.GetSafeHandle());
    }
    return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}


/*
[리사이즈 처리]
- 창 크기 변경 시 호출됩니다.
- 여기서는 ApplyLayout()로 재배치하고, 필요 시 Invalidate()로 재그림을 트리거합니다.
*/
void CShopDownDlg::OnSize(UINT nType, int cx, int cy)
{
    /* [UI-STEP] 창 크기 변경 처리(레이아웃 재배치 + 스크롤 갱신)
     * 1) 최소 크기보다 작아지면 컨트롤이 겹치지 않게 마진/폭을 보정한다.
     * 2) LayoutControls() 호출로 전체 재배치한다.
     * 3) UpdateScrollBar()로 페이지 크기/범위를 다시 계산한다.
     * 4) Invalidate()로 화면 갱신한다.
     */

    CDialog::OnSize(nType, cx, cy);
    if (cx <= 0 || cy <= 0) return;
    if (m_editProd[0].GetSafeHwnd()) LayoutControls();
}

// ============================================================================
// OnPaint  - full double-buffered render
// ============================================================================
// --------------------------------------------------------------
// 커스텀 페인팅
//  - 배경/카드/행(단말기/가맹점) 렌더링
// --------------------------------------------------------------
void CShopDownDlg::OnPaint()
{
    /* [UI-STEP] 커스텀 렌더링(카드/행/호버/선택/언더레이)
     * 1) CPaintDC로 DC를 얻고 필요 시 메모리 DC를 사용해 깜빡임을 줄인다.
     * 2) 배경을 먼저 칠하고(전체 배경), 카드 영역을 순회하며 각 행/카드를 그린다.
     * 3) 선택/호버 상태에 따라 카드 배경(GetRowCardBg)과 보더를 바꿔 그린다.
     * 4) 텍스트/아이콘(있다면)을 마지막에 그려 시각적 우선순위를 맞춘다.
     *
     * [참고]
     * - 스크롤 오프셋이 적용되는 화면이므로, '그릴 때 y - scrollY' 형태로 좌표를 변환한다.
     */

    CPaintDC dc(this);

    CRect rc;
    GetClientRect(&rc);

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap memBmp;
    memBmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&memBmp);

    memDC.FillSolidRect(&rc, KFTC_DLG_CONTENT_BG);

    Gdiplus::Graphics g(memDC.m_hDC);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);


    // ----------------------------------------------------------------
    // Section header/card background is drawn by parent dialog (ShopSetupDlg)
    // Here we only draw item cards and labels.
    // ----------------------------------------------------------------

    const float clipTop    = 0.f;
    const float clipBottom = (float)rc.Height();

    // ----------------------------------------------------------------
    // Text helpers
    // ----------------------------------------------------------------
    const int curDpi = (int)ModernUIDpi::GetDpiForHwnd(m_hWnd);
    if (m_nCachedPaintDpi != curDpi || !m_pFontFamily)
    {
        delete m_pFontLbl;     m_pFontLbl     = nullptr;
        delete m_pFontVal;     m_pFontVal     = nullptr;
        delete m_pFontValBold; m_pFontValBold = nullptr;
        delete m_pFontFamily;  m_pFontFamily  = nullptr;
        const float lblPx = (float)ModernUIDpi::Scale(m_hWnd, 12);
        const float valPx = (float)ModernUIDpi::Scale(m_hWnd, 13);
        m_pFontFamily  = new Gdiplus::FontFamily(L"Malgun Gothic");
        m_pFontLbl     = new Gdiplus::Font(m_pFontFamily, lblPx, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        m_pFontVal     = new Gdiplus::Font(m_pFontFamily, valPx, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        m_pFontValBold = new Gdiplus::Font(m_pFontFamily, valPx, Gdiplus::FontStyleBold,    Gdiplus::UnitPixel);
        m_nCachedPaintDpi = curDpi;
    }

    // StringFormat is always the same - create once, share across all DrawEllips calls
    Gdiplus::StringFormat sfEllips;
    sfEllips.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
    sfEllips.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
    sfEllips.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    sfEllips.SetAlignment(Gdiplus::StringAlignmentNear);

    auto DrawEllips = [&](const CString& s, const Gdiplus::RectF& r, Gdiplus::Font* f, const Gdiplus::Color& c)
    {
        if (s.IsEmpty()) return;
        Gdiplus::SolidBrush br(c);
#ifdef UNICODE
        g.DrawString(s, -1, f, r, &sfEllips, &br);
#else
        CStringW ws(s);
        g.DrawString(ws, -1, f, r, &sfEllips, &br);
#endif
    };

    const Gdiplus::Color cLbl(255, 120, 128, 142);
    const Gdiplus::Color cVal(255, 24, 28, 35);
    const Gdiplus::Color cPh (255, 160, 165, 175);

    // ----------------------------------------------------------------
    // 3) Item cards + labels + right-side info (drawn)
    // ----------------------------------------------------------------
    const int pageStart = m_nCurrentPage * kRowsPerPage;
    for (int slot = 0; slot < kRowsPerPage; ++slot)
    {
        const int rowIdx = pageStart + slot;
        const CRect& r = m_rcRow[slot];

        // Data-driven card tone:
        // - 대표 가맹점(다운로드 결과)이 있으면 정상 카드(white)
        // - 없으면(미다운로드/조회 실패/미존재 등) 살짝 톤을 주어 구분
        CString rep  = m_rowData[rowIdx].retail_name;
        const bool hasRep = !rep.IsEmpty();

        // item card
        Gdiplus::RectF rf((float)r.left, (float)r.top, (float)r.Width(), (float)r.Height());
        Gdiplus::GraphicsPath rp;
        ModernUIGfx::AddRoundRect(rp, rf, 12.f);

        // Normal: white, Pending/Empty: very light blue tint (quiet)
        Gdiplus::SolidBrush brCard(hasRep
            ? Gdiplus::Color(255, 255, 255, 255)
            : Gdiplus::Color(255, 246, 248, 251));
        g.FillPath(&brCard, &rp);
        // Border removed for a cleaner modern look (background-only separation)

        // left labels
        {
            // prod / biz labels (same row)
            const float yLbl = (float)r.top + (float)m_card.cardPad;
            const float hLbl = (float)m_card.labelH;

            Gdiplus::RectF lp((float)m_rcProd[slot].left, yLbl, (float)m_rcProd[slot].Width(), hLbl);
            Gdiplus::RectF lb((float)m_rcBiz[slot].left,  yLbl, (float)m_rcBiz[slot].Width(),  hLbl);
            DrawEllips(_T("단말기 제품번호"), lp, m_pFontLbl, cLbl);
            DrawEllips(_T("사업자번호"),     lb, m_pFontLbl, cLbl);

            // pwd label
            const float yPwdLbl = (float)m_rcPwd[slot].top - (float)m_card.labelGap - (float)m_card.labelH;
            Gdiplus::RectF lpwd((float)m_rcPwd[slot].left, yPwdLbl, (float)m_rcPwd[slot].Width(), hLbl);
            DrawEllips(_T("비밀번호"), lpwd, m_pFontLbl, cLbl);
        }

        // right labels + values
        {
            const float yLbl = (float)r.top + (float)m_card.cardPad;
            const float hLbl = (float)m_card.labelH;

            // label rects align with value rects
            Gdiplus::RectF l1((float)m_rcInfoRep[slot].left,  yLbl, (float)m_rcInfoRep[slot].Width(), hLbl);
            DrawEllips(_T("대표 가맹점"), l1, m_pFontLbl, cLbl);

            // '단말기별 가맹점'은 2줄(라벨/값) 유지:
            //  - 라벨: 비밀번호 라벨 줄(y2Label)
            //  - 값  : 비밀번호 Edit 줄(y2Edit), 버튼과 동일한 Y (좌측 공간에 표시)
            Gdiplus::RectF l2((float)m_rcInfoTerm[slot].left,    (float)m_rcInfoTerm[slot].top,    (float)m_rcInfoTerm[slot].Width(),    (float)m_rcInfoTerm[slot].Height());
            DrawEllips(_T("단말기별 가맹점"), l2, m_pFontLbl, cLbl);

            CString rep  = m_rowData[rowIdx].retail_name;
            CString term = m_rowData[rowIdx].second_name;

            // value rects
            Gdiplus::RectF v1((float)m_rcInfoRep[slot].left,      (float)m_rcInfoRep[slot].top,      (float)m_rcInfoRep[slot].Width(),      (float)m_rcInfoRep[slot].Height());
            Gdiplus::RectF v2((float)m_rcInfoTermVal[slot].left,  (float)m_rcInfoTermVal[slot].top,  (float)m_rcInfoTermVal[slot].Width(),  (float)m_rcInfoTermVal[slot].Height());

            // Values: make it obvious these are filled by '다운로드'
            if (rep.IsEmpty())
                DrawEllips(_T("다운로드 후 표시"), v1, m_pFontVal, cPh);
            else
                DrawEllips(rep, v1, m_pFontValBold, cVal);

            if (term.IsEmpty())
            {
                // 단말기별 가맹점은 다운로드 후에도 값이 비어 있을 수 있음.
                // 대표 가맹점 값이 존재하면 '없음' 표시로 '-'를 보여준다.
                if (!rep.IsEmpty())
                    DrawEllips(_T("-"), v2, m_pFontValBold, cVal);
                else
                    DrawEllips(_T("다운로드 후 표시"), v2, m_pFontVal, cPh);
            }
            else
            {
                DrawEllips(term, v2, m_pFontValBold, cVal);
            }
}

        // row number (fixed gutter, more legible, no overlap)
        {
            CString n; n.Format(_T("%d"), rowIdx + 1);
            const float x = (float)r.left + 8.f;
            const float y = (float)r.top + (float)m_card.cardPad - 1.f;
            const float w = (float)m_card.idxW - 12.f;
            Gdiplus::RectF nr(x, y, w, (float)m_card.labelH + 4.f);
            Gdiplus::SolidBrush br(Gdiplus::Color(255, 92, 102, 120));
            Gdiplus::StringFormat sf;
            sf.SetTrimming(Gdiplus::StringTrimmingNone);
            sf.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
            sf.SetAlignment(Gdiplus::StringAlignmentFar);
            sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
#ifdef UNICODE
            g.DrawString(n, -1, m_pFontValBold, nr, &sf, &br);
#else
            CStringW wn(n);
            g.DrawString(wn, -1, m_pFontValBold, nr, &sf, &br);
#endif
        }
    }

    // Draw modern circular prev/next nav buttons + page indicator
    if (!m_rcNavBar.IsRectEmpty() && !m_rcPrevBtn.IsRectEmpty())
    {
        const bool bCanPrev = (m_nCurrentPage > 0);
        const bool bCanNext = (m_nCurrentPage < kTotalPages - 1);

        // bPressed: draw slightly shrunk circle + darker fill for tactile feedback
        auto DrawNavCircleBtn = [&](const CRect& btnRc, bool bEnabled, bool bHover, bool bPressed, bool bLeft)
        {
            Gdiplus::RectF rf((float)btnRc.left, (float)btnRc.top,
                              (float)btnRc.Width(), (float)btnRc.Height());

            // Pressed: shrink circle 10% for visual feedback
            if (bPressed && bEnabled)
            {
                float shrink = rf.Width * 0.10f;
                rf.X      += shrink / 2.f;
                rf.Y      += shrink / 2.f;
                rf.Width  -= shrink;
                rf.Height -= shrink;
            }

            // Background circle  (BLUE_500 / BLUE_400 / BLUE_600 palette)
            Gdiplus::Color fillClr;
            if (!bEnabled)
                fillClr = Gdiplus::Color(255, 235, 237, 240);   // KFTC_BTN_DISABLED_BG
            else if (bPressed)
                fillClr = Gdiplus::Color(255,   0,  76, 168);   // BLUE_600
            else if (bHover)
                fillClr = Gdiplus::Color(255,  15, 124, 255);   // BLUE_400 (KFTC_PRIMARY_HOVER)
            else
                fillClr = Gdiplus::Color(255,   0, 100, 221);   // BLUE_500 (KFTC_PRIMARY)

            Gdiplus::SolidBrush brFill(fillClr);
            g.FillEllipse(&brFill, rf);

            // Chevron arrow
            const float cx = rf.X + rf.Width / 2.f;
            const float cy = rf.Y + rf.Height / 2.f;
            const float aw = rf.Width  * 0.17f;
            const float ah = rf.Height * 0.25f;

            Gdiplus::Color arrowClr = bEnabled
                ? Gdiplus::Color(255, 255, 255, 255)
                : Gdiplus::Color(255, 148, 163, 184);   // GRAY_400 equivalent

            Gdiplus::Pen pen(arrowClr, rf.Width * 0.085f);
            pen.SetStartCap(Gdiplus::LineCapRound);
            pen.SetEndCap(Gdiplus::LineCapRound);
            pen.SetLineJoin(Gdiplus::LineJoinRound);

            if (bLeft) {
                Gdiplus::PointF pts[3] = {
                    { cx + aw, cy - ah },
                    { cx - aw, cy      },
                    { cx + aw, cy + ah }
                };
                g.DrawLines(&pen, pts, 3);
            } else {
                Gdiplus::PointF pts[3] = {
                    { cx - aw, cy - ah },
                    { cx + aw, cy      },
                    { cx - aw, cy + ah }
                };
                g.DrawLines(&pen, pts, 3);
            }
        };

        DrawNavCircleBtn(m_rcPrevBtn, bCanPrev, m_bHoverPrev, m_bPressedPrev, true);
        DrawNavCircleBtn(m_rcNextBtn, bCanNext, m_bHoverNext, m_bPressedNext, false);

        // Page indicator: slides in from direction on page change
        {
            CString pageStr;
            pageStr.Format(_T("%d / %d"), m_nCurrentPage + 1, kTotalPages);
            const float navCX = (float)(m_rcPrevBtn.right + m_rcNextBtn.left) / 2.f;
            const float navCY = (float)(m_rcPrevBtn.top + m_rcPrevBtn.bottom) / 2.f;
            const float pw = (float)ModernUIDpi::Scale(m_hWnd, 80);
            const float ph = (float)ModernUIDpi::Scale(m_hWnd, 28);
            float yOff = 0.f;
            Gdiplus::Color textClr(255, 60, 70, 90);
            if (m_nNavAnim > 0)
            {
                const int dir = m_bNavAnimNext ? 1 : -1;
                const float step = (float)ModernUIDpi::Scale(m_hWnd, 9);
                yOff = (float)(dir * m_nNavAnim) * step;
                BYTE a = (BYTE)(85 + 56 * (3 - m_nNavAnim));
                textClr = Gdiplus::Color(a, 0, 100, 221);
            }
            Gdiplus::RectF navRF(navCX - pw/2.f, navCY - ph/2.f + yOff, pw, ph);
            Gdiplus::StringFormat sfNav;
            sfNav.SetAlignment(Gdiplus::StringAlignmentCenter);
            sfNav.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            Gdiplus::RectF clipRF((float)m_rcNavBar.left, (float)m_rcNavBar.top,
                                  (float)m_rcNavBar.Width(), (float)m_rcNavBar.Height());
            g.SetClip(clipRF);
            Gdiplus::SolidBrush brNav(textClr);
#ifdef UNICODE
            g.DrawString(pageStr, -1, m_pFontValBold, navRF, &sfNav, &brNav);
#else
            CStringW wPage(pageStr);
            g.DrawString(wPage, -1, m_pFontValBold, navRF, &sfNav, &brNav);
#endif
            g.ResetClip();
        }
    }

    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

// ============================================================================
// RebindSlots - load current page's data into the 2 slot controls
// ============================================================================
void CShopDownDlg::RebindSlots()
{
    const int pageStart = m_nCurrentPage * kRowsPerPage;
    for (int slot = 0; slot < kRowsPerPage; ++slot)
    {
        const int rowIdx = pageStart + slot;
        const bool bValid = (rowIdx < kRowCount);

        if (!bValid)
        {
            if (m_editProd[slot].GetSafeHwnd()) m_editProd[slot].SetWindowText(_T(""));
            if (m_editBiz[slot].GetSafeHwnd())  m_editBiz[slot].SetWindowText(_T(""));
            if (m_editPwd[slot].GetSafeHwnd())  m_editPwd[slot].SetWindowText(_T(""));
            if (m_btnDownload[slot].GetSafeHwnd()) m_btnDownload[slot].EnableWindow(FALSE);
            if (m_btnDelete[slot].GetSafeHwnd())   m_btnDelete[slot].EnableWindow(FALSE);
            continue;
        }

        if (m_editProd[slot].GetSafeHwnd()) m_editProd[slot].SetWindowText(m_rowData[rowIdx].prdid);
        if (m_editBiz[slot].GetSafeHwnd())  m_editBiz[slot].SetWindowText(m_rowData[rowIdx].regno);
        if (m_editPwd[slot].GetSafeHwnd())  m_editPwd[slot].SetWindowText(m_rowData[rowIdx].passwd);

        const bool hasRep = !m_rowData[rowIdx].retail_name.IsEmpty();
        if (m_btnDownload[slot].GetSafeHwnd()) m_btnDownload[slot].EnableWindow(TRUE);
        if (m_btnDelete[slot].GetSafeHwnd())   m_btnDelete[slot].EnableWindow(hasRep ? TRUE : FALSE);
    }
}
// ============================================================================
// Card/Control background sync (UnderlayColor)
// ============================================================================
COLORREF CShopDownDlg::GetRowCardBg(int slot) const
{
    // OnPaint()와 동일한 규칙을 사용해야 화면과 컨트롤 underlay가 일치한다.
    // Normal: white, Pending/Empty: very light blue tint (quiet)
    const int rowIdx = m_nCurrentPage * kRowsPerPage + slot;
    if (slot < 0 || slot >= kRowsPerPage || rowIdx >= kRowCount) return RGB(255, 255, 255);

    const bool hasRep = !m_rowData[rowIdx].retail_name.IsEmpty();
    return hasRep ? RGB(255, 255, 255) : RGB(246, 248, 251);
}

void CShopDownDlg::ApplyRowUnderlay(int slot, BOOL bRedraw /*= TRUE*/)
{
    if (slot < 0 || slot >= kRowsPerPage) return;

    const COLORREF cr = GetRowCardBg(slot);

    if (m_editProd[slot].GetSafeHwnd()) m_editProd[slot].SetUnderlayColor(cr);
    if (m_editBiz[slot].GetSafeHwnd())  m_editBiz[slot].SetUnderlayColor(cr);
    if (m_editPwd[slot].GetSafeHwnd())  m_editPwd[slot].SetUnderlayColor(cr);

    if (m_btnDownload[slot].GetSafeHwnd()) m_btnDownload[slot].SetUnderlayColor(cr);

    
    if (m_btnDelete[slot].GetSafeHwnd())  m_btnDelete[slot].SetUnderlayColor(cr);
    if (bRedraw)
    {
        if (m_editProd[slot].GetSafeHwnd())    m_editProd[slot].Invalidate(FALSE);
        if (m_editBiz[slot].GetSafeHwnd())     m_editBiz[slot].Invalidate(FALSE);
        if (m_editPwd[slot].GetSafeHwnd())     m_editPwd[slot].Invalidate(FALSE);
        if (m_btnDownload[slot].GetSafeHwnd()) m_btnDownload[slot].Invalidate(FALSE);
        if (m_btnDelete[slot].GetSafeHwnd())   m_btnDelete[slot].Invalidate(FALSE);
    }
}

void CShopDownDlg::ApplyAllRowUnderlays()
{
    for (int slot = 0; slot < kRowsPerPage; ++slot)
        ApplyRowUnderlay(slot, FALSE);
}

BOOL CShopDownDlg::OnNcActivate(BOOL bActive)
{
    // CShopDownDlg 는 버튼이 50개(다운로드 25 + 삭제 25) 있다.
    // DefDlgProcA(WM_NCACTIVATE) -> xxxSaveDlgFocus -> 버튼 50개에
    // BM_SETSTYLE SendMessage -> O(N^2) 동기 연쇄 -> "응답없음".
    // DefWindowProc / DefDlgProc 를 일절 호출하지 않고 TRUE 만 반환해 차단.
    UNREFERENCED_PARAMETER(bActive);
    return TRUE;
}

// ============================================================================
// Pagination
// ============================================================================
void CShopDownDlg::RefreshPage()
{
    RebindSlots();
    UpdatePageButtons();
    ApplyAllRowUnderlays();
    ::RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void CShopDownDlg::OnPrevPageClick()
{
    if (m_nCurrentPage > 0)
    {
        m_nCurrentPage--;
        m_bNavAnimNext = false; m_nNavAnim = 3;
        KillTimer(42); SetTimer(42, 50, NULL);
        RefreshPage();
    }
}

void CShopDownDlg::OnNextPageClick()
{
    if (m_nCurrentPage < kTotalPages - 1)
    {
        m_nCurrentPage++;
        m_bNavAnimNext = true; m_nNavAnim = 3;
        KillTimer(42); SetTimer(42, 50, NULL);
        RefreshPage();
    }
}

// ============================================================================
// OnLButtonDown - hit-test custom nav buttons
// ============================================================================
void CShopDownDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
    bool bHitPrev = (!m_rcPrevBtn.IsRectEmpty() && m_rcPrevBtn.PtInRect(point));
    bool bHitNext = (!m_rcNextBtn.IsRectEmpty() && m_rcNextBtn.PtInRect(point));
    if (bHitPrev || bHitNext)
    {
        m_bPressedPrev = bHitPrev;
        m_bPressedNext = bHitNext;
        SetCapture();
        Invalidate(FALSE);
    }
    else
    {
        CDialog::OnLButtonDown(nFlags, point);
    }
}

// ============================================================================
// OnMouseMove / OnMouseLeave - hover state for custom nav buttons
// ============================================================================
void CShopDownDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    if (!m_bMouseTracked)
    {
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, m_hWnd, 0 };
        ::TrackMouseEvent(&tme);
        m_bMouseTracked = true;
    }
    bool hp = (!m_rcPrevBtn.IsRectEmpty() && m_rcPrevBtn.PtInRect(point)) ? true : false;
    bool hn = (!m_rcNextBtn.IsRectEmpty() && m_rcNextBtn.PtInRect(point)) ? true : false;
    if (hp != m_bHoverPrev || hn != m_bHoverNext)
    {
        m_bHoverPrev = hp;
        m_bHoverNext = hn;
        Invalidate(FALSE);
    }
    CDialog::OnMouseMove(nFlags, point);
}


// ============================================================================
// OnLButtonDblClk - treat rapid double-click same as single click
// ============================================================================
void CShopDownDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    OnLButtonDown(nFlags, point);
}
// ============================================================================
// OnLButtonUp - fire nav click on release (no UpdateWindow delay)
// ============================================================================
void CShopDownDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_bPressedPrev || m_bPressedNext)
    {
        bool wasPrev = m_bPressedPrev;
        bool wasNext = m_bPressedNext;
        m_bPressedPrev = false;
        m_bPressedNext = false;
        ReleaseCapture();
        Invalidate(FALSE);
        if (wasPrev && m_nCurrentPage > 0)
            OnPrevPageClick();
        else if (wasNext && m_nCurrentPage < kTotalPages - 1)
            OnNextPageClick();
    }
    else
    {
        CDialog::OnLButtonUp(nFlags, point);
    }
}
void CShopDownDlg::OnMouseLeave()
{
    m_bMouseTracked = false;
    m_bPressedPrev = false;
    m_bPressedNext = false;
    if (m_bHoverPrev || m_bHoverNext)
    {
        m_bHoverPrev = false;
        m_bHoverNext = false;
        Invalidate(FALSE);
    }
}
// ============================================================================
// OnTimer - page indicator slide animation tick
// ============================================================================
void CShopDownDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 42 && m_nNavAnim > 0)
    {
        m_nNavAnim--;
        Invalidate(FALSE);
        if (m_nNavAnim == 0) KillTimer(42);
        return;
    }
    CDialog::OnTimer(nIDEvent);
}
void CShopDownDlg::UpdatePageButtons()
{
    Invalidate(FALSE);
}
