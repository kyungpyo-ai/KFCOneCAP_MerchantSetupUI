#include "stdafx.h"
#include "resource.h"
#include "KFTCOneCAPDlg.h"
#include "ReaderSetupDlg.h"
#include "ShopSetupDlg.h"

// Release 쑴니메이의하 다이아로그 오핀 지연 메시지
// Poll until the pressed card button finishes its release animation, then open
#define kTimerWaitRelease 201

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

namespace
{
    const UINT_PTR kCardAnimTimerId = 1;
    Gdiplus::Color g_cardIconColor(255, 31, 114, 214);
}

CHomeCardButton::CHomeCardButton()
    : m_bHover(FALSE)
    , m_bPressed(FALSE)
    , m_bTracking(FALSE)
    , m_nHoverProgress(0)
    , m_nPressProgress(0)
{
}

BEGIN_MESSAGE_MAP(CHomeCardButton, CButton)
    ON_WM_MOUSEMOVE()
    ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_CAPTURECHANGED()
    ON_WM_CANCELMODE()
    ON_WM_TIMER()
END_MESSAGE_MAP()

void CHomeCardButton::ResetVisualState()
{
    m_bHover = FALSE;
    m_bPressed = FALSE;
    m_bTracking = FALSE;
    m_nHoverProgress = 0;
    m_nPressProgress = 0;
    KillTimer(kCardAnimTimerId);
    if (::IsWindow(m_hWnd))
        Invalidate(FALSE);
}

void CHomeCardButton::StartTrackMouseLeave()
{
    if (m_bTracking)
        return;

    TRACKMOUSEEVENT tme = { 0 };
    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = m_hWnd;
    if (_TrackMouseEvent(&tme))
        m_bTracking = TRUE;
}

void CHomeCardButton::StartAnimTimer()
{
    if (::IsWindow(m_hWnd))
        SetTimer(kCardAnimTimerId, 10, NULL);
}

void CHomeCardButton::StopAnimTimerIfIdle()
{
    const int nHoverTarget = m_bHover ? 100 : 0;
    const int nPressTarget = m_bPressed ? 100 : 0;

    if (m_nHoverProgress == nHoverTarget && m_nPressProgress == nPressTarget)
        KillTimer(kCardAnimTimerId);
}

void CHomeCardButton::StepAnimation()
{
    const int nHoverTarget = m_bHover ? 100 : 0;
    const int nPressTarget = m_bPressed ? 100 : 0;
    BOOL bChanged = FALSE;

    if (m_nHoverProgress < nHoverTarget)
    {
        m_nHoverProgress = min(100, m_nHoverProgress + 14);  // hover in ~70ms
        bChanged = TRUE;
    }
    else if (m_nHoverProgress > nHoverTarget)
    {
        m_nHoverProgress = max(0, m_nHoverProgress - 10);   // hover out ~100ms
        bChanged = TRUE;
    }

    if (m_nPressProgress < nPressTarget)
    {
        m_nPressProgress = min(100, m_nPressProgress + 34);
        bChanged = TRUE;
    }
    else if (m_nPressProgress > nPressTarget)
    {
        // ease-out: 처음엔 빠르게, 끝으로 갈수록 부드럽게 감속 → 떨림 없이 자연스러운 복귀
        // exponential decay: step always shrinks proportionally, no plateau
        m_nPressProgress = m_nPressProgress * 70 / 100;
        if (m_nPressProgress < 2) m_nPressProgress = 0;
        bChanged = TRUE;
    }

    if (bChanged && ::IsWindow(m_hWnd))
        Invalidate(FALSE);

    StopAnimTimerIfIdle();
}

void CHomeCardButton::OnMouseMove(UINT nFlags, CPoint point)
{
    StartTrackMouseLeave();
    if (!m_bHover)
    {
        m_bHover = TRUE;
        StartAnimTimer();
    }
    CButton::OnMouseMove(nFlags, point);
}

LRESULT CHomeCardButton::OnMouseLeave(WPARAM, LPARAM)
{
    m_bTracking = FALSE;
    m_bHover = FALSE;
    m_bPressed = FALSE;
    StartAnimTimer();
    return 0;
}

void CHomeCardButton::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_bPressed = TRUE;
    StartAnimTimer();
    CButton::OnLButtonDown(nFlags, point);
}

void CHomeCardButton::OnLButtonUp(UINT nFlags, CPoint point)
{
    CRect rc;
    GetClientRect(&rc);
    m_bPressed = FALSE;
    m_bHover = rc.PtInRect(point);
    StartAnimTimer();
    CButton::OnLButtonUp(nFlags, point);
}

void CHomeCardButton::OnCaptureChanged(CWnd* pWnd)
{
    if (::IsWindow(m_hWnd))
    {
        CRect rc;
        GetClientRect(&rc);
        CPoint pt;
        ::GetCursorPos(&pt);
        ScreenToClient(&pt);
        m_bHover = rc.PtInRect(pt);
    }
    m_bPressed = FALSE;
    StartAnimTimer();
    CButton::OnCaptureChanged(pWnd);
}

void CHomeCardButton::OnCancelMode()
{
    m_bPressed = FALSE;
    StartAnimTimer();
    CButton::OnCancelMode();
}

void CHomeCardButton::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == kCardAnimTimerId)
    {
        StepAnimation();
        return;
    }
    CButton::OnTimer(nIDEvent);
}

