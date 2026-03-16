#include "stdafx.h"
#include "ShopDownDlg.h"
#include <string>

// Info panel text colors (card layout)
static const COLORREF kClrInfoLabel   = RGB(120, 128, 142); // secondary label text
static const COLORREF kClrInfoValue   = RGB( 24,  28,  35); // primary value text
static const COLORREF kClrInfoEmpty   = RGB(160, 165, 175); // empty / placeholder text
static const COLORREF kClrEditText    = RGB( 18,  24,  40); // edit control text (enabled)
static const COLORREF kClrEditTextDis = RGB(120, 125, 135); // edit control text (disabled)

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

void CShopDownDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_SHOPDOWN_BTN_DOWNLOAD_1, m_btnDownload[0]);
    DDX_Control(pDX, IDC_SHOPDOWN_BTN_DOWNLOAD_2, m_btnDownload[1]);
    DDX_Control(pDX, IDC_SHOPDOWN_BTN_DELETE_1,   m_btnDelete[0]);
    DDX_Control(pDX, IDC_SHOPDOWN_BTN_DELETE_2,   m_btnDelete[1]);
    DDX_Control(pDX, IDC_SHOPDOWN_EDIT_PROD_1,    m_editProd[0]);
    DDX_Control(pDX, IDC_SHOPDOWN_EDIT_PROD_2,    m_editProd[1]);
    DDX_Control(pDX, IDC_SHOPDOWN_EDIT_BIZ_1,     m_editBiz[0]);
    DDX_Control(pDX, IDC_SHOPDOWN_EDIT_BIZ_2,     m_editBiz[1]);
    DDX_Control(pDX, IDC_SHOPDOWN_EDIT_PWD_1,     m_editPwd[0]);
    DDX_Control(pDX, IDC_SHOPDOWN_EDIT_PWD_2,     m_editPwd[1]);
}

BEGIN_MESSAGE_MAP(CShopDownDlg, CDialog)
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_WM_SETCURSOR()
    ON_WM_ACTIVATE()
    ON_WM_NCACTIVATE()
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
    , m_hFontLbl(nullptr)
    , m_hFontVal(nullptr)
    , m_hFontValBold(nullptr)
    , m_bHoverPrev(false)
    , m_bHoverNext(false)
    , m_bPressedPrev(false)
    , m_bPressedNext(false)
    , m_bMouseTracked(false)
    , m_nNavAnim(0)
    , m_bNavAnimNext(false)
    , m_nAnimFromPage(0)
    , m_fPillFrom(0.f)
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
    // 창 소멸 시 실행 중인 애니메이션 타이머가 있다면 안전하게 해제
    KillTimer(42);

    if (m_hFontLbl)     { ::DeleteObject(m_hFontLbl);     m_hFontLbl     = nullptr; }
    if (m_hFontVal)     { ::DeleteObject(m_hFontVal);     m_hFontVal     = nullptr; }
    if (m_hFontValBold) { ::DeleteObject(m_hFontValBold); m_hFontValBold = nullptr; }
    m_memDC.DeleteDC();
    m_memBmp.DeleteObject();
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

    // +++ 수정: 자식 컨트롤 간섭 차단 및 시스템 더블 버퍼링 적용
    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    m_brushBg.CreateSolidBrush(KFTC_DLG_CONTENT_BG);
    m_brushCard.CreateSolidBrush(RGB(255, 255, 255));
    m_brushCardDisabled.CreateSolidBrush(KFTC_CARD_DISABLED_BG);

    SetRedraw(FALSE);
    CreateControlsOnce();
    ApplyFonts();
    // 숨겨져 있던 컨트롤들의 좌표를 계산하고 SWP_SHOWWINDOW로 정렬하여 띄움
    LayoutControls();

    SetRedraw(TRUE);

    return TRUE;
}

