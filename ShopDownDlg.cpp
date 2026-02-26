#include "stdafx.h"
#include "ShopDownDlg.h"
#include <string>

IMPLEMENT_DYNAMIC(CShopDownDlg, CDialog)

BEGIN_MESSAGE_MAP(CShopDownDlg, CDialog)
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
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
        idxW     = ModernUIDpi::Scale(hwnd, 34);
editGap  = ModernUIDpi::Scale(hwnd, 10);
    labelH   = ModernUIDpi::Scale(hwnd, 18);
    labelGap = ModernUIDpi::Scale(hwnd, 6);
    infoLineH= ModernUIDpi::Scale(hwnd, 22);

        btnW = ModernUIDpi::Scale(hwnd, 92);
            btnH = ModernUIDpi::Scale(hwnd, 42);

    // item card available inner width
    // (section margin is handled by caller; here we just decide the split ratio)
    int innerW = clientWidth - cardM * 2 - cardPad * 2 - idxW;

    // left column: fixed-ish, but clamp for smaller windows
    int leftMin = ModernUIDpi::Scale(hwnd, 360);
        int leftMax = ModernUIDpi::Scale(hwnd, 420);
int idealLeft = (int)(innerW * 0.52);
    if (idealLeft < leftMin) idealLeft = leftMin;
    if (idealLeft > leftMax) idealLeft = leftMax;

    leftW  = idealLeft;
    rightW = innerW - leftW - colGap;
    if (rightW < ModernUIDpi::Scale(hwnd, 220))
    {
        // fallback: squeeze left a bit if right is too small
        int need = ModernUIDpi::Scale(hwnd, 220) - rightW;
        leftW = max(leftMin, leftW - need);
        rightW = innerW - leftW - colGap;
    }
}


// ============================================================================
// Construction / Destruction
// ============================================================================
CShopDownDlg::CShopDownDlg(CWnd* pParent)
    : CDialog(CShopDownDlg::IDD, pParent)
    , m_nScrollPos(0)
    , m_nTotalContentH(0)
    , m_nViewH(0)
    , m_nPendingScrollPos(-1)
    , m_bScrollTimerActive(FALSE){
    InitPointerArrays();
}

CShopDownDlg::~CShopDownDlg()
{
    if (m_bScrollTimerActive)
    {
        KillTimer(kScrollTimerId);
        m_bScrollTimerActive = FALSE;
    }
    if (m_brushBg.GetSafeHandle())   m_brushBg.DeleteObject();
    if (m_brushCard.GetSafeHandle()) m_brushCard.DeleteObject();
}