namespace
{
    COLORREF kHomeBg = RGB(0xF8, 0xFA, 0xFC);
    COLORREF kCardBg = RGB(255, 255, 255);
    COLORREF kCardBorder = RGB(232, 236, 242);
    COLORREF kCardBorderHover = RGB(226, 232, 240);
    COLORREF kCardFillHover = RGB(255, 255, 255);
    COLORREF kCardFillPressed = RGB(210, 230, 255);   // 더 진한 파랑: 선명한 눌림감
    // kIconBg 계열: DrawCardIcon 내부 직접 보간으로 대체됨
    COLORREF kTitleText = RGB(25, 31, 40);      // 헤더 h1: #191F28
    COLORREF kCardTitleText = RGB(51, 61, 75);   // 카드 h3: #333D4B
    COLORREF kSubText = RGB(139, 149, 161);   // #8B95A1
    COLORREF kFooterDivider = RGB(220, 226, 234);

    void AddRoundRectPath(GraphicsPath& path, const RectF& rc, REAL radius)
    {
        REAL d = radius * 2.0f;
        path.AddArc(rc.X, rc.Y, d, d, 180.0f, 90.0f);
        path.AddArc(rc.GetRight() - d, rc.Y, d, d, 270.0f, 90.0f);
        path.AddArc(rc.GetRight() - d, rc.GetBottom() - d, d, d, 0.0f, 90.0f);
        path.AddArc(rc.X, rc.GetBottom() - d, d, d, 90.0f, 90.0f);
        path.CloseFigure();
    }
}

CKFTCOneCAPDlg::CKFTCOneCAPDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CKFTCOneCAPDlg::IDD, pParent)
    , m_bFontsReady(FALSE)
    , m_ePendingOpen(PENDING_NONE)
    , m_pLogoBitmap(NULL)
    , m_nFooterDividerY(0)
{
}

CKFTCOneCAPDlg::~CKFTCOneCAPDlg()
{
    if (m_pLogoBitmap != NULL)
    {
        delete m_pLogoBitmap;
        m_pLogoBitmap = NULL;
    }
}

void CKFTCOneCAPDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CKFTCOneCAPDlg, CDialog)
    ON_BN_CLICKED(IDC_READER_SETUP, OnReaderSetup)
    ON_BN_CLICKED(IDC_SHOP_SETUP, OnShopSetup)
    ON_BN_CLICKED(IDC_TRANS, OnTrans)
    ON_BN_CLICKED(IDC_RECEIPT_SETUP, OnReceiptSetup)
    ON_BN_CLICKED(IDC_MINIMIZE, OnMinimize)
    ON_BN_CLICKED(IDC_EXIT, OnExit)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_DRAWITEM()
    ON_WM_CTLCOLOR()
    ON_WM_CLOSE()
    ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CKFTCOneCAPDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    ModernUIGfx::EnsureGdiplusStartup();

    EnsureFonts();

    SetWindowText(_T("KFTCOneCAP3001"));

    if (m_brBackground.GetSafeHandle() != NULL)
        m_brBackground.DeleteObject();
    m_brBackground.CreateSolidBrush(kHomeBg);

    m_btnReaderCard.SubclassDlgItem(IDC_READER_SETUP, this);
    m_btnShopCard.SubclassDlgItem(IDC_SHOP_SETUP, this);
    m_btnTransCard.SubclassDlgItem(IDC_TRANS, this);
    m_btnReceiptCard.SubclassDlgItem(IDC_RECEIPT_SETUP, this);

    m_btnMinimize.SubclassDlgItem(IDC_MINIMIZE, this);
    m_btnExit.SubclassDlgItem(IDC_EXIT, this);

    m_btnMinimize.SetButtonStyle(ButtonStyle::Auto);
    m_btnExit.SetButtonStyle(ButtonStyle::Auto);
    m_btnMinimize.SetUnderlayColor(kHomeBg);
    m_btnExit.SetUnderlayColor(kHomeBg);
    // sys-btn: normal text=#8B95A1, hover bg=#E5E8EB, hover text=#333D4B
    m_btnMinimize.SetColors(kHomeBg, RGB(229, 232, 235), RGB(139, 149, 161));
    m_btnMinimize.SetHoverTextColor(RGB(51, 61, 75));
    m_btnExit.SetColors(kHomeBg, RGB(229, 232, 235), RGB(139, 149, 161));
    m_btnExit.SetHoverTextColor(RGB(51, 61, 75));

    m_btnMinimize.SetWindowText(_T("최소화"));
    m_btnExit.SetWindowText(_T("프로그램 종료"));

    if (GetDlgItem(IDC_READER_SETUP)) GetDlgItem(IDC_READER_SETUP)->ModifyStyle(WS_TABSTOP, 0);
    if (GetDlgItem(IDC_SHOP_SETUP)) GetDlgItem(IDC_SHOP_SETUP)->ModifyStyle(WS_TABSTOP, 0);
    if (GetDlgItem(IDC_TRANS)) GetDlgItem(IDC_TRANS)->ModifyStyle(WS_TABSTOP, 0);
    if (GetDlgItem(IDC_RECEIPT_SETUP)) GetDlgItem(IDC_RECEIPT_SETUP)->ModifyStyle(WS_TABSTOP, 0);

    GetDlgItem(IDC_READER_SETUP)->SetWindowText(_T(""));
    GetDlgItem(IDC_SHOP_SETUP)->SetWindowText(_T(""));
    GetDlgItem(IDC_TRANS)->SetWindowText(_T(""));
    GetDlgItem(IDC_RECEIPT_SETUP)->SetWindowText(_T(""));

    LoadLogoImage();
    LayoutControls();
    CenterWindow();
    return TRUE;
}

BOOL CKFTCOneCAPDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    return CDialog::OnCommand(wParam, lParam);
}