// ============================================================================
// CreateControlsOnce
// ============================================================================
void CShopDownDlg::CreateControlsOnce()
{
    // 데이터 초기화 부분은 기존과 동일
    struct { LPCTSTR prod; LPCTSTR biz; LPCTSTR name; } data[25] = {
        {_T("K074404214"), _T("1050844729"), _T("금융결제원 테스트")},
    };

    for (int i = 0; i < kRowCount; ++i)
    {
        m_rowData[i].prdid = (data[i].prod ? data[i].prod : _T(""));
        m_rowData[i].regno = (data[i].biz ? data[i].biz : _T(""));
        m_rowData[i].retail_name = (data[i].name ? data[i].name : _T(""));
        m_rowData[i].second_name = _T("");
    }

    // Edit/버튼 컨트롤은 리소스 기반 정적 컨트롤로 전환했으므로
    // 여기서는 데이터만 초기화하고, 실제 스타일/색상은 컨트롤 인스턴스에 적용한다.

    for (int slot = 0; slot < kRowsPerPage; ++slot)
    {
        m_btnDownload[slot].SetColors(KFTC_PRIMARY, KFTC_PRIMARY_HOVER, RGB(255, 255, 255));
        m_btnDownload[slot].SetTextPx(14);
        m_btnDelete[slot].SetColors(KFTC_BTN_SECONDARY, KFTC_BTN_SECONDARY_HOV, RGB(40, 40, 40));
        m_btnDelete[slot].SetTextPx(14);
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
    ModernUIFont::ApplyUIFontFace(lf);

    // Field labels: match other tabs (13px)
    lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 14);
    lf.lfWeight = FW_NORMAL;
    m_fontHeader.DeleteObject();
    m_fontHeader.CreateFontIndirect(&lf);

    // Cell text (edit, button): match other tabs (13px)
    lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 14);
    lf.lfWeight = FW_NORMAL;
    m_fontCell.DeleteObject();
    m_fontCell.CreateFontIndirect(&lf);

    // Row number badge: semi-bold 13px
    lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, 14);
    lf.lfWeight = FW_SEMIBOLD;
    m_fontBadge.DeleteObject();
    m_fontBadge.CreateFontIndirect(&lf);

    for (int slot = 0; slot < kRowsPerPage; ++slot)
    {
        m_editProd[slot].SetFont(&m_fontCell, FALSE);
        m_editBiz[slot].SetFont(&m_fontCell, FALSE);
        m_editPwd[slot].SetFont(&m_fontCell, FALSE);
        m_btnDownload[slot].SetFont(&m_fontCell, FALSE);
        m_btnDelete[slot].SetFont(&m_fontCell, FALSE);
    }

    // Pre-create GDI paint fonts so first OnPaint does not block on font load
    {
        const int dpi = (int)ModernUIDpi::GetDpiForHwnd(m_hWnd);
        if (m_nCachedPaintDpi != dpi || !m_hFontLbl)
        {
            if (m_hFontLbl)     { ::DeleteObject(m_hFontLbl);     m_hFontLbl     = nullptr; }
            if (m_hFontVal)     { ::DeleteObject(m_hFontVal);     m_hFontVal     = nullptr; }
            if (m_hFontValBold) { ::DeleteObject(m_hFontValBold); m_hFontValBold = nullptr; }
            const int lblPx = ModernUIDpi::Scale(m_hWnd, 14);
            const int valPx = ModernUIDpi::Scale(m_hWnd, 14);
            LOGFONT lf = {};
            lf.lfCharSet = DEFAULT_CHARSET;
            ModernUIFont::ApplyUIFontFace(lf);
            lf.lfHeight = -lblPx;
            lf.lfWeight = FW_NORMAL;
            m_hFontLbl = ::CreateFontIndirect(&lf);
            lf.lfHeight = -valPx;
            m_hFontVal = ::CreateFontIndirect(&lf);
            lf.lfWeight = FW_BOLD;
            m_hFontValBold = ::CreateFontIndirect(&lf);
            m_nCachedPaintDpi = dpi;
        }
    }
}