void CShopDownDlg::InitPointerArrays()
{
    m_pPrdid[0]=&m_prdid1;  m_pPrdid[1]=&m_prdid2;  m_pPrdid[2]=&m_prdid3;  m_pPrdid[3]=&m_prdid4;  m_pPrdid[4]=&m_prdid5;
    m_pPrdid[5]=&m_prdid6;  m_pPrdid[6]=&m_prdid7;  m_pPrdid[7]=&m_prdid8;  m_pPrdid[8]=&m_prdid9;  m_pPrdid[9]=&m_prdid10;
    m_pPrdid[10]=&m_prdid11; m_pPrdid[11]=&m_prdid12; m_pPrdid[12]=&m_prdid13; m_pPrdid[13]=&m_prdid14; m_pPrdid[14]=&m_prdid15;
    m_pPrdid[15]=&m_prdid16; m_pPrdid[16]=&m_prdid17; m_pPrdid[17]=&m_prdid18; m_pPrdid[18]=&m_prdid19; m_pPrdid[19]=&m_prdid20;
    m_pPrdid[20]=&m_prdid21; m_pPrdid[21]=&m_prdid22; m_pPrdid[22]=&m_prdid23; m_pPrdid[23]=&m_prdid24; m_pPrdid[24]=&m_prdid25;

    m_pRegno[0]=&m_regno1;  m_pRegno[1]=&m_regno2;  m_pRegno[2]=&m_regno3;  m_pRegno[3]=&m_regno4;  m_pRegno[4]=&m_regno5;
    m_pRegno[5]=&m_regno6;  m_pRegno[6]=&m_regno7;  m_pRegno[7]=&m_regno8;  m_pRegno[8]=&m_regno9;  m_pRegno[9]=&m_regno10;
    m_pRegno[10]=&m_regno11; m_pRegno[11]=&m_regno12; m_pRegno[12]=&m_regno13; m_pRegno[13]=&m_regno14; m_pRegno[14]=&m_regno15;
    m_pRegno[15]=&m_regno16; m_pRegno[16]=&m_regno17; m_pRegno[17]=&m_regno18; m_pRegno[18]=&m_regno19; m_pRegno[19]=&m_regno20;
    m_pRegno[20]=&m_regno21; m_pRegno[21]=&m_regno22; m_pRegno[22]=&m_regno23; m_pRegno[23]=&m_regno24; m_pRegno[24]=&m_regno25;

    m_pPasswd[0]=&m_passwd1;  m_pPasswd[1]=&m_passwd2;  m_pPasswd[2]=&m_passwd3;  m_pPasswd[3]=&m_passwd4;  m_pPasswd[4]=&m_passwd5;
    m_pPasswd[5]=&m_passwd6;  m_pPasswd[6]=&m_passwd7;  m_pPasswd[7]=&m_passwd8;  m_pPasswd[8]=&m_passwd9;  m_pPasswd[9]=&m_passwd10;
    m_pPasswd[10]=&m_passwd11; m_pPasswd[11]=&m_passwd12; m_pPasswd[12]=&m_passwd13; m_pPasswd[13]=&m_passwd14; m_pPasswd[14]=&m_passwd15;
    m_pPasswd[15]=&m_passwd16; m_pPasswd[16]=&m_passwd17; m_pPasswd[17]=&m_passwd18; m_pPasswd[18]=&m_passwd19; m_pPasswd[19]=&m_passwd20;
    m_pPasswd[20]=&m_passwd21; m_pPasswd[21]=&m_passwd22; m_pPasswd[22]=&m_passwd23; m_pPasswd[23]=&m_passwd24; m_pPasswd[24]=&m_passwd25;

    m_pRetailName[0]=&m_retail_name1;  m_pRetailName[1]=&m_retail_name2;  m_pRetailName[2]=&m_retail_name3;  m_pRetailName[3]=&m_retail_name4;  m_pRetailName[4]=&m_retail_name5;
    m_pRetailName[5]=&m_retail_name6;  m_pRetailName[6]=&m_retail_name7;  m_pRetailName[7]=&m_retail_name8;  m_pRetailName[8]=&m_retail_name9;  m_pRetailName[9]=&m_retail_name10;
    m_pRetailName[10]=&m_retail_name11; m_pRetailName[11]=&m_retail_name12; m_pRetailName[12]=&m_retail_name13; m_pRetailName[13]=&m_retail_name14; m_pRetailName[14]=&m_retail_name15;
    m_pRetailName[15]=&m_retail_name16; m_pRetailName[16]=&m_retail_name17; m_pRetailName[17]=&m_retail_name18; m_pRetailName[18]=&m_retail_name19; m_pRetailName[19]=&m_retail_name20;
    m_pRetailName[20]=&m_retail_name21; m_pRetailName[21]=&m_retail_name22; m_pRetailName[22]=&m_retail_name23; m_pRetailName[23]=&m_retail_name24; m_pRetailName[24]=&m_retail_name25;

    m_pSecondName[0]=&m_second_name1;  m_pSecondName[1]=&m_second_name2;  m_pSecondName[2]=&m_second_name3;  m_pSecondName[3]=&m_second_name4;  m_pSecondName[4]=&m_second_name5;
    m_pSecondName[5]=&m_second_name6;  m_pSecondName[6]=&m_second_name7;  m_pSecondName[7]=&m_second_name8;  m_pSecondName[8]=&m_second_name9;  m_pSecondName[9]=&m_second_name10;
    m_pSecondName[10]=&m_second_name11; m_pSecondName[11]=&m_second_name12; m_pSecondName[12]=&m_second_name13; m_pSecondName[13]=&m_second_name14; m_pSecondName[14]=&m_second_name15;
    m_pSecondName[15]=&m_second_name16; m_pSecondName[16]=&m_second_name17; m_pSecondName[17]=&m_second_name18; m_pSecondName[18]=&m_second_name19; m_pSecondName[19]=&m_second_name20;
    m_pSecondName[20]=&m_second_name21; m_pSecondName[21]=&m_second_name22; m_pSecondName[22]=&m_second_name23; m_pSecondName[23]=&m_second_name24; m_pSecondName[24]=&m_second_name25;
}

