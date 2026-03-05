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
{
    memset(m_bRowCreated, 0, sizeof(m_bRowCreated));
    InitPointerArrays();
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

void CShopDownDlg::InitPointerArrays()
{
    m_pPrdid[0]=&m_prdid1;  m_pPrdid[1]=&m_prdid2;  m_pPrdid[2]=&m_prdid3;  m_pPrdid[3]=&m_prdid4;  m_pPrdid[4]=&m_prdid5;
    m_pPrdid[5]=&m_prdid6;  m_pPrdid[6]=&m_prdid7;  m_pPrdid[7]=&m_prdid8;  m_pPrdid[8]=&m_prdid9;  m_pPrdid[9]=&m_prdid10;
    m_pPrdid[10]=&m_prdid11; m_pPrdid[11]=&m_prdid12; m_pPrdid[12]=&m_prdid13; m_pPrdid[13]=&m_prdid14; m_pPrdid[14]=&m_prdid15;
    m_pPrdid[15]=&m_prdid16; m_pPrdid[16]=&m_prdid17; m_pPrdid[17]=&m_prdid18; m_pPrdid[18]=&m_prdid19; m_pPrdid[19]=&m_prdid20;

    m_pRegno[0]=&m_regno1;  m_pRegno[1]=&m_regno2;  m_pRegno[2]=&m_regno3;  m_pRegno[3]=&m_regno4;  m_pRegno[4]=&m_regno5;
    m_pRegno[5]=&m_regno6;  m_pRegno[6]=&m_regno7;  m_pRegno[7]=&m_regno8;  m_pRegno[8]=&m_regno9;  m_pRegno[9]=&m_regno10;
    m_pRegno[10]=&m_regno11; m_pRegno[11]=&m_regno12; m_pRegno[12]=&m_regno13; m_pRegno[13]=&m_regno14; m_pRegno[14]=&m_regno15;
    m_pRegno[15]=&m_regno16; m_pRegno[16]=&m_regno17; m_pRegno[17]=&m_regno18; m_pRegno[18]=&m_regno19; m_pRegno[19]=&m_regno20;

    m_pPasswd[0]=&m_passwd1;  m_pPasswd[1]=&m_passwd2;  m_pPasswd[2]=&m_passwd3;  m_pPasswd[3]=&m_passwd4;  m_pPasswd[4]=&m_passwd5;
    m_pPasswd[5]=&m_passwd6;  m_pPasswd[6]=&m_passwd7;  m_pPasswd[7]=&m_passwd8;  m_pPasswd[8]=&m_passwd9;  m_pPasswd[9]=&m_passwd10;
    m_pPasswd[10]=&m_passwd11; m_pPasswd[11]=&m_passwd12; m_pPasswd[12]=&m_passwd13; m_pPasswd[13]=&m_passwd14; m_pPasswd[14]=&m_passwd15;
    m_pPasswd[15]=&m_passwd16; m_pPasswd[16]=&m_passwd17; m_pPasswd[17]=&m_passwd18; m_pPasswd[18]=&m_passwd19; m_pPasswd[19]=&m_passwd20;

    m_pRetailName[0]=&m_retail_name1;  m_pRetailName[1]=&m_retail_name2;  m_pRetailName[2]=&m_retail_name3;  m_pRetailName[3]=&m_retail_name4;  m_pRetailName[4]=&m_retail_name5;
    m_pRetailName[5]=&m_retail_name6;  m_pRetailName[6]=&m_retail_name7;  m_pRetailName[7]=&m_retail_name8;  m_pRetailName[8]=&m_retail_name9;  m_pRetailName[9]=&m_retail_name10;
    m_pRetailName[10]=&m_retail_name11; m_pRetailName[11]=&m_retail_name12; m_pRetailName[12]=&m_retail_name13; m_pRetailName[13]=&m_retail_name14; m_pRetailName[14]=&m_retail_name15;
    m_pRetailName[15]=&m_retail_name16; m_pRetailName[16]=&m_retail_name17; m_pRetailName[17]=&m_retail_name18; m_pRetailName[18]=&m_retail_name19; m_pRetailName[19]=&m_retail_name20;

    m_pSecondName[0]=&m_second_name1;  m_pSecondName[1]=&m_second_name2;  m_pSecondName[2]=&m_second_name3;  m_pSecondName[3]=&m_second_name4;  m_pSecondName[4]=&m_second_name5;
    m_pSecondName[5]=&m_second_name6;  m_pSecondName[6]=&m_second_name7;  m_pSecondName[7]=&m_second_name8;  m_pSecondName[8]=&m_second_name9;  m_pSecondName[9]=&m_second_name10;
    m_pSecondName[10]=&m_second_name11; m_pSecondName[11]=&m_second_name12; m_pSecondName[12]=&m_second_name13; m_pSecondName[13]=&m_second_name14; m_pSecondName[14]=&m_second_name15;
    m_pSecondName[15]=&m_second_name16; m_pSecondName[16]=&m_second_name17; m_pSecondName[17]=&m_second_name18; m_pSecondName[18]=&m_second_name19; m_pSecondName[19]=&m_second_name20;
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

    // Create Prev/Next navigation buttons (always visible)
    m_btnPrevPage.Create(_T("<"),
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS|BS_OWNERDRAW,
        CRect(0,0,10,10), this, kBtnPrev);
    m_btnPrevPage.SetColors(KFTC_BTN_SECONDARY, KFTC_BTN_SECONDARY_HOV, RGB(40,40,40));

    m_btnNextPage.Create(_T(">"),
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS|BS_OWNERDRAW,
        CRect(0,0,10,10), this, kBtnNext);
    m_btnNextPage.SetColors(KFTC_PRIMARY, KFTC_PRIMARY_HOVER, RGB(255,255,255));

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
        *m_pPrdid[i]      = (data[i].prod ? data[i].prod : _T(""));
        *m_pRegno[i]      = (data[i].biz  ? data[i].biz  : _T(""));
        *m_pRetailName[i] = (data[i].name ? data[i].name : _T(""));
        *m_pSecondName[i] = _T("");
    }
}