void CKFTCOneCAPDlg::EnsureFonts()
{
    if (m_bFontsReady)
        return;

    NONCLIENTMETRICS ncm = { 0 };
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    LOGFONT lf = ncm.lfMessageFont;
    lf.lfQuality = CLEARTYPE_QUALITY;
    _tcscpy(lf.lfFaceName, _T("맑은 고딕"));

    lf.lfHeight = -MulDiv(18, ModernUIDpi::GetDpiForHwnd(m_hWnd), 72);
    lf.lfWeight = FW_EXTRABOLD;   // font-weight: 800
    m_fontTitle.CreateFontIndirect(&lf);

    lf.lfHeight = -MulDiv(11, ModernUIDpi::GetDpiForHwnd(m_hWnd), 72);
    lf.lfWeight = FW_NORMAL;
    m_fontSubtitle.CreateFontIndirect(&lf);

    lf.lfHeight = -MulDiv(14, ModernUIDpi::GetDpiForHwnd(m_hWnd), 72);
    lf.lfWeight = FW_BOLD;
    m_fontCardTitle.CreateFontIndirect(&lf);

    lf.lfHeight = -MulDiv(10, ModernUIDpi::GetDpiForHwnd(m_hWnd), 72);
    lf.lfWeight = FW_NORMAL;
    m_fontCardDesc.CreateFontIndirect(&lf);

    lf.lfHeight = -MulDiv(12, ModernUIDpi::GetDpiForHwnd(m_hWnd), 72);
    lf.lfWeight = FW_NORMAL;
    m_fontFooter.CreateFontIndirect(&lf);

    m_bFontsReady = TRUE;
}

int CKFTCOneCAPDlg::SX(int px) const
{
    return ModernUIDpi::Scale(m_hWnd, px);
}

void CKFTCOneCAPDlg::LayoutControls()
{
    if (!::IsWindow(m_hWnd))
        return;

    CRect rc;
    GetClientRect(&rc);

    const int marginX = SX(50);
    const int topCardsVisual = SX(135);    // 4개의 버튼 위치(y축)
    const int cardGap = SX(18);
    const int footerBtnW = SX(184);
    const int footerBtnH = SX(40);

    int cardW = (rc.Width() - (marginX * 2) - (cardGap * 3));
    cardW /= 4;
    const int cardVisualH = SX(260);
    const int cardCtrlTop = topCardsVisual - SX(10);
    const int cardCtrlH = cardVisualH + SX(18);

    m_nFooterDividerY = topCardsVisual + cardVisualH + SX(25);
    int footerY = m_nFooterDividerY + SX(24);

    int x = marginX;
    if (GetDlgItem(IDC_READER_SETUP)) GetDlgItem(IDC_READER_SETUP)->MoveWindow(x, cardCtrlTop, cardW, cardCtrlH);
    x += cardW + cardGap;
    if (GetDlgItem(IDC_SHOP_SETUP)) GetDlgItem(IDC_SHOP_SETUP)->MoveWindow(x, cardCtrlTop, cardW, cardCtrlH);
    x += cardW + cardGap;
    if (GetDlgItem(IDC_TRANS)) GetDlgItem(IDC_TRANS)->MoveWindow(x, cardCtrlTop, cardW, cardCtrlH);
    x += cardW + cardGap;
    if (GetDlgItem(IDC_RECEIPT_SETUP)) GetDlgItem(IDC_RECEIPT_SETUP)->MoveWindow(x, cardCtrlTop, cardW, cardCtrlH);

    int totalFooterW = footerBtnW * 2 + SX(16);
    int footerX = (rc.Width() - totalFooterW) / 2;
    if (::IsWindow(m_btnMinimize.m_hWnd)) m_btnMinimize.MoveWindow(footerX, footerY, footerBtnW, footerBtnH);
    if (::IsWindow(m_btnExit.m_hWnd)) m_btnExit.MoveWindow(footerX + footerBtnW + SX(16), footerY, footerBtnW, footerBtnH);

    Invalidate(FALSE);
}

CString CKFTCOneCAPDlg::GetLogoPath() const
{
    TCHAR modulePath[MAX_PATH] = { 0 };
    ::GetModuleFileName(NULL, modulePath, MAX_PATH);
    CString path(modulePath);
    int pos = path.ReverseFind(_T('\\'));
    if (pos >= 0)
        path = path.Left(pos + 1);
    path += _T("img_ci_mark.png");
    return path;
}

void CKFTCOneCAPDlg::LoadLogoImage()
{
    if (m_pLogoBitmap != NULL)
    {
        delete m_pLogoBitmap;
        m_pLogoBitmap = NULL;
    }

#ifdef _UNICODE
    CStringW pathW(GetLogoPath());
#else
    CString pathA = GetLogoPath();
    int len = MultiByteToWideChar(CP_ACP, 0, pathA, -1, NULL, 0);
    CStringW pathW;
    LPWSTR p = pathW.GetBuffer(len + 1);
    MultiByteToWideChar(CP_ACP, 0, pathA, -1, p, len);
    pathW.ReleaseBuffer();
#endif
    Bitmap* bmp = Bitmap::FromFile(pathW);
    if (bmp != NULL && bmp->GetLastStatus() == Ok)
        m_pLogoBitmap = bmp;
    else if (bmp != NULL)
        delete bmp;
}

void CKFTCOneCAPDlg::DrawBackground(CDC& dc)
{
    CRect rc;
    GetClientRect(&rc);
    dc.FillSolidRect(&rc, kHomeBg);
}