void CShopDownDlg::LayoutControls()
{
    if (m_bInLayout || !GetSafeHwnd()) return;
    m_bInLayout = TRUE;

    CRect rc;
    GetClientRect(&rc);
    if (rc.IsRectEmpty()) { m_bInLayout = FALSE; return; }

    const int padX = ModernUIDpi::Scale(m_hWnd, 12);
    const int padY = ModernUIDpi::Scale(m_hWnd, 8);
    const int navH = ModernUIDpi::Scale(m_hWnd, 44);
    const int rowGap = ModernUIDpi::Scale(m_hWnd, 12);
    const int ctrlH = ModernUIDpi::Scale(m_hWnd, 38);

    // [1] 네비게이션 좌표 계산
    const int navY_bot = rc.Height() - padY - navH;
    m_rcNavBar.SetRect(padX, navY_bot, rc.Width() - padX, navY_bot + navH);

    const int navBtnS = ModernUIDpi::Scale(m_hWnd, 32);
    const int navGap = ModernUIDpi::Scale(m_hWnd, 140);
    const int navCX = m_rcNavBar.CenterPoint().x;
    const int navCY = m_rcNavBar.CenterPoint().y;

    m_rcPrevBtn.SetRect(navCX - navGap / 2 - navBtnS, navCY - navBtnS / 2, navCX - navGap / 2, navCY + navBtnS / 2);
    m_rcNextBtn.SetRect(navCX + navGap / 2, navCY - navBtnS / 2, navCX + navGap / 2 + navBtnS, navCY + navBtnS / 2);

    // [2] 행 높이 및 카드 배치
    const int availH = max(0, navY_bot - padY - 6);
    int rowH = (availH - (kRowsPerPage - 1) * rowGap) / kRowsPerPage;
    m_card.Compute(rc.Width() - padX * 2, m_hWnd);

    HDWP hdwp = ::BeginDeferWindowPos(kRowsPerPage * 5);
    const int pageStart = m_nCurrentPage * kRowsPerPage;

    for (int slot = 0; slot < kRowsPerPage; ++slot)
    {
        const int rowIdx = pageStart + slot;
        const bool bValid = (rowIdx < kRowCount);
        const int y = padY + slot * (rowH + rowGap);

        m_rcRow[slot].SetRect(padX, y, rc.right - padX, y + rowH);
        CRect inner = m_rcRow[slot];
        inner.DeflateRect(m_card.cardPad, m_card.cardPad);

        const int xL = inner.left + m_card.idxW;
        const int labelH = m_card.labelH;
        const int labelGap = m_card.labelGap;
        const int editGap = m_card.editGap;

        const int y1Edit = inner.top + labelH + labelGap;
        const int halfW = (m_card.leftW - editGap) / 2;
        m_rcProd[slot].SetRect(xL, y1Edit, xL + halfW, y1Edit + ctrlH);
        m_rcBiz[slot].SetRect(xL + halfW + editGap, y1Edit, xL + m_card.leftW, y1Edit + ctrlH);

        const int y2Label = y1Edit + ctrlH + editGap;
        const int y2Edit = y2Label + labelH + labelGap;
        m_rcPwd[slot].SetRect(xL, y2Edit, xL + m_card.leftW, y2Edit + ctrlH);

        const int btnW = ModernUIDpi::Scale(m_hWnd, 84);
        const int btnGap = ModernUIDpi::Scale(m_hWnd, 8);
        const int btnY = inner.bottom - ctrlH;
        const int btnRight = inner.right - ModernUIDpi::Scale(m_hWnd, 4);
        m_rcDel[slot].SetRect(btnRight - btnW, btnY, btnRight, btnY + ctrlH);
        m_rcBtn[slot].SetRect(m_rcDel[slot].left - btnGap - btnW, btnY, m_rcDel[slot].left - btnGap, btnY + ctrlH);

        const int xR = xL + m_card.leftW + m_card.colGap;
        m_rcInfoRep[slot].SetRect(xR, y1Edit, xR + m_card.rightW, y1Edit + m_card.infoLineH);
        m_rcInfoTerm[slot].SetRect(xR, y2Label, xR + m_card.rightW, y2Label + m_card.labelH);
        m_rcInfoTermVal[slot].SetRect(xR, y2Edit, xR + m_card.rightW, y2Edit + m_card.infoLineH);

        const UINT flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOCOPYBITS;
        const UINT sw = bValid ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;

        if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_editProd[slot].m_hWnd, NULL, m_rcProd[slot].left, m_rcProd[slot].top, m_rcProd[slot].Width(), m_rcProd[slot].Height(), flags | sw);
        if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_editBiz[slot].m_hWnd, NULL, m_rcBiz[slot].left, m_rcBiz[slot].top, m_rcBiz[slot].Width(), m_rcBiz[slot].Height(), flags | sw);
        if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_editPwd[slot].m_hWnd, NULL, m_rcPwd[slot].left, m_rcPwd[slot].top, m_rcPwd[slot].Width(), m_rcPwd[slot].Height(), flags | sw);
        if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_btnDownload[slot].m_hWnd, NULL, m_rcBtn[slot].left, m_rcBtn[slot].top, m_rcBtn[slot].Width(), m_rcBtn[slot].Height(), flags | sw);
        if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_btnDelete[slot].m_hWnd, NULL, m_rcDel[slot].left, m_rcDel[slot].top, m_rcDel[slot].Width(), m_rcDel[slot].Height(), flags | sw);

        ApplyRowUnderlay(slot, FALSE);
    }
    if (hdwp) ::EndDeferWindowPos(hdwp);

    RebindSlots();
    m_bInLayout = FALSE;
    Invalidate(FALSE);
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
    InvalidateRect(&m_rcRow[slot], FALSE);
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
    InvalidateRect(&m_rcRow[slot], FALSE);
}