// ============================================================================
// OnInitDialog
// ============================================================================
BOOL CShopDownDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    ModernUIGfx::EnsureGdiplusStartup();

    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    m_brushBg.CreateSolidBrush(RGB(250, 251, 253));
    m_brushCard.CreateSolidBrush(RGB(255, 255, 255));
    m_brushCardDisabled.CreateSolidBrush(RGB(245, 246, 248));

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
    if (m_editProd[0].GetSafeHwnd()) return;

    const DWORD edtBase  = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS
                         | ES_AUTOHSCROLL | ES_CENTER;
    const DWORD edtStyle = edtBase;                    // 입력 가능 (prod, biz)
    const DWORD pwdStyle = edtBase | ES_PASSWORD;      // 입력 가능 (pwd)
    const DWORD roStyle  = edtBase | ES_READONLY;      // 읽기전용 (name, etc)

    struct { LPCTSTR prod; LPCTSTR biz; LPCTSTR name; } data[25] = {
        {_T("K074404214"), _T("1050844729"), _T("금융결제원 테스트")},

    };

    // ShopDownDlg는 행(row) 배경을 OnPaint()에서 직접 그리므로
    // WM_CTLCOLOR로 "진짜 배경"을 자동 추출할 수 없습니다.
    // 따라서 행 parity(짝/홀)에 맞춰 UnderlayColor를 강제 지정합니다.
    const COLORREF crRowBg = RGB(255, 255, 255); // item card bg

    for (int i = 0; i < kRowCount; ++i)
    {
m_editProd[i].CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
            edtStyle, CRect(0,0,10,10), this, 62000+(i*10)+1);
        m_editProd[i].SetUnderlayColor(crRowBg);

        m_editBiz[i].CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
            edtStyle, CRect(0,0,10,10), this, 62000+(i*10)+2);
        m_editBiz[i].SetUnderlayColor(crRowBg);

        m_editPwd[i].CreateEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
            pwdStyle, CRect(0,0,10,10), this, 62000+(i*10)+3);
        m_editPwd[i].SetUnderlayColor(crRowBg);

        m_btnDownload[i].Create(_T("다운로드"),
            WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS|BS_OWNERDRAW,
            CRect(0,0,10,10), this, kBtnBase+i);
        m_btnDownload[i].SetColors(KFTC_PRIMARY, KFTC_PRIMARY_HOVER, RGB(255,255,255));
        m_btnDownload[i].SetUnderlayColor(crRowBg);

        m_editMerchantName[i].CreateEx(0, _T("EDIT"), _T(""),
            roStyle, CRect(0,0,10,10), this, 62000+(i*10)+4);
        m_editMerchantName[i].SetUnderlayColor(crRowBg);

        m_editEtc[i].CreateEx(0, _T("EDIT"), _T(""),
            roStyle, CRect(0,0,10,10), this, 62000+(i*10)+5);
        m_editEtc[i].SetUnderlayColor(crRowBg);

        m_editMerchantName[i].ShowWindow(SW_HIDE);
        m_editEtc[i].ShowWindow(SW_HIDE);

        *m_pPrdid[i]      = data[i].prod;
        *m_pRegno[i]      = data[i].biz;
        *m_pRetailName[i] = data[i].name;
        *m_pSecondName[i] = _T("");

        m_editProd[i].SetWindowText(*m_pPrdid[i]);
        m_editBiz[i].SetWindowText(*m_pRegno[i]);
    }
}