void CKFTCOneCAPDlg::DrawHeader(CDC& dc)
{
    const int left = SX(56);
    const int top = SX(38);
    const int logoBox = SX(54);
    const int textLeft = left + logoBox + SX(15);

    Graphics g(dc.GetSafeHdc());
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    RectF rcLogo((REAL)left, (REAL)(top + SX(6)), (REAL)logoBox, (REAL)logoBox);

    if (m_pLogoBitmap != NULL)
    {
        const REAL pad = (REAL)SX(1);
        g.DrawImage(m_pLogoBitmap, RectF(rcLogo.X + pad, rcLogo.Y + pad, rcLogo.Width - pad * 2.0f, rcLogo.Height - pad * 2.0f));
    }
    else
    {
        SolidBrush fillBrush(Color(255, 18, 148, 233));
        g.FillEllipse(&fillBrush, RectF(rcLogo.X + SX(10), rcLogo.Y + SX(10), rcLogo.Width - SX(20), rcLogo.Height - SX(20)));
    }

    dc.SetBkMode(TRANSPARENT);

    //메인타이틀 위치
    CRect rcTitle(textLeft, top + SX(2), textLeft + SX(360), top + SX(34));
    // 서브타이틀 위치
    CRect rcSub(textLeft, top + SX(40), textLeft + SX(470), top + SX(63));

    CFont* pOld = dc.SelectObject(&m_fontTitle);
    dc.SetTextColor(kTitleText);
    dc.DrawText(_T("KFTCOneCAP"), &rcTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    dc.SelectObject(&m_fontSubtitle);
    dc.SetTextColor(kSubText);
    dc.DrawText(_T("금융결제원 결제 솔루션 프로그램 v1.0.0.1"), &rcSub, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    dc.SelectObject(pOld);
}

void CKFTCOneCAPDlg::DrawFooterDivider(CDC& dc)
{
    CRect rc;
    GetClientRect(&rc);

    CPen pen(PS_SOLID, 1, kFooterDivider);
    CPen* pOld = dc.SelectObject(&pen);
    int y = (m_nFooterDividerY > 0) ? m_nFooterDividerY : (rc.bottom - SX(78));
    dc.MoveTo(SX(40), y);
    dc.LineTo(rc.right - SX(40), y);
    dc.SelectObject(pOld);
}

void CKFTCOneCAPDlg::OnPaint()
{
    CPaintDC dc(this);
    DrawBackground(dc);
    DrawHeader(dc);
    DrawFooterDivider(dc);
}

BOOL CKFTCOneCAPDlg::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void CKFTCOneCAPDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialog::OnSize(nType, cx, cy);
    LayoutControls();
}

CString CKFTCOneCAPDlg::GetCardTitle(HomeCardType type) const
{
    switch (type)
    {
    case CARD_READER:  return _T("리더기 설정");
    case CARD_SHOP:    return _T("가맹점 설정");
    case CARD_TRANS:   return _T("결제 및 수납");
    default:           return _T("전표 설정");
    }
}

CString CKFTCOneCAPDlg::GetCardDescription(HomeCardType type) const
{
    switch (type)
    {
    case CARD_READER:
        return _T("COM 포트 할당 및 장비 상태\r\n점검 등 기기를 제어합니다.");
    case CARD_SHOP:
        return _T("가맹점 정보를 다운로드하고\r\n연동 환경을 구성합니다.");
    case CARD_TRANS:
        return _T("고객의 무인수납기 결제\r\n화면을 띄웁니다.");
    default:
        return _T("영수증 출력 포맷 및 전표\r\n관련 세부 옵션을\r\n설정합니다.");
    }
}

// 4. 다시 설계한 리더기 아이콘 (카드가 꽂힌 세로형 리더기)
void CKFTCOneCAPDlg::DrawReaderIcon(Graphics& g, const RectF& rc, const Color& clr)
{
    SolidBrush br(clr);
    Pen pen(clr, 1.3f);
    pen.SetLineJoin(LineJoinRound);

    auto AddRR = [](GraphicsPath& p, RectF r, REAL rad) {
        REAL d = rad * 2.0f;
        p.AddArc(r.X, r.Y, d, d, 180.0f, 90.0f);
        p.AddArc(r.GetRight() - d, r.Y, d, d, 270.0f, 90.0f);
        p.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0.0f, 90.0f);
        p.AddArc(r.X, r.GetBottom() - d, d, d, 90.0f, 90.0f);
        p.CloseFigure();
        };

    // 몸체 + 화면 구멍 + 키패드 버튼 구멍을 FillModeAlternate 한 번에 처리
    // → 배경색이 비쳐서 화면과 키버튼이 뚜렷하게 보임
    GraphicsPath bodyPath(FillModeAlternate);
    AddRR(bodyPath, RectF(rc.X + 4.5f, rc.Y + 1.0f, 19.0f, 23.5f), 3.0f);  // 몸체

    bodyPath.AddRectangle(RectF(rc.X + 7.0f, rc.Y + 3.5f, 14.0f, 8.0f));    // 화면 구멍

    // 키패드 3x2 버튼 구멍 (배경이 비쳐 버튼처럼 보임)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            REAL kx = rc.X + 7.5f + (i * 4.5f);
            REAL ky = rc.Y + 14.5f + (j * 4.2f);
            AddRR(bodyPath, RectF(kx, ky, 3.2f, 2.8f), 0.7f);  // 각 키버튼 구멍
        }
    }

    g.FillPath(&br, &bodyPath);

    // 카드 슬롯 (하단 가로선)
    Pen slotPen(clr, 1.5f);
    slotPen.SetStartCap(LineCapRound);
    slotPen.SetEndCap(LineCapRound);
    g.DrawLine(&slotPen, PointF(rc.X + 8.5f, rc.Y + 23.2f), PointF(rc.X + 19.5f, rc.Y + 23.2f));
}