// ============================================================================
// CreateRow  -- lazily create Win32 controls for a single row
// Called from LayoutControls() the first time a row scrolls into view.
// ============================================================================
void CShopDownDlg::CreateRow(int i)
{
    if (i < 0 || i >= kRowCount || m_bRowCreated[i]) return;

    const DWORD edtBase  = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS
                         | ES_AUTOHSCROLL | ES_CENTER;
    const DWORD edtStyle = edtBase;
    const DWORD pwdStyle = edtBase | ES_PASSWORD;
    const DWORD roStyle  = edtBase | ES_READONLY;

    m_editProd[i].CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
        edtStyle, CRect(0,0,10,10), this, 62000+(i*10)+1);
    m_editBiz[i].CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
        edtStyle, CRect(0,0,10,10), this, 62000+(i*10)+2);
    m_editPwd[i].CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
        pwdStyle, CRect(0,0,10,10), this, 62000+(i*10)+3);
    m_btnDownload[i].Create(_T("다운로드"),
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS|BS_OWNERDRAW,
        CRect(0,0,10,10), this, kBtnBase+i);
    m_btnDownload[i].SetColors(KFTC_PRIMARY, KFTC_PRIMARY_HOVER, RGB(255,255,255));
    m_btnDelete[i].Create(_T("삭제"),
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS|BS_OWNERDRAW,
        CRect(0,0,10,10), this, kDelBase+i);
    m_btnDelete[i].SetColors(KFTC_BTN_SECONDARY, KFTC_BTN_SECONDARY_HOV, RGB(40,40,40));

    m_editProd[i].SetWindowText(*m_pPrdid[i]);
    m_editBiz[i].SetWindowText(*m_pRegno[i]);

    const bool hasRep = (m_pRetailName[i] && !m_pRetailName[i]->IsEmpty());
    m_btnDelete[i].EnableWindow(hasRep ? TRUE : FALSE);

    if (m_fontCell.GetSafeHandle())
    {
        m_editProd[i].SetFont(&m_fontCell);
        m_editBiz[i].SetFont(&m_fontCell);
        m_editPwd[i].SetFont(&m_fontCell);
        m_btnDownload[i].SetFont(&m_fontCell);
    }

    m_bRowCreated[i] = true;
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

    for (int i = 0; i < kRowCount; ++i)
    {
        if (!m_bRowCreated[i]) continue;
        m_editProd[i].SetFont(&m_fontCell);
        m_editBiz[i].SetFont(&m_fontCell);
        m_editPwd[i].SetFont(&m_fontCell);
        m_btnDownload[i].SetFont(&m_fontCell);
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
    const int navH   = ModernUIDpi::Scale(m_hWnd, 56);

    const int availH = max(0, rc.Height() - padY * 2 - navH);
    int rowH = (availH - rowGap) / kRowsPerPage;

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

    const int pageStart = m_nCurrentPage * kRowsPerPage;

    // prod/biz/pwd/download/delete (5 per row) + 2 nav buttons
    HDWP hdwp = ::BeginDeferWindowPos(kRowCount * 5 + 2);

    for (int i = 0; i < kRowCount; ++i)
    {
        const bool bOnPage = (i >= pageStart && i < pageStart + kRowsPerPage);
        const int slot = i - pageStart;  // 0 or 1 for on-page rows

        // On-page rows get real coords; off-page rows go off-screen
        const int y = bOnPage ? (padY + slot * (rowH + rowGap)) : -(rowH * 2);

        m_rcRow[i].SetRect(padX, y, rc.right - padX, y + rowH);

        const int cardPad = m_card.cardPad;
        CRect inner = m_rcRow[i];
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

        m_rcProd[i].SetRect(xL,                 y1Edit, xL + halfW,         y1Edit + ctrlH);
        m_rcBiz[i].SetRect (xL + halfW+editGap, y1Edit, xL + m_card.leftW,  y1Edit + ctrlH);

        // row2 (pwd)
        const int y2Label = y1Edit + ctrlH + editGap;
        const int y2Edit  = y2Label + labelH + labelGap;
        m_rcPwd[i].SetRect(xL, y2Edit, xL + m_card.leftW, y2Edit + ctrlH);

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

            m_rcDel[i].SetRect(right - btnW, btnY, right, btnY + btnH);
            m_rcBtn[i].SetRect(m_rcDel[i].left - btnGap - btnW, btnY, m_rcDel[i].left - btnGap, btnY + btnH);

            termRight = xR + m_card.rightW;
        }

        // info text rectangles
        const int yInfo1 = inner.top + labelH + labelGap;
        m_rcInfoRep[i].SetRect(xR, yInfo1, xR + m_card.rightW, yInfo1 + m_card.infoLineH);

        const int repValBottom = yInfo1 + m_card.infoLineH;
        const int yTermLabel   = repValBottom + editGap;
        m_rcInfoTerm[i].SetRect(xR, yTermLabel, termRight, yTermLabel + m_card.labelH);

        const int yTermVal = yTermLabel + m_card.labelH + labelGap;
        m_rcInfoTermVal[i].SetRect(xR, yTermVal, termRight, yTermVal + m_card.infoLineH);

        // Lazy creation: only create controls for on-page rows
        if (bOnPage && !m_bRowCreated[i])
            CreateRow(i);

        // Enabled/disabled visual states
        {
            ApplyRowUnderlay(i, FALSE);
            const BOOL enBtn = m_bRowCreated[i] ? ::IsWindowEnabled(m_btnDownload[i].m_hWnd) : FALSE;
            if (enBtn)
                m_btnDownload[i].SetColors(KFTC_PRIMARY, KFTC_PRIMARY_HOVER, RGB(255,255,255));
            else
                m_btnDownload[i].SetColors(KFTC_BTN_DISABLED_BG, KFTC_BTN_DISABLED_BG, RGB(160,165,175));

            const BOOL enDel = m_bRowCreated[i] ? ::IsWindowEnabled(m_btnDelete[i].m_hWnd) : FALSE;
            if (enDel)
                m_btnDelete[i].SetColors(KFTC_BTN_SECONDARY, KFTC_BTN_SECONDARY_HOV, RGB(40,40,40));
            else
                m_btnDelete[i].SetColors(KFTC_CARD_DISABLED_BG, KFTC_CARD_DISABLED_BG, RGB(160,160,160));
        }

        const UINT baseFlags = SWP_NOZORDER | SWP_NOACTIVATE;
        const UINT showHide  = bOnPage ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;

        if (m_bRowCreated[i])
        {
            if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_editProd[i].m_hWnd,    NULL, m_rcProd[i].left, m_rcProd[i].top, m_rcProd[i].Width(), m_rcProd[i].Height(), baseFlags | showHide);
            if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_editBiz[i].m_hWnd,     NULL, m_rcBiz[i].left,  m_rcBiz[i].top,  m_rcBiz[i].Width(),  m_rcBiz[i].Height(),  baseFlags | showHide);
            if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_editPwd[i].m_hWnd,     NULL, m_rcPwd[i].left,  m_rcPwd[i].top,  m_rcPwd[i].Width(),  m_rcPwd[i].Height(),  baseFlags | showHide);
            if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_btnDownload[i].m_hWnd, NULL, m_rcBtn[i].left,  m_rcBtn[i].top,  m_rcBtn[i].Width(),  m_rcBtn[i].Height(),  baseFlags | showHide);
            if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_btnDelete[i].m_hWnd,   NULL, m_rcDel[i].left,  m_rcDel[i].top,  m_rcDel[i].Width(),  m_rcDel[i].Height(),  baseFlags | showHide);
        }
    }

    // Navigation bar (below the 2 visible cards)
    const int navY = padY + kRowsPerPage * (rowH + rowGap) - rowGap;
    m_rcNavBar.SetRect(padX, navY, rc.right - padX, navY + navH);

    const int navBtnW = ModernUIDpi::Scale(m_hWnd, 88);
    const int navBtnH = ModernUIDpi::Scale(m_hWnd, 36);
    const int navBtnY = navY + (navH - navBtnH) / 2;
    const int midX    = rc.Width() / 2;
    const int half    = ModernUIDpi::Scale(m_hWnd, 56);

    if (m_btnPrevPage.GetSafeHwnd())
    {
        if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_btnPrevPage.m_hWnd, NULL,
            midX - half - navBtnW, navBtnY, navBtnW, navBtnH,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
    if (m_btnNextPage.GetSafeHwnd())
    {
        if (hdwp) hdwp = ::DeferWindowPos(hdwp, m_btnNextPage.m_hWnd, NULL,
            midX + half, navBtnY, navBtnW, navBtnH,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }

    if (hdwp) ::EndDeferWindowPos(hdwp);

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
        if (nID == kBtnPrev) { OnPrevPageClick(); return TRUE; }
        if (nID == kBtnNext) { OnNextPageClick(); return TRUE; }
        if (nID >= kBtnBase && nID < kBtnBase + kRowCount)
        {
            OnDownloadClick((int)(nID - kBtnBase));
            return TRUE;
        }
        if (nID >= kDelBase && nID < kDelBase + kRowCount)
        {
            OnDeleteClick((int)(nID - kDelBase));
            return TRUE;
        }
    }
    return CDialog::OnCommand(wParam, lParam);
}