// ============================================================================
// ApplyFonts
// ============================================================================
void CShopDownDlg::ApplyFonts()
{
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
void CShopDownDlg::LayoutControls()
{
    static BOOL s_bInLayout = FALSE;
    if (s_bInLayout) return;
    s_bInLayout = TRUE;

    CRect rc;
    GetClientRect(&rc);

    m_card.Compute(rc.Width(), m_hWnd);

    const int rowGap = ModernUIDpi::Scale(m_hWnd, kRowGap);
    const int ctrlH  = ModernUIDpi::Scale(m_hWnd, kCtrlH);

    // Make ~2 cards fit in the visible area (larger typography to match other tabs).
    const int padY   = ModernUIDpi::Scale(m_hWnd, 8);
    const int availH = max(0, rc.Height() - padY * 2);
    int rowH         = (availH - rowGap) / 2; // 2 rows, 1 gap

    // Clamp to keep controls readable and consistent across DPI.
    const int rowMin = ModernUIDpi::Scale(m_hWnd, 192);
    const int rowMax = ModernUIDpi::Scale(m_hWnd, 240);
    if (rowH < rowMin) rowH = rowMin;
    if (rowH > rowMax) rowH = rowMax;

    const int contentStartY = padY; // header is drawn by parent (ShopSetupDlg)

    m_nTotalContentH = padY * 2 + kRowCount * (rowH + rowGap) - rowGap;
    m_nViewH         = rc.Height();

    int y = contentStartY - m_nScrollPos;

    // prod/biz/pwd/button + (merchant/term edits are kept but always hidden)
    HDWP hdwp = ::BeginDeferWindowPos(kRowCount * 6);

    for (int i = 0; i < kRowCount; ++i)
    {
                // Reserve a right gutter when the vertical scrollbar is visible
        const bool hasVScroll = (m_nTotalContentH > m_nViewH);
        const int sbW = hasVScroll ? ::GetSystemMetrics(SM_CXVSCROLL) : 0;
        const int rightGutter = hasVScroll ? (sbW + ModernUIDpi::Scale(m_hWnd, 12)) : 0;
        m_rcRow[i].SetRect(m_card.cardM, y, rc.right - m_card.cardM - rightGutter, y + rowH);

        // viewport clip: section card body
        const int viewTop    = contentStartY;
        const int viewBottom = rc.Height();

        const bool bInView = (m_rcRow[i].bottom > viewTop) && (m_rcRow[i].top < viewBottom);

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

        // button (align with password row to keep a clean grid and avoid
        // stealing space from the 대표 가맹점 line when it is long)
        const int btnY = y2Edit; // align with "비밀번호" edit/value baseline
        m_rcBtn[i].SetRect(xR + m_card.rightW - m_card.btnW, btnY, xR + m_card.rightW, btnY + m_card.btnH);

        // info text rectangles (value line area)
        const int yInfo1 = inner.top + labelH + labelGap;
        m_rcInfoRep[i].SetRect(xR, yInfo1, xR + m_card.rightW, yInfo1 + m_card.infoLineH);

        // Reserve space for the button on the 2nd line so the right-side value
        // does not run underneath the CTA.
        const int gapBtn = ModernUIDpi::Scale(m_hWnd, 12);
        const int yInfo2 = y2Edit; // align with password edit
        m_rcInfoTerm[i].SetRect(xR, yInfo2, m_rcBtn[i].left - gapBtn, yInfo2 + m_card.infoLineH);

        // Enabled/disabled visual states (match other tabs)
        {
            const BOOL enProd = ::IsWindowEnabled(m_editProd[i].m_hWnd);
            const BOOL enBiz  = ::IsWindowEnabled(m_editBiz[i].m_hWnd);
            const BOOL enPwd  = ::IsWindowEnabled(m_editPwd[i].m_hWnd);
            const COLORREF crEdEn  = RGB(255,255,255);
            const COLORREF crEdDis = RGB(245,246,248);
            m_editProd[i].SetUnderlayColor(enProd ? crEdEn : crEdDis);
            m_editBiz[i].SetUnderlayColor (enBiz  ? crEdEn : crEdDis);
            m_editPwd[i].SetUnderlayColor (enPwd  ? crEdEn : crEdDis);

            const BOOL enBtn = ::IsWindowEnabled(m_btnDownload[i].m_hWnd);
            if (enBtn)
                m_btnDownload[i].SetColors(KFTC_PRIMARY, KFTC_PRIMARY_HOVER, RGB(255,255,255));
            else
                m_btnDownload[i].SetColors(RGB(235,237,240), RGB(235,237,240), RGB(160,165,175));
        }

        const UINT baseFlags = SWP_NOZORDER | SWP_NOACTIVATE;
        const UINT showHide  = bInView ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;

        if (hdwp)
        {
            hdwp = ::DeferWindowPos(hdwp, m_editProd[i].m_hWnd,    NULL, m_rcProd[i].left, m_rcProd[i].top, m_rcProd[i].Width(), m_rcProd[i].Height(), baseFlags | showHide);
            hdwp = ::DeferWindowPos(hdwp, m_editBiz[i].m_hWnd,     NULL, m_rcBiz[i].left,  m_rcBiz[i].top,  m_rcBiz[i].Width(),  m_rcBiz[i].Height(),  baseFlags | showHide);
            hdwp = ::DeferWindowPos(hdwp, m_editPwd[i].m_hWnd,     NULL, m_rcPwd[i].left,  m_rcPwd[i].top,  m_rcPwd[i].Width(),  m_rcPwd[i].Height(),  baseFlags | showHide);
            hdwp = ::DeferWindowPos(hdwp, m_btnDownload[i].m_hWnd, NULL, m_rcBtn[i].left,  m_rcBtn[i].top,  m_rcBtn[i].Width(),  m_rcBtn[i].Height(),  baseFlags | showHide);

            // keep these hidden (we draw text ourselves for a cleaner "info" look)
            hdwp = ::DeferWindowPos(hdwp, m_editMerchantName[i].m_hWnd, NULL, 0,0,0,0, baseFlags | SWP_HIDEWINDOW);
            hdwp = ::DeferWindowPos(hdwp, m_editEtc[i].m_hWnd,          NULL, 0,0,0,0, baseFlags | SWP_HIDEWINDOW);
        }

        y += rowH + rowGap;
    }

    if (hdwp) ::EndDeferWindowPos(hdwp);

    ::RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);

    s_bInLayout = FALSE;
    UpdateScrollBar();
}