void CKFTCOneCAPDlg::DrawShopIcon(Graphics& g, const RectF& rc, const Color& clr)
{
    SolidBrush br(clr);

    // 1. 지붕 삼각형
    PointF roof[] = {
        {rc.X + 14.0f, rc.Y + 1.5f},
        {rc.X + 1.5f,  rc.Y + 11.5f},
        {rc.X + 26.5f, rc.Y + 11.5f}
    };
    g.FillPolygon(&br, roof, 3);

    // 2. 건물 본체 + 문 cutout (FillModeAlternate → 문이 투명 구멍)
    GraphicsPath buildPath(FillModeAlternate);
    buildPath.AddRectangle(RectF(rc.X + 3.5f, rc.Y + 11.0f, 21.0f, 15.0f));   // 본체
    buildPath.AddRectangle(RectF(rc.X + 11.0f, rc.Y + 17.5f, 6.0f, 8.5f));    // 문 구멍
    g.FillPath(&br, &buildPath);

}

// 결제 및 수납 - IC칩 신용카드 (둥근 모서리 + 자기 띠 + IC칩 + 번호)
void CKFTCOneCAPDlg::DrawTransIcon(Graphics& g, const RectF& rc, const Color& clr)
{
    SolidBrush br(clr);

    auto AddRR = [](GraphicsPath& p, RectF r, REAL rad) {
        REAL d = rad * 2.0f;
        p.AddArc(r.X, r.Y, d, d, 180.0f, 90.0f);
        p.AddArc(r.GetRight() - d, r.Y, d, d, 270.0f, 90.0f);
        p.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0.0f, 90.0f);
        p.AddArc(r.X, r.GetBottom() - d, d, d, 90.0f, 90.0f);
        p.CloseFigure();
        };

    // 카드 외곽 + 자기띠(구멍) + IC칩 슬롯(구멍)
    GraphicsPath card(FillModeAlternate);
    AddRR(card, RectF(rc.X + 1.0f, rc.Y + 4.5f, 26.0f, 19.0f), 2.5f); // 카드 외곽
    card.AddRectangle(RectF(rc.X + 1.0f, rc.Y + 8.0f, 26.0f, 3.0f));   // 자기띠
    AddRR(card, RectF(rc.X + 3.5f, rc.Y + 13.5f, 9.0f, 6.0f), 1.0f);  // IC칩 슬롯
    g.FillPath(&br, &card);

    // IC칩 내부 접점 3x2 (FillModeAlternate: 슬롯 위에 채움→구멍 반복)
    GraphicsPath chip(FillModeAlternate);
    AddRR(chip, RectF(rc.X + 3.5f, rc.Y + 13.5f, 9.0f, 6.0f), 1.0f);  // 칩 채움
    for (int c2 = 0; c2 < 3; c2++)
        for (int r2 = 0; r2 < 2; r2++)
            AddRR(chip,
                RectF(rc.X + 4.3f + (c2 * 2.8f), rc.Y + 14.2f + (r2 * 2.4f), 1.8f, 1.6f),
                0.3f);
    g.FillPath(&br, &chip);
}

void CKFTCOneCAPDlg::DrawReceiptIcon(Graphics& g, const RectF& rc, const Color& clr)
{
    SolidBrush br(clr);

    // 1. 영수증 본체 (지그재그 하단) + 텍스트 라인 cutout (FillModeAlternate)
    GraphicsPath path(FillModeAlternate);
    path.StartFigure();
    // 외곽: 상단 → 우측 → 지그재그 하단 → 좌측
    path.AddLine(rc.X + 6.0f, rc.Y + 2.5f, rc.X + 22.0f, rc.Y + 2.5f);
    path.AddLine(rc.X + 22.0f, rc.Y + 2.5f, rc.X + 22.0f, rc.Y + 21.0f);
    path.AddLine(rc.X + 22.0f, rc.Y + 21.0f, rc.X + 19.5f, rc.Y + 24.0f);
    path.AddLine(rc.X + 19.5f, rc.Y + 24.0f, rc.X + 17.0f, rc.Y + 21.0f);
    path.AddLine(rc.X + 17.0f, rc.Y + 21.0f, rc.X + 14.0f, rc.Y + 24.0f);
    path.AddLine(rc.X + 14.0f, rc.Y + 24.0f, rc.X + 11.0f, rc.Y + 21.0f);
    path.AddLine(rc.X + 11.0f, rc.Y + 21.0f, rc.X + 8.5f, rc.Y + 24.0f);
    path.AddLine(rc.X + 8.5f, rc.Y + 24.0f, rc.X + 6.0f, rc.Y + 21.0f);
    path.AddLine(rc.X + 6.0f, rc.Y + 21.0f, rc.X + 6.0f, rc.Y + 2.5f);
    path.CloseFigure();

    // 텍스트 라인 3줄 (구멍 → 배경색으로 텍스트 느낌)
    path.AddRectangle(RectF(rc.X + 8.5f, rc.Y + 6.5f, 10.0f, 1.8f));   // 줄1 (짧음: 제목)
    path.AddRectangle(RectF(rc.X + 8.5f, rc.Y + 10.0f, 10.0f, 1.5f));   // 줄2
    path.AddRectangle(RectF(rc.X + 8.5f, rc.Y + 13.0f, 8.0f, 1.5f));   // 줄3 (더 짧음)
    path.AddRectangle(RectF(rc.X + 8.5f, rc.Y + 16.0f, 6.5f, 1.5f));   // 줄4 (가장 짧음)

    g.FillPath(&br, &path);
}