void CShopDownDlg::OnDownloadClick(int index)
{
    /* [UI-STEP] 다운로드 버튼 처리(UI 연동)
     * 1) 선택된 단말/가맹점 정보를 확인한다.
     * 2) 다운로드 요청을 수행한 뒤 결과를 리스트/카드에 반영한다.
     * 3) 반영 후 Invalidate()로 화면 갱신한다.
     */

    if (index < 0 || index >= kRowCount) return;

    // 테스트용: 다운로드 시 대표가맹점명(RetailName)에 "TEST"를 채워 카드 상태(배경)를 즉시 전환
    if (m_pRetailName[index])
        *m_pRetailName[index] = _T("TEST");

    // 필요 시 단말기별 가맹점도 동일하게 표시(빈칸이면 보기 어색해서 같이 채움)
    if (m_pSecondName[index] && m_pSecondName[index]->IsEmpty())
        *m_pSecondName[index] = _T("TEST");

    // 화면/컨트롤 underlay 즉시 동기화
    // 다운로드되면 삭제 버튼 활성화
    if (m_btnDelete[index].GetSafeHwnd()) m_btnDelete[index].EnableWindow(TRUE);

    ApplyRowUnderlay(index, FALSE);

    RedrawWindow(&m_rcRow[index], nullptr,
        RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

void CShopDownDlg::OnDeleteClick(int index)
{
    /* [UI-STEP] 삭제 버튼 처리(UI 연동)
     * 1) 선택 항목을 확인하고 삭제 동작을 수행한다.
     * 2) 삭제 후 스크롤 범위가 바뀌면 UpdateScrollBar()를 호출한다.
     * 3) Invalidate()로 화면 갱신한다.
     */

    if (index < 0 || index >= kRowCount) return;

    // 해당 카드(행)의 모든 데이터 초기화
    if (m_pRetailName[index])  *m_pRetailName[index]  = _T("");
    if (m_pSecondName[index])  *m_pSecondName[index]  = _T("");
    if (m_pPrdid[index])       *m_pPrdid[index]       = _T("");
    if (m_pRegno[index])       *m_pRegno[index]       = _T("");
    if (m_pPasswd[index])      *m_pPasswd[index]      = _T("");

    if (m_editProd[index].GetSafeHwnd()) m_editProd[index].SetWindowText(_T(""));
    if (m_editBiz[index].GetSafeHwnd())  m_editBiz[index].SetWindowText(_T(""));
    if (m_editPwd[index].GetSafeHwnd())  m_editPwd[index].SetWindowText(_T(""));

    // 삭제되면 삭제 버튼 비활성화
    if (m_btnDelete[index].GetSafeHwnd()) m_btnDelete[index].EnableWindow(FALSE);

    // 화면/컨트롤 underlay 즉시 동기화 (빈 상태 → 연한 톤)
    ApplyRowUnderlay(index, FALSE);

    RedrawWindow(&m_rcRow[index], nullptr,
        RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
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
    ModernUIGfx::EnsureGdiplusStartup();

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
    for (int i = 0; i < kRowCount; ++i)
    {
        const CRect& r = m_rcRow[i];
        if (i < pageStart || i >= pageStart + kRowsPerPage) continue;

        // Data-driven card tone:
        // - 대표 가맹점(다운로드 결과)이 있으면 정상 카드(white)
        // - 없으면(미다운로드/조회 실패/미존재 등) 살짝 톤을 주어 구분
        CString rep  = *m_pRetailName[i];
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

            Gdiplus::RectF lp((float)m_rcProd[i].left, yLbl, (float)m_rcProd[i].Width(), hLbl);
            Gdiplus::RectF lb((float)m_rcBiz[i].left,  yLbl, (float)m_rcBiz[i].Width(),  hLbl);
            DrawEllips(_T("단말기 제품번호"), lp, m_pFontLbl, cLbl);
            DrawEllips(_T("사업자번호"),     lb, m_pFontLbl, cLbl);

            // pwd label
            const float yPwdLbl = (float)m_rcPwd[i].top - (float)m_card.labelGap - (float)m_card.labelH;
            Gdiplus::RectF lpwd((float)m_rcPwd[i].left, yPwdLbl, (float)m_rcPwd[i].Width(), hLbl);
            DrawEllips(_T("비밀번호"), lpwd, m_pFontLbl, cLbl);
        }

        // right labels + values
        {
            const float yLbl = (float)r.top + (float)m_card.cardPad;
            const float hLbl = (float)m_card.labelH;

            // label rects align with value rects
            Gdiplus::RectF l1((float)m_rcInfoRep[i].left,  yLbl, (float)m_rcInfoRep[i].Width(), hLbl);
            DrawEllips(_T("대표 가맹점"), l1, m_pFontLbl, cLbl);

            // '단말기별 가맹점'은 2줄(라벨/값) 유지:
            //  - 라벨: 비밀번호 라벨 줄(y2Label)
            //  - 값  : 비밀번호 Edit 줄(y2Edit), 버튼과 동일한 Y (좌측 공간에 표시)
            Gdiplus::RectF l2((float)m_rcInfoTerm[i].left,    (float)m_rcInfoTerm[i].top,    (float)m_rcInfoTerm[i].Width(),    (float)m_rcInfoTerm[i].Height());
            DrawEllips(_T("단말기별 가맹점"), l2, m_pFontLbl, cLbl);

            CString rep  = *m_pRetailName[i];
            CString term = *m_pSecondName[i];

            // value rects
            Gdiplus::RectF v1((float)m_rcInfoRep[i].left,      (float)m_rcInfoRep[i].top,      (float)m_rcInfoRep[i].Width(),      (float)m_rcInfoRep[i].Height());
            Gdiplus::RectF v2((float)m_rcInfoTermVal[i].left,  (float)m_rcInfoTermVal[i].top,  (float)m_rcInfoTermVal[i].Width(),  (float)m_rcInfoTermVal[i].Height());

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
            CString n; n.Format(_T("%d"), i + 1);
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

    // Draw page indicator in navigation bar
    if (!m_rcNavBar.IsRectEmpty())
    {
        CString pageStr;
        pageStr.Format(_T("%d / %d"), m_nCurrentPage + 1, kTotalPages);
        const float navCX = (float)(m_rcNavBar.left + m_rcNavBar.right) / 2.f;
        const float navCY = (float)(m_rcNavBar.top + m_rcNavBar.bottom) / 2.f;
        const float pw = (float)ModernUIDpi::Scale(m_hWnd, 80);
        const float ph = (float)ModernUIDpi::Scale(m_hWnd, 28);
        Gdiplus::RectF navRF(navCX - pw/2.f, navCY - ph/2.f, pw, ph);
        Gdiplus::StringFormat sfNav;
        sfNav.SetAlignment(Gdiplus::StringAlignmentCenter);
        sfNav.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        Gdiplus::SolidBrush brNav(Gdiplus::Color(255, 60, 70, 90));
#ifdef UNICODE
        g.DrawString(pageStr, -1, m_pFontValBold, navRF, &sfNav, &brNav);
#else
        CStringW wPage(pageStr);
        g.DrawString(wPage, -1, m_pFontValBold, navRF, &sfNav, &brNav);
#endif
    }

    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

// ============================================================================
// Card/Control background sync (UnderlayColor)
// ============================================================================
COLORREF CShopDownDlg::GetRowCardBg(int index) const
{
    // OnPaint()와 동일한 규칙을 사용해야 화면과 컨트롤 underlay가 일치한다.
    // Normal: white, Pending/Empty: very light blue tint (quiet)
    if (index < 0 || index >= kRowCount) return RGB(255, 255, 255);

    CString rep;
    if (m_pRetailName[index]) rep = *m_pRetailName[index];
    const bool hasRep = !rep.IsEmpty();
    return hasRep ? RGB(255, 255, 255) : RGB(246, 248, 251);
}

void CShopDownDlg::ApplyRowUnderlay(int index, BOOL bRedraw /*= TRUE*/)
{
    if (index < 0 || index >= kRowCount) return;

    const COLORREF cr = GetRowCardBg(index);

    if (m_editProd[index].GetSafeHwnd()) m_editProd[index].SetUnderlayColor(cr);
    if (m_editBiz[index].GetSafeHwnd())  m_editBiz[index].SetUnderlayColor(cr);
    if (m_editPwd[index].GetSafeHwnd())  m_editPwd[index].SetUnderlayColor(cr);

    if (m_btnDownload[index].GetSafeHwnd()) m_btnDownload[index].SetUnderlayColor(cr);

    
    if (m_btnDelete[index].GetSafeHwnd())  m_btnDelete[index].SetUnderlayColor(cr);
    if (bRedraw)
    {
        if (m_editProd[index].GetSafeHwnd())    m_editProd[index].Invalidate(FALSE);
        if (m_editBiz[index].GetSafeHwnd())     m_editBiz[index].Invalidate(FALSE);
        if (m_editPwd[index].GetSafeHwnd())     m_editPwd[index].Invalidate(FALSE);
        if (m_btnDownload[index].GetSafeHwnd()) m_btnDownload[index].Invalidate(FALSE);
        if (m_btnDelete[index].GetSafeHwnd())   m_btnDelete[index].Invalidate(FALSE);
    }
}

void CShopDownDlg::ApplyAllRowUnderlays()
{
    for (int i = 0; i < kRowCount; ++i)
        ApplyRowUnderlay(i, TRUE);
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
void CShopDownDlg::OnPrevPageClick()
{
    if (m_nCurrentPage > 0) { m_nCurrentPage--; LayoutControls(); }
}

void CShopDownDlg::OnNextPageClick()
{
    if (m_nCurrentPage < kTotalPages - 1) { m_nCurrentPage++; LayoutControls(); }
}

void CShopDownDlg::UpdatePageButtons()
{
    if (m_btnPrevPage.GetSafeHwnd())
        m_btnPrevPage.EnableWindow(m_nCurrentPage > 0);
    if (m_btnNextPage.GetSafeHwnd())
        m_btnNextPage.EnableWindow(m_nCurrentPage < kTotalPages - 1);
}