// ============================================================================
// UpdateScrollBar
// ============================================================================
void CShopDownDlg::UpdateScrollBar()
{
    if (m_nTotalContentH <= m_nViewH)
    {
        ShowScrollBar(SB_VERT, FALSE);
        return;
    }
    ShowScrollBar(SB_VERT, TRUE);

    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_ALL;
    si.nMin   = 0;
    si.nMax   = m_nTotalContentH - 1;
    si.nPage  = m_nViewH;
    si.nPos   = m_nScrollPos;
    SetScrollInfo(SB_VERT, &si, TRUE);
}

// ============================================================================
// ApplyScroll  - single entry point for all scroll position changes
// ============================================================================
void CShopDownDlg::ApplyScroll(int newPos)
{
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_ALL;
    GetScrollInfo(SB_VERT, &si);

    const int maxPos = max(0, (int)(si.nMax - (int)si.nPage + 1));
    newPos = max(0, min(newPos, maxPos));
    if (newPos == m_nScrollPos) return;

    m_nScrollPos = newPos;
    si.fMask = SIF_POS;
    si.nPos  = m_nScrollPos;
    SetScrollInfo(SB_VERT, &si, TRUE);

    LayoutControls();
    // LayoutControls()가 RedrawWindow로 무효화/자식까지 갱신합니다.
    // 여기서 UpdateWindow()를 강제 호출하면 스크롤/타이머 중첩으로
    // 재진입(Paint) 가능성이 있어 호출하지 않습니다.
}