void CKFTCOneCAPDlg::DrawCardIcon(Graphics& g, const CRect& rcIcon, HomeCardType type, int nHoverProgress, int nPressProgress)
{
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    // 1. 색상 결정 - 3단계 부드러운 보간
    // Normal: bg=BLUE_50(#EBF4FF), icon=BLUE_500
    // Hover:  bg=BLUE_100(#A8D0FF), icon=BLUE_500
    // Press:  bg=BLUE_500, icon=White
    Color colorBg, colorIcon;
    {
        int hBgR = 235 + (168 - 235) * nHoverProgress / 100;
        int hBgG = 244 + (208 - 244) * nHoverProgress / 100;
        int hBgB = 255;

        int bgR = hBgR + (0 - hBgR) * nPressProgress / 100;
        int bgG = hBgG + (100 - hBgG) * nPressProgress / 100;
        int bgB = hBgB + (221 - hBgB) * nPressProgress / 100;
        bgR = max(0, min(255, bgR));
        bgG = max(0, min(255, bgG));
        bgB = max(0, min(255, bgB));
        colorBg = Color(255, (BYTE)bgR, (BYTE)bgG, (BYTE)bgB);

        int iconR = 0 + (255 - 0) * nPressProgress / 100;
        int iconG = 100 + (255 - 100) * nPressProgress / 100;
        int iconB = 221 + (255 - 221) * nPressProgress / 100;
        colorIcon = Color(255, (BYTE)min(255, iconR), (BYTE)min(255, iconG), (BYTE)min(255, iconB));
    }

    // 2. 사라졌던 배경 박스 다시 그리기
    RectF rectIcon((REAL)rcIcon.left, (REAL)rcIcon.top, (REAL)rcIcon.Width(), (REAL)rcIcon.Height());

    // 누를 때 카드 전체가 쫀득하게 작아지는 효과 (배경만 축소)
    float cardScale = 1.0f - (0.10f * nPressProgress / 100.0f);   // 10%: 아이콘 눌림감 강화
    PointF center(rectIcon.X + rectIcon.Width / 2.0f, rectIcon.Y + rectIcon.Height / 2.0f);

    g.TranslateTransform(center.X, center.Y);
    g.ScaleTransform(cardScale, cardScale);
    g.TranslateTransform(-center.X, -center.Y);

    SolidBrush brBg(colorBg);
    GraphicsPath pathBg;
    REAL rad = 12.0f; // 둥근 모서리 반지름
    pathBg.AddArc(rectIcon.X, rectIcon.Y, rad * 2, rad * 2, 180, 90);
    pathBg.AddArc(rectIcon.GetRight() - rad * 2, rectIcon.Y, rad * 2, rad * 2, 270, 90);
    pathBg.AddArc(rectIcon.GetRight() - rad * 2, rectIcon.GetBottom() - rad * 2, rad * 2, rad * 2, 0, 90);
    pathBg.AddArc(rectIcon.X, rectIcon.GetBottom() - rad * 2, rad * 2, rad * 2, 90, 90);
    pathBg.CloseFigure();
    g.FillPath(&brBg, &pathBg);

    g.ResetTransform(); // 배경 축소 효과 해제 (아이콘을 위해 리셋)

    // 3.  아이콘 그리기 (누를 때 작아지지 않게 설정)
    // 호버 시에는 커지지만, 누를 때는 작아지지 않고 호버 크기(1.1배) 유지
    float iconScale = 1.0f + (0.1f * nHoverProgress / 100.0f);

    g.TranslateTransform(center.X, center.Y);
    g.ScaleTransform(iconScale, iconScale);
    g.TranslateTransform(-center.X, -center.Y);

    RectF rcDraw(center.X - 14.0f, center.Y - 14.0f, 28.0f, 28.0f);
    SolidBrush brIcon(colorIcon);

    switch (type) {
    case CARD_READER:  DrawReaderIcon(g, rcDraw, colorIcon); break;
    case CARD_SHOP:    DrawShopIcon(g, rcDraw, colorIcon);   break;
    case CARD_TRANS:   DrawTransIcon(g, rcDraw, colorIcon);  break;
    case CARD_RECEIPT: DrawReceiptIcon(g, rcDraw, colorIcon); break;
    }

    g.ResetTransform();
}