/*
[배경 지우기]
- 깜빡임을 줄이기 위해 보통 TRUE를 반환하거나, 자체 더블버퍼와 함께 사용합니다.
- 배경을 직접 그리는 구조라면 기본 처리(DefWindowProc)를 피하는 편이 좋습니다.
*/
BOOL CShopDownDlg::OnEraseBkgnd(CDC*) {
    /* * [중요] CDialog::OnEraseBkgnd(pDC)를 호출하면 안 됩니다!
         * 부모 함수를 호출하면 윈도우 기본 배경색(회색)으로 화면을 한 번 밀어버리기 때문에
         * 우리가 OnPaint에서 그린 내용과 충돌하여 '깜빡임'과 '주변 창 간섭'이 생깁니다.
         */

         // 배경을 지우지 않고 바로 OnPaint가 모든 영역을 그리도록 "TRUE"를 반환합니다.
    return TRUE;
}

HBRUSH CShopDownDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_DLG)
    {
        return (HBRUSH)m_brushBg.GetSafeHandle();
    }
    if (nCtlColor == CTLCOLOR_EDIT)
    {
        const BOOL bEnabled = pWnd && pWnd->IsWindowEnabled();
        // Trigger MFC reflection so CSkinnedEdit::CtlColor runs:
        // it sets BkMode(OPAQUE) + BkColor + rounded clip region on the DC.
        // Without this, the native edit renders text/caret directly via GetDC
        // without the clip, causing white bg to overwrite rounded border pixels (flicker).
        HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
        pDC->SetTextColor(bEnabled ? kClrEditText : kClrEditTextDis);
        return hbr;
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
    // [1] 기본 클래스 함수 호출
    CDialog::OnSize(nType, cx, cy);

    // [2] 창 크기가 유효하고, 컨트롤들이 생성된 상태인지 확인
    if (cx > 0 && cy > 0 && m_editProd[0].GetSafeHwnd())
    {
        // 핵심: 창 크기가 변할 때(또는 처음 결정될 때) 
        // 레이아웃을 다시 잡아 버튼 좌표(m_rcPrevBtn 등)를 생성합니다.
        LayoutControls();
    }
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
    CPaintDC dc(this);
    CRect rc;
    GetClientRect(&rc);
    if (rc.IsRectEmpty()) return;

    // [1] 더블 버퍼링용 메모리 DC 세팅
    if (!m_memDC.GetSafeHdc()) m_memDC.CreateCompatibleDC(&dc);
    if (m_memBmpSize != rc.Size())
    {
        m_memBmp.DeleteObject();
        m_memBmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
        m_memBmpSize = rc.Size();
    }
    CBitmap* pOldBmp = m_memDC.SelectObject(&m_memBmp);

    // 메모장 깜빡임 방지: 배경을 메모리 DC에서 먼저 단색으로 채움
    m_memDC.FillSolidRect(&rc, KFTC_DLG_CONTENT_BG);

    Gdiplus::Graphics g(m_memDC.m_hDC);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    if (m_hFontLbl == nullptr) ApplyFonts();

    const COLORREF clrLbl = kClrInfoLabel;
    const COLORREF clrVal = kClrInfoValue;
    const COLORREF clrPh  = kClrInfoEmpty;

    // [2] 카드형 행 데이터 렌더링
    const int pageStart = m_nCurrentPage * kRowsPerPage;
    for (int slot = 0; slot < kRowsPerPage; ++slot)
    {
        const int rowIdx = pageStart + slot;
        if (rowIdx >= kRowCount) continue;

        const CRect& r = m_rcRow[slot];
        CString rep = m_rowData[rowIdx].retail_name;
        CString term = m_rowData[rowIdx].second_name;
        const bool hasRep = !rep.IsEmpty();

        // 카드 배경
        Gdiplus::RectF rf((float)r.left, (float)r.top, (float)r.Width(), (float)r.Height());
        Gdiplus::GraphicsPath rp;
        ModernUIGfx::AddRoundRect(rp, rf, 12.f);
        Gdiplus::SolidBrush brCard(hasRep ? Gdiplus::Color(255, 255, 255, 255) : Gdiplus::Color(255, 246, 248, 251));
        g.FillPath(&brCard, &rp);

        const float yLbl = (float)r.top + (float)m_card.cardPad;
        const float hLbl = (float)m_card.labelH;

        {
            HDC hdcLbl = g.GetHDC();
            ::SetBkMode(hdcLbl, TRANSPARENT);
            HFONT hOldLbl = (HFONT)::SelectObject(hdcLbl, m_hFontLbl);
            ::SetTextColor(hdcLbl, clrLbl);
            RECT rcL1 = { m_rcProd[slot].left, (LONG)yLbl, m_rcProd[slot].right, (LONG)(yLbl + hLbl) };
            ::DrawTextW(hdcLbl, L"단말기 제품번호", -1, &rcL1, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
            RECT rcL2 = { m_rcBiz[slot].left, (LONG)yLbl, m_rcBiz[slot].right, (LONG)(yLbl + hLbl) };
            ::DrawTextW(hdcLbl, L"사업자번호", -1, &rcL2, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
            const float yPwdLbl = (float)m_rcPwd[slot].top - (float)m_card.labelGap - (float)m_card.labelH;
            RECT rcL3 = { m_rcPwd[slot].left, (LONG)yPwdLbl, m_rcPwd[slot].right, (LONG)(yPwdLbl + hLbl) };
            ::DrawTextW(hdcLbl, L"비밀번호", -1, &rcL3, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
            RECT rcL4 = { m_rcInfoRep[slot].left, (LONG)yLbl, m_rcInfoRep[slot].right, (LONG)(yLbl + hLbl) };
            ::DrawTextW(hdcLbl, L"대표 상호명", -1, &rcL4, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
            RECT rcL5 = { m_rcInfoTerm[slot].left, m_rcInfoTerm[slot].top, m_rcInfoTerm[slot].right, m_rcInfoTerm[slot].bottom };
            ::DrawTextW(hdcLbl, L"단말기별 상호명", -1, &rcL5, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
            ::SelectObject(hdcLbl, hOldLbl);
            g.ReleaseHDC(hdcLbl);
        }

        // 대표 상호명 값 표시
        CStringW strRepW;
        if (hasRep) strRepW = rep;
        else strRepW = L"다운로드 후 표시";

        // 단말기별 상호명 값 표시
        CStringW strTermW;
        if (!term.IsEmpty()) strTermW = term;
        else if (hasRep) strTermW = L"-";
        else strTermW = L"다운로드 후 표시";

        {
            HDC hdcVal = g.GetHDC();
            ::SetBkMode(hdcVal, TRANSPARENT);
            HFONT hOldVal = (HFONT)::SelectObject(hdcVal, hasRep ? m_hFontValBold : m_hFontVal);
            ::SetTextColor(hdcVal, hasRep ? clrVal : clrPh);
            RECT rcV1 = { m_rcInfoRep[slot].left, m_rcInfoRep[slot].top, m_rcInfoRep[slot].right, m_rcInfoRep[slot].bottom };
            ::DrawTextW(hdcVal, strRepW, -1, &rcV1, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
            ::SelectObject(hdcVal, (!term.IsEmpty()) ? m_hFontValBold : m_hFontVal);
            ::SetTextColor(hdcVal, (!term.IsEmpty() || hasRep) ? clrVal : clrPh);
            RECT rcV2 = { m_rcInfoTermVal[slot].left, m_rcInfoTermVal[slot].top, m_rcInfoTermVal[slot].right, m_rcInfoTermVal[slot].bottom };
            ::DrawTextW(hdcVal, strTermW, -1, &rcV2, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
            ::SelectObject(hdcVal, hOldVal);
            g.ReleaseHDC(hdcVal);
        }

        // 행 번호
        CString strNum; strNum.Format(_T("%d"), rowIdx + 1);
        CStringW strNumW(strNum);
        {
            HDC hdcBadge = g.GetHDC();
            ::SetBkMode(hdcBadge, TRANSPARENT);
            HFONT hOldBadge = (HFONT)::SelectObject(hdcBadge, m_hFontValBold);
            ::SetTextColor(hdcBadge, RGB(92, 102, 120));
            RECT rcBadge = { (LONG)(r.left + 8), (LONG)(r.top + m_card.cardPad - 1), (LONG)(r.left + 8 + m_card.idxW - 12), (LONG)(r.top + m_card.cardPad - 1 + m_card.labelH + 4) };
            ::DrawTextW(hdcBadge, strNumW, -1, &rcBadge, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            ::SelectObject(hdcBadge, hOldBadge);
            g.ReleaseHDC(hdcBadge);
        }
    }

    // [3] 하단 네비게이션 버튼 렌더링 (사라짐 문제 해결)
    if (!m_rcNavBar.IsRectEmpty() && !m_rcPrevBtn.IsRectEmpty())
    {
        const int totalP = (kRowCount + kRowsPerPage - 1) / kRowsPerPage;
        const bool bCanPrev = (m_nCurrentPage > 0);
        const bool bCanNext = (m_nCurrentPage < totalP - 1);

        auto DrawNavBtn = [&](const CRect& btnRc, bool bEnabled, bool bHover, bool bPressed, bool bLeft) {
            Gdiplus::RectF rf((float)btnRc.left, (float)btnRc.top, (float)btnRc.Width(), (float)btnRc.Height());
            if (bPressed && bEnabled) rf.Inflate(-rf.Width * 0.1f, -rf.Height * 0.1f);

            Gdiplus::Color fillClr = bEnabled ? (bPressed ? Gdiplus::Color(255, 0, 76, 168) : (bHover ? Gdiplus::Color(255, 15, 124, 255) : Gdiplus::Color(255, 0, 100, 221)))
                : Gdiplus::Color(255, 235, 237, 240);
            Gdiplus::SolidBrush br(fillClr);
            g.FillEllipse(&br, rf);

            Gdiplus::Pen pen(bEnabled ? Gdiplus::Color::White : Gdiplus::Color(255, 148, 163, 184), rf.Width * 0.08f);
            pen.SetStartCap(Gdiplus::LineCapRound); pen.SetEndCap(Gdiplus::LineCapRound);
            float cx = rf.X + rf.Width / 2.f, cy = rf.Y + rf.Height / 2.f, aw = rf.Width * 0.15f, ah = rf.Height * 0.22f;
            if (bLeft) { Gdiplus::PointF pts[3] = { {cx + aw, cy - ah}, {cx - aw, cy}, {cx + aw, cy + ah} }; g.DrawLines(&pen, pts, 3); }
            else { Gdiplus::PointF pts[3] = { {cx - aw, cy - ah}, {cx + aw, cy}, {cx - aw, cy + ah} }; g.DrawLines(&pen, pts, 3); }
            };

        DrawNavBtn(m_rcPrevBtn, bCanPrev, m_bHoverPrev, m_bPressedPrev, true);
        DrawNavBtn(m_rcNextBtn, bCanNext, m_bHoverNext, m_bPressedNext, false);

        CString pageStr; pageStr.Format(_T("%d / %d"), m_nCurrentPage + 1, totalP);
        CStringW pageStrW = pageStr;
        {
            HDC hdcNav = g.GetHDC();
            ::SetBkMode(hdcNav, TRANSPARENT);
            HFONT hOldNav = (HFONT)::SelectObject(hdcNav, m_hFontValBold);
            ::SetTextColor(hdcNav, RGB(60, 70, 90));
            RECT rcNav = { m_rcNavBar.left, m_rcNavBar.top, m_rcNavBar.right, m_rcNavBar.bottom };
            ::DrawTextW(hdcNav, pageStrW, -1, &rcNav, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            ::SelectObject(hdcNav, hOldNav);
            g.ReleaseHDC(hdcNav);
        }
    }

    // 최종 비트맵 전송
    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &m_memDC, 0, 0, SRCCOPY);
    m_memDC.SelectObject(pOldBmp);
}

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

void CShopDownDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    // +++ 수정: CDialog::OnActivate(nState, pWndOther, bMinimized); 를 호출하지 않습니다.
    // 시스템 기본 메시지 처리기로 넘겨서 불필요한 자식 컨트롤 순회를 차단합니다.
    ::DefWindowProc(m_hWnd, WM_ACTIVATE, nState, (LPARAM)(pWndOther ? pWndOther->m_hWnd : NULL));
}

BOOL CShopDownDlg::OnNcActivate(BOOL bActive)
{
    // +++ 수정: 부모 클래스 호출 없이 바로 기본 윈도우 프로시저로 처리
    return (BOOL)::DefWindowProc(m_hWnd, WM_NCACTIVATE, bActive, 0);
}

// ============================================================================
// Pagination
// ============================================================================
void CShopDownDlg::RefreshPage()
{
    RebindSlots();
    ApplyAllRowUnderlays(); // sync underlay colors after page data changes
    UpdatePageButtons();
    if (IsWindowVisible())
        Invalidate(FALSE);
}

void CShopDownDlg::OnPrevPageClick()
{
    if (m_nCurrentPage > 0)
    {
        // Capture current visual pill position (handles rapid clicks during animation)
        if (m_nNavAnim > 0) {
            float t = 1.f - (float)m_nNavAnim / 10.f;
            float u = 1.f - t;
            float prog = 1.f - u * u * u;
            m_fPillFrom = (float)m_nAnimFromPage + ((float)m_nCurrentPage - (float)m_nAnimFromPage) * prog;
        } else {
            m_fPillFrom = (float)m_nCurrentPage;
        }
        m_nAnimFromPage = m_nCurrentPage;
        m_nCurrentPage--;
        m_bNavAnimNext = false; m_nNavAnim = 10;
        KillTimer(42); SetTimer(42, 16, NULL);
        RefreshPage();
    }
}

void CShopDownDlg::OnNextPageClick()
{
    if (m_nCurrentPage < kTotalPages - 1)
    {
        if (m_nNavAnim > 0) {
            float t = 1.f - (float)m_nNavAnim / 10.f;
            float u = 1.f - t;
            float prog = 1.f - u * u * u;
            m_fPillFrom = (float)m_nAnimFromPage + ((float)m_nCurrentPage - (float)m_nAnimFromPage) * prog;
        } else {
            m_fPillFrom = (float)m_nCurrentPage;
        }
        m_nAnimFromPage = m_nCurrentPage;
        m_nCurrentPage++;
        m_bNavAnimNext = true; m_nNavAnim = 10;
        KillTimer(42); SetTimer(42, 16, NULL);
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
        InvalidateRect(&m_rcNavBar, FALSE);
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
        InvalidateRect(&m_rcNavBar, FALSE);
    }
    CDialog::OnMouseMove(nFlags, point);
}


// ============================================================================
// OnSetCursor - hand cursor over prev/next nav buttons
// ============================================================================
BOOL CShopDownDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (nHitTest == HTCLIENT)
    {
        CPoint pt;
        ::GetCursorPos(&pt);
        ScreenToClient(&pt);
        if ((!m_rcPrevBtn.IsRectEmpty() && m_rcPrevBtn.PtInRect(pt)) ||
            (!m_rcNextBtn.IsRectEmpty() && m_rcNextBtn.PtInRect(pt)))
        {
            ::SetCursor(::LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
    }
    return CDialog::OnSetCursor(pWnd, nHitTest, message);
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
        InvalidateRect(&m_rcNavBar, FALSE);
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
        InvalidateRect(&m_rcNavBar, FALSE);
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
        InvalidateRect(&m_rcNavBar, FALSE);
        if (m_nNavAnim == 0) KillTimer(42);
        return;
    }
    CDialog::OnTimer(nIDEvent);
}
void CShopDownDlg::UpdatePageButtons()
{
    InvalidateRect(&m_rcNavBar, FALSE);
}