// ============================================================================
// OnVScroll
// ============================================================================
// ============================================================================
// QueueScroll - throttle scroll updates via timer (shared by wheel & thumbtrack)
// ============================================================================
void CShopDownDlg::QueueScroll(int newPos)
{
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_ALL;
    GetScrollInfo(SB_VERT, &si);

    int maxPos = 0;
    if (si.nPage > 0)
        maxPos = max(0, (int)si.nMax - (int)si.nPage + 1);
    else
        maxPos = max(0, (int)si.nMax);

    if (newPos < 0) newPos = 0;
    if (newPos > maxPos) newPos = maxPos;

    m_nPendingScrollPos = newPos;

    // keep scrollbar thumb responsive immediately
    si.fMask = SIF_POS;
    si.nPos  = newPos;
    SetScrollInfo(SB_VERT, &si, TRUE);

    if (!m_bScrollTimerActive)
    {
        SetTimer(kScrollTimerId, kScrollTimerMs, NULL);
        m_bScrollTimerActive = TRUE;
    }
}

void CShopDownDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_ALL;
    GetScrollInfo(SB_VERT, &si);

    const int lineStep = ModernUIDpi::Scale(m_hWnd, kRowH + kRowGap);
    int newPos = si.nPos;

    switch (nSBCode)
    {
    case SB_LINEUP:        newPos -= lineStep;      break;
    case SB_LINEDOWN:      newPos += lineStep;      break;
    case SB_PAGEUP:        newPos -= (int)si.nPage; break;
    case SB_PAGEDOWN:      newPos += (int)si.nPage; break;

    case SB_THUMBTRACK:
        m_nPendingScrollPos = (int)nPos;
        if (!m_bScrollTimerActive)
        {
            SetTimer(kScrollTimerId, kScrollTimerMs, NULL);
            m_bScrollTimerActive = TRUE;
        }
        si.fMask = SIF_POS; si.nPos = (int)nPos;
        SetScrollInfo(SB_VERT, &si, TRUE);
        CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
        return;

    case SB_THUMBPOSITION:
        if (m_bScrollTimerActive) { KillTimer(kScrollTimerId); m_bScrollTimerActive = FALSE; }
        newPos = (int)nPos;
        break;

    case SB_ENDSCROLL:
        if (m_bScrollTimerActive)
        {
            KillTimer(kScrollTimerId);
            m_bScrollTimerActive = FALSE;
            if (m_nPendingScrollPos >= 0) { ApplyScroll(m_nPendingScrollPos); m_nPendingScrollPos = -1; }
        }
        CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
        return;

    default:
        CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
        return;
    }

    ApplyScroll(newPos);
    CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CShopDownDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == kScrollTimerId)
    {
        // THUMBTRACK 동안만 타이머를 잠깐 돌려 과도한 Layout/Paint를 막는다.
        // 적용할 값이 없으면 즉시 타이머를 정지한다(유휴 상태에서 WM_TIMER 폭주 방지).
        if (m_nPendingScrollPos >= 0)
        {
            ApplyScroll(m_nPendingScrollPos);
            m_nPendingScrollPos = -1;
        }
        if (m_bScrollTimerActive)
        {
            KillTimer(kScrollTimerId);
            m_bScrollTimerActive = FALSE;
        }
        return; // base class로 넘길 필요 없음
    }
    CDialog::OnTimer(nIDEvent);
}