void CKFTCOneCAPDlg::DrawHomeCard(LPDRAWITEMSTRUCT lpDIS, HomeCardType type)
{
    CDC dc;
    dc.Attach(lpDIS->hDC);

    CRect rc = lpDIS->rcItem;

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);

    memDC.FillSolidRect(0, 0, rc.Width(), rc.Height(), kHomeBg);

    int nHoverProgress = 0;
    int nPressProgress = 0;

    CWnd* pWnd = GetDlgItem((int)lpDIS->CtlID);
    CHomeCardButton* pCard = (CHomeCardButton*)pWnd;
    if (pCard != NULL && ::IsWindow(pCard->m_hWnd))
    {
        nHoverProgress = pCard->GetHoverProgress();
        nPressProgress = pCard->GetPressProgress();
    }

    Graphics g(memDC.GetSafeHdc());
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    const int hoverLift = MulDiv(SX(6), nHoverProgress * (100 - nPressProgress) / 100, 100);  // hover lift suppressed while pressed
    const int pressDown = MulDiv(SX(5), nPressProgress, 100);   // 깊은 눌림감
    const int nVisualOffsetY = pressDown - hoverLift;
    const int pressedInsetX = MulDiv(SX(3), nPressProgress, 100);   // 떨림 최소화
    const int pressedInsetY = MulDiv(SX(2), nPressProgress, 100);

    CRect rcPaint(0, SX(18), rc.Width(), rc.Height());
    rcPaint.DeflateRect(SX(2), SX(2));
    rcPaint.bottom -= SX(16);
    rcPaint.DeflateRect(pressedInsetX, pressedInsetY);
    rcPaint.OffsetRect(0, nVisualOffsetY);

    RectF blueGlowOuterRect((REAL)rcPaint.left - (REAL)SX(10),
        (REAL)rcPaint.top + (REAL)SX(2),
        (REAL)rcPaint.Width() + (REAL)SX(20),
        (REAL)rcPaint.Height() + (REAL)SX(6));
    GraphicsPath blueGlowOuterPath;
    AddRoundRectPath(blueGlowOuterPath, blueGlowOuterRect, (REAL)SX(28));
    BYTE blueGlowOuterAlpha = (BYTE)(3 + (nHoverProgress * 12) / 100);   // 은은하게: max ~15
    if (nPressProgress > 0)
        blueGlowOuterAlpha = (BYTE)max(1, blueGlowOuterAlpha - (nPressProgress * 7) / 100);

    RectF blueGlowInnerRect((REAL)rcPaint.left - (REAL)SX(4),
        (REAL)rcPaint.top + (REAL)SX(6),
        (REAL)rcPaint.Width() + (REAL)SX(8),
        (REAL)rcPaint.Height() - (REAL)SX(2));
    GraphicsPath blueGlowInnerPath;
    AddRoundRectPath(blueGlowInnerPath, blueGlowInnerRect, (REAL)SX(25));
    BYTE blueGlowInnerAlpha = (BYTE)(3 + (nHoverProgress * 10) / 100);   // 은은하게: max ~13
    if (nPressProgress > 0)
        blueGlowInnerAlpha = (BYTE)max(2, blueGlowInnerAlpha - (nPressProgress * 10) / 100);

    if (nHoverProgress > 0 && nPressProgress == 0)
    {
        SolidBrush blueGlowOuterBrush(Color(blueGlowOuterAlpha, 66, 152, 255));   // BLUE_300: 더 부드러운 glow
        g.FillPath(&blueGlowOuterBrush, &blueGlowOuterPath);

        SolidBrush blueGlowInnerBrush(Color(blueGlowInnerAlpha, 66, 152, 255));   // BLUE_300
        g.FillPath(&blueGlowInnerBrush, &blueGlowInnerPath);
    }

    RectF shadowRect((REAL)rcPaint.left + (REAL)SX(4),
        (REAL)rcPaint.top + (REAL)SX(16) + (REAL)MulDiv(SX(5), nHoverProgress, 100),
        (REAL)rcPaint.Width() - (REAL)SX(8),
        (REAL)rcPaint.Height() - (REAL)SX(14));
    GraphicsPath shadowPath;
    AddRoundRectPath(shadowPath, shadowRect, (REAL)SX(22));
    BYTE shadowAlpha = (BYTE)(12 + (nHoverProgress * 22) / 100);
    if (nPressProgress > 0)
        shadowAlpha = (BYTE)max(6, shadowAlpha - (nPressProgress * 8) / 100);
    {
        // 기본 그림자 (항상 표시) - box-shadow: 0 4px 12px rgba(0,0,0,0.04)
        BYTE baseShadowAlpha = 10;
        SolidBrush baseShadowBrush(Color(baseShadowAlpha, 17, 24, 39));
        g.FillPath(&baseShadowBrush, &shadowPath);
    }
    if (nHoverProgress > 0 && nPressProgress == 0)
    {
        // 호버 그림자 강화
        BYTE extraShadowAlpha = (BYTE)((nHoverProgress * 22) / 100);
        SolidBrush shadowBrush(Color(extraShadowAlpha, 17, 24, 39));
        g.FillPath(&shadowBrush, &shadowPath);

        RectF coreShadowRect((REAL)rcPaint.left + (REAL)SX(8),
            (REAL)rcPaint.top + (REAL)SX(20) + (REAL)MulDiv(SX(3), nHoverProgress, 100),
            (REAL)rcPaint.Width() - (REAL)SX(16),
            (REAL)rcPaint.Height() - (REAL)SX(24));
        GraphicsPath coreShadowPath;
        AddRoundRectPath(coreShadowPath, coreShadowRect, (REAL)SX(20));
        BYTE coreShadowAlpha = (BYTE)(4 + (nHoverProgress * 12) / 100);
        SolidBrush coreShadowBrush(Color(coreShadowAlpha, 17, 24, 39));
        g.FillPath(&coreShadowBrush, &coreShadowPath);
    }

    int fillR = GetRValue(kCardBg) + ((GetRValue(kCardFillHover) - GetRValue(kCardBg)) * nHoverProgress) / 100;
    int fillG = GetGValue(kCardBg) + ((GetGValue(kCardFillHover) - GetGValue(kCardBg)) * nHoverProgress) / 100;
    int fillB = GetBValue(kCardBg) + ((GetBValue(kCardFillHover) - GetBValue(kCardBg)) * nHoverProgress) / 100;

    fillR += ((GetRValue(kCardFillPressed) - fillR) * nPressProgress) / 100;
    fillG += ((GetGValue(kCardFillPressed) - fillG) * nPressProgress) / 100;
    fillB += ((GetBValue(kCardFillPressed) - fillB) * nPressProgress) / 100;

    int borderR = GetRValue(kCardBorder) + ((GetRValue(kCardBorderHover) - GetRValue(kCardBorder)) * nHoverProgress) / 100;
    int borderG = GetGValue(kCardBorder) + ((GetGValue(kCardBorderHover) - GetGValue(kCardBorder)) * nHoverProgress) / 100;
    int borderB = GetBValue(kCardBorder) + ((GetBValue(kCardBorderHover) - GetBValue(kCardBorder)) * nHoverProgress) / 100;

    RectF card((REAL)rcPaint.left + 0.5f, (REAL)rcPaint.top + 0.5f, (REAL)rcPaint.Width() - 1.0f, (REAL)rcPaint.Height() - 1.0f);
    GraphicsPath path;
    AddRoundRectPath(path, card, (REAL)SX(20));

    SolidBrush fillBrush(Color(255, fillR, fillG, fillB));
    g.FillPath(&fillBrush, &path);
    // 테두리: 정지 상태만 미세하게, hover/press 시 사라짐
    BYTE borderAlpha = (BYTE)((40 * (100 - nHoverProgress) / 100) * (100 - nPressProgress) / 100);
    if (borderAlpha > 0)
    {
        Pen borderPen(Color(borderAlpha, borderR, borderG, borderB), 1.0f);
        borderPen.SetLineJoin(LineJoinRound);
        g.DrawPath(&borderPen, &path);
    }

    const int iconShiftY = -MulDiv(SX(3), nHoverProgress, 100) + MulDiv(SX(3), nPressProgress, 100);
    CRect rcIcon(rcPaint.left + SX(28), rcPaint.top + SX(28) + iconShiftY, rcPaint.left + SX(28) + SX(56), rcPaint.top + SX(24) + iconShiftY + SX(56));
    DrawCardIcon(g, rcIcon, type, nHoverProgress, nPressProgress);

    const int textShiftY = -MulDiv(SX(3), nHoverProgress, 100) + MulDiv(SX(3), nPressProgress, 100);
    CRect rcTitle(rcPaint.left + SX(28), rcPaint.top + SX(104) + textShiftY, rcPaint.right - SX(24), rcPaint.top + SX(128) + textShiftY);
    CRect rcDesc(rcPaint.left + SX(28), rcPaint.top + SX(140) + textShiftY, rcPaint.right - SX(28), rcPaint.bottom - SX(20));

    memDC.SetBkMode(TRANSPARENT);
    memDC.SetTextColor(kCardTitleText);   // #333D4B
    CFont* pOld = memDC.SelectObject(&m_fontCardTitle);
    memDC.DrawText(GetCardTitle(type), &rcTitle, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);

    memDC.SelectObject(&m_fontCardDesc);
    memDC.SetTextColor(kSubText);
    memDC.DrawText(GetCardDescription(type), &rcDesc, DT_LEFT | DT_TOP | DT_WORDBREAK);
    memDC.SelectObject(pOld);

    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);

    memDC.SelectObject(pOldBmp);
    bmp.DeleteObject();
    memDC.DeleteDC();
    dc.Detach();
}

void CKFTCOneCAPDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
    switch (nIDCtl)
    {
    case IDC_READER_SETUP:
        DrawHomeCard(lpDIS, CARD_READER);
        return;
    case IDC_SHOP_SETUP:
        DrawHomeCard(lpDIS, CARD_SHOP);
        return;
    case IDC_TRANS:
        DrawHomeCard(lpDIS, CARD_TRANS);
        return;
    case IDC_RECEIPT_SETUP:
        DrawHomeCard(lpDIS, CARD_RECEIPT);
        return;
    default:
        CDialog::OnDrawItem(nIDCtl, lpDIS);
        return;
    }
}

HBRUSH CKFTCOneCAPDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC)
    {
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetBkColor(kHomeBg);
        return (HBRUSH)m_brBackground.GetSafeHandle();
    }
    return hbr;
}

void CKFTCOneCAPDlg::OnReaderSetup()
{
    m_ePendingOpen = PENDING_READER;
    SetTimer(kTimerWaitRelease, 16, NULL);
}

void CKFTCOneCAPDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == kTimerWaitRelease)
    {
        // Wait until the clicked card button finishes its release animation.
        // PressProgress reaches 0 regardless of PC speed -> no fixed delay needed.
        CHomeCardButton* pBtn =
            (m_ePendingOpen == PENDING_SHOP) ? &m_btnShopCard :
            (m_ePendingOpen == PENDING_READER) ? &m_btnReaderCard : NULL;

        if (!pBtn || pBtn->GetPressProgress() < 15)  // visually complete, no need to wait for exact 0
        {
            KillTimer(kTimerWaitRelease);
            EPendingOpen ePending = m_ePendingOpen;
            m_ePendingOpen = PENDING_NONE;

            if (ePending == PENDING_SHOP)
            {
                CShopSetupDlg dlg(this);
                dlg.DoModal();
            }
            else if (ePending == PENDING_READER)
            {
                CReaderSetupDlg dlg(this);
                dlg.DoModal();
            }
        }
        // else: still animating, wait for next tick
    }
    else
    {
        CDialog::OnTimer(nIDEvent);
    }
}

void CKFTCOneCAPDlg::OnShopSetup()
{
    m_ePendingOpen = PENDING_SHOP;
    SetTimer(kTimerWaitRelease, 16, NULL);
}

void CKFTCOneCAPDlg::OnTrans()
{
    AfxMessageBox(_T("결제 및 수납 화면은 다음 버전에서 연결할 예정입니다."), MB_ICONINFORMATION);
}

void CKFTCOneCAPDlg::OnReceiptSetup()
{
    AfxMessageBox(_T("전표 설정 화면은 다음 버전에서 연결할 예정입니다."), MB_ICONINFORMATION);
}

void CKFTCOneCAPDlg::OnMinimize()
{
    ShowWindow(SW_MINIMIZE);
}

void CKFTCOneCAPDlg::OnExit()
{
    EndDialog(IDCANCEL);
}

void CKFTCOneCAPDlg::OnClose()
{
    EndDialog(IDCANCEL);
}