// ============================================================================
// PreTranslateMessage
// WM_MOUSEWHEEL은 커서 아래 윈도우(자식 컨트롤)로 직접 전달되어
// 부모 OnMouseWheel에 도달하지 않는다.
// PreTranslateMessage에서 가로채 부모(this)가 직접 처리한다.
// ============================================================================
BOOL CShopDownDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_MOUSEWHEEL)
    {
        // 메시지 수신 윈도우가 자신이거나 자식 컨트롤인 경우 모두 처리
        if (pMsg->hwnd == m_hWnd || ::IsChild(m_hWnd, pMsg->hwnd))
        {
            short zDelta = (short)HIWORD(pMsg->wParam);
            const int step  = ModernUIDpi::Scale(m_hWnd, kRowH + kRowGap);
            const int lines = zDelta / WHEEL_DELTA;
            SCROLLINFO si;
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask  = SIF_ALL;
            GetScrollInfo(SB_VERT, &si);
            QueueScroll(si.nPos - lines * step);
            return TRUE;  // 자식 컨트롤에 전달하지 않음
        }
    }
    return CDialog::PreTranslateMessage(pMsg);
}

// ============================================================================
BOOL CShopDownDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    const int step  = ModernUIDpi::Scale(m_hWnd, kRowH + kRowGap);
    const int lines = zDelta / WHEEL_DELTA;
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_ALL;
    GetScrollInfo(SB_VERT, &si);
    QueueScroll(si.nPos - lines * step);
    return TRUE;
}

// ============================================================================
// OnMouseMove / OnMouseLeave  (hover row highlight)
// ============================================================================


// ============================================================================
// Commands / events
// ============================================================================
BOOL CShopDownDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT nID   = LOWORD(wParam);
    UINT nCode = HIWORD(wParam);
    if (nCode == BN_CLICKED && nID >= kBtnBase && nID < kBtnBase + kRowCount)
    {
        OnDownloadClick(nID - kBtnBase);
        return TRUE;
    }
    return CDialog::OnCommand(wParam, lParam);
}

void CShopDownDlg::OnDownloadClick(int index)
{
    CString msg;
    msg.Format(_T("가맹점%d 다운로드"), index + 1);
    AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
}

BOOL CShopDownDlg::OnEraseBkgnd(CDC*) { return TRUE; }

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


void CShopDownDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    if (cx <= 0 || cy <= 0) return;
    if (m_editProd[0].GetSafeHwnd()) LayoutControls();
    Invalidate(FALSE);
}

// ============================================================================
// OnPaint  - full double-buffered render
// ============================================================================
void CShopDownDlg::OnPaint()
{
    CPaintDC dc(this);
    ModernUIGfx::EnsureGdiplusStartup();

    CRect rc;
    GetClientRect(&rc);

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap memBmp;
    memBmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&memBmp);

    memDC.FillSolidRect(&rc, RGB(250, 251, 253));

    Gdiplus::Graphics g(memDC.m_hDC);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    auto RR = [](Gdiplus::GraphicsPath& path, Gdiplus::RectF r, float rad)
    {
        if (rad <= 0.f) { path.AddRectangle(r); return; }
        const float d = rad * 2.f;
        Gdiplus::RectF a(r.X, r.Y, d, d);
        path.AddArc(a, 180, 90); a.X = r.X + r.Width - d;
        path.AddArc(a, 270, 90); a.Y = r.Y + r.Height - d;
        path.AddArc(a,   0, 90); a.X = r.X;
        path.AddArc(a,  90, 90); path.CloseFigure();
    };

    // ----------------------------------------------------------------
    // Section header/card background is drawn by parent dialog (ShopSetupDlg)
    // Here we only draw item cards and labels.
    // ----------------------------------------------------------------

    const float clipTop    = 0.f;
    const float clipBottom = (float)rc.Height();

    // ----------------------------------------------------------------
    // Text helpers
    // ----------------------------------------------------------------
    Gdiplus::FontFamily ff(L"Malgun Gothic");
    const float lblPx = (float)ModernUIDpi::Scale(m_hWnd, 13);
    const float valPx = (float)ModernUIDpi::Scale(m_hWnd, 13);

    Gdiplus::Font fLbl(&ff, lblPx, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::Font fVal(&ff, valPx, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::Font fValBold(&ff, valPx, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);

    auto DrawEllips = [&](const CString& s, const Gdiplus::RectF& r, Gdiplus::Font* f, const Gdiplus::Color& c)
    {
        if (s.IsEmpty()) return;
        Gdiplus::SolidBrush br(c);
        Gdiplus::StringFormat sf;
        sf.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
        sf.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
                sf.SetLineAlignment(Gdiplus::StringAlignmentNear);
        sf.SetAlignment(Gdiplus::StringAlignmentNear);

#ifdef UNICODE
        g.DrawString(s, -1, f, r, &sf, &br);
#else
        // CP949 build: CString is ANSI, convert to wide
        CStringW ws(s);
        g.DrawString(ws, -1, f, r, &sf, &br);
#endif
    };

    const Gdiplus::Color cLbl(255, 120, 128, 142);
    const Gdiplus::Color cVal(255, 24, 28, 35);
    const Gdiplus::Color cPh (255, 160, 165, 175);

    // ----------------------------------------------------------------
    // 3) Item cards + labels + right-side info (drawn)
    // ----------------------------------------------------------------
    for (int i = 0; i < kRowCount; ++i)
    {
        const CRect& r = m_rcRow[i];
        if (r.bottom <= clipTop || r.top >= clipBottom) continue;

        // Data-driven card tone:
        // - 대표 가맹점(다운로드 결과)이 있으면 정상 카드(white)
        // - 없으면(미다운로드/조회 실패/미존재 등) 살짝 톤을 주어 구분
        CString rep  = *m_pRetailName[i];
        CString term = *m_pSecondName[i];
        const bool hasRep = !rep.IsEmpty();

        // item card
        Gdiplus::RectF rf((float)r.left, (float)r.top, (float)r.Width(), (float)r.Height());
        Gdiplus::GraphicsPath rp;
        RR(rp, rf, 12.f);

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
            DrawEllips(_T("단말기 제품번호"), lp, &fLbl, cLbl);
            DrawEllips(_T("사업자번호"),     lb, &fLbl, cLbl);

            // pwd label
            const float yPwdLbl = (float)m_rcPwd[i].top - (float)m_card.labelGap - (float)m_card.labelH;
            Gdiplus::RectF lpwd((float)m_rcPwd[i].left, yPwdLbl, (float)m_rcPwd[i].Width(), hLbl);
            DrawEllips(_T("비밀번호"), lpwd, &fLbl, cLbl);
        }

        // right labels + values
        {
            const float yLbl = (float)r.top + (float)m_card.cardPad;
            const float hLbl = (float)m_card.labelH;

            // label rects align with value rects
            Gdiplus::RectF l1((float)m_rcInfoRep[i].left,  yLbl, (float)m_rcInfoRep[i].Width(), hLbl);
            DrawEllips(_T("대표 가맹점"), l1, &fLbl, cLbl);

            const float y2Lbl = (float)m_rcInfoTerm[i].top - (float)m_card.labelGap - (float)m_card.labelH;
            Gdiplus::RectF l2((float)m_rcInfoTerm[i].left, y2Lbl, (float)m_rcInfoTerm[i].Width(), hLbl);
            DrawEllips(_T("단말기별 가맹점"), l2, &fLbl, cLbl);

            CString rep  = *m_pRetailName[i];
            CString term = *m_pSecondName[i];

            // value rects
            Gdiplus::RectF v1((float)m_rcInfoRep[i].left,  (float)m_rcInfoRep[i].top,  (float)m_rcInfoRep[i].Width(),  (float)m_rcInfoRep[i].Height());
            Gdiplus::RectF v2((float)m_rcInfoTerm[i].left, (float)m_rcInfoTerm[i].top, (float)m_rcInfoTerm[i].Width(), (float)m_rcInfoTerm[i].Height());

            DrawEllips(rep,  v1, &fValBold, cVal);
            DrawEllips(term, v2, &fVal,     cVal);
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
            g.DrawString(n, -1, &fValBold, nr, &sf, &br);
#else
            CStringW wn(n);
            g.DrawString(wn, -1, &fValBold, nr, &sf, &br);
#endif
        }
    }

    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}

