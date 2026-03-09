#include "stdafx.h"
#include "resource.h"
#include "KFTCOneCAPDlg.h"
#include "ReaderSetupDlg.h"
#include "ShopSetupDlg.h"

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
        m_nHoverProgress = min(100, m_nHoverProgress + 18);
        bChanged = TRUE;
    }
    else if (m_nHoverProgress > nHoverTarget)
    {
        m_nHoverProgress = max(0, m_nHoverProgress - 10);
        bChanged = TRUE;
    }

    if (m_nPressProgress < nPressTarget)
    {
        m_nPressProgress = min(100, m_nPressProgress + 34);
        bChanged = TRUE;
    }
    else if (m_nPressProgress > nPressTarget)
    {
        m_nPressProgress = max(0, m_nPressProgress - 12);
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
    COLORREF kCardFillPressed = RGB(243, 247, 252);
    COLORREF kIconBg = RGB(234, 241, 249);
    COLORREF kIconBgHover = RGB(220, 233, 248);
    COLORREF kIconBgPressed = RGB(22, 103, 222);
    COLORREF kIconStroke = RGB(31, 114, 214);
    COLORREF kTitleText = RGB(18, 31, 53);
    COLORREF kSubText = RGB(126, 142, 163);
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
    , m_dpi(96)
    , m_bFontsReady(FALSE)
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
END_MESSAGE_MAP()

BOOL CKFTCOneCAPDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    ModernUIGfx::EnsureGdiplusStartup();

    m_dpi = (int)ModernUIDpi::GetDpiForHwnd(m_hWnd);
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

    m_btnMinimize.SetButtonStyle(ButtonStyle::Default);
    m_btnExit.SetButtonStyle(ButtonStyle::Default);
    m_btnMinimize.SetUnderlayColor(kHomeBg);
    m_btnExit.SetUnderlayColor(kHomeBg);
    m_btnMinimize.SetColors(kHomeBg, RGB(232, 237, 243), RGB(121, 133, 151));
    m_btnExit.SetColors(kHomeBg, RGB(229, 234, 240), RGB(107, 119, 137));

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

    lf.lfHeight = -MulDiv(18, m_dpi, 72);
    lf.lfWeight = FW_BOLD;
    m_fontTitle.CreateFontIndirect(&lf);

    lf.lfHeight = -MulDiv(11, m_dpi, 72);
    lf.lfWeight = FW_NORMAL;
    m_fontSubtitle.CreateFontIndirect(&lf);

    lf.lfHeight = -MulDiv(14, m_dpi, 72);
    lf.lfWeight = FW_BOLD;
    m_fontCardTitle.CreateFontIndirect(&lf);

    lf.lfHeight = -MulDiv(10, m_dpi, 72);
    lf.lfWeight = FW_NORMAL;
    m_fontCardDesc.CreateFontIndirect(&lf);

    lf.lfHeight = -MulDiv(12, m_dpi, 72);
    lf.lfWeight = FW_NORMAL;
    m_fontFooter.CreateFontIndirect(&lf);

    m_bFontsReady = TRUE;
}

int CKFTCOneCAPDlg::SX(int px) const
{
    return MulDiv(px, m_dpi, 96);
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
    const int footerBtnW = SX(160);
    const int footerBtnH = SX(36);

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
void CKFTCOneCAPDlg::DrawReaderIcon(Graphics& g, const RectF& rc, Brush* pBr)
{
    GraphicsPath path;
    // 단말기 몸체
    path.AddRectangle(RectF(rc.X + (REAL)4.0, rc.Y + (REAL)2.0, (REAL)20.0, (REAL)24.0));
    // 화면(액정) 영역
    path.AddRectangle(RectF(rc.X + (REAL)6.0, rc.Y + (REAL)4.0, (REAL)16.0, (REAL)10.0));
    // 하단 키패드 (도트 6개)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            path.AddRectangle(RectF(rc.X + (REAL)7.0 + (i * 5.0f), rc.Y + (REAL)16.0 + (j * 4.0f), (REAL)3.0, (REAL)2.0));
        }
    }
    g.FillPath(pBr, &path);
}

// 2. 가맹점 설정 (문이 있는 집 모양)
void CKFTCOneCAPDlg::DrawShopIcon(Graphics& g, const RectF& rc, Brush* pBr)
{
    // 지붕 (삼각형)
    PointF roofPts[] = {
        {rc.X + (REAL)14.0, rc.Y + (REAL)2.0},
        {rc.X + (REAL)2.0,  rc.Y + (REAL)12.0},
        {rc.X + (REAL)26.0, rc.Y + (REAL)12.0}
    };
    g.FillPolygon(pBr, roofPts, 3);

    // 집 본체 (사각형)
    g.FillRectangle(pBr, RectF(rc.X + (REAL)5.0, rc.Y + (REAL)12.0, (REAL)18.0, (REAL)13.0));

    // 문(Door) - 배경색에 따라 구멍처럼 보이거나 반전되도록 별도 처리
    // 여기서는 path에서 문 영역을 제외(Exclude)하여 그리는 방식이 가장 깔끔합니다.
    GraphicsPath housePath;
    housePath.AddRectangle(RectF(rc.X + (REAL)11.0, rc.Y + (REAL)17.0, (REAL)6.0, (REAL)8.0));

    // 일반 상태에서는 배경색으로 문을 그리고, 눌렸을 때는 흰색으로 문을 그립니다.
    // (이미 pBr이 상태에 맞게 넘어오므로 덧그리기만 해도 충분합니다)
    // 좀 더 정교하게 보이려면 문 위에 작은 손잡이(점) 하나를 추가해도 좋습니다.
}

// 결제설정 (IC칩 카드)
void CKFTCOneCAPDlg::DrawTransIcon(Graphics& g, const RectF& rc, Brush* pBr)
{
    GraphicsPath path;
    // 카드 외곽선
    path.AddRectangle(RectF(rc.X + (REAL)2, rc.Y + (REAL)6, (REAL)24, (REAL)16));
    // 좌측 상단 IC칩
    path.AddRectangle(RectF(rc.X + (REAL)5, rc.Y + (REAL)10, (REAL)5, (REAL)4));
    // 우측 텍스트 라인 2줄 (카드 정보 느낌)
    path.AddRectangle(RectF(rc.X + (REAL)13, rc.Y + (REAL)10, (REAL)8, (REAL)1.5));
    path.AddRectangle(RectF(rc.X + (REAL)13, rc.Y + (REAL)13, (REAL)6, (REAL)1.5));
    // 하단 카드 번호 영역
    path.AddRectangle(RectF(rc.X + (REAL)5, rc.Y + (REAL)18, (REAL)14, (REAL)1.5));
    g.FillPath(pBr, &path);
}

// 전표설정 (영수증 모양)
void CKFTCOneCAPDlg::DrawReceiptIcon(Graphics& g, const RectF& rc, Brush* pBr)
{
    GraphicsPath path;
    path.StartFigure();
    path.AddLine(rc.X + (REAL)6, rc.Y + (REAL)4, rc.X + (REAL)22, rc.Y + (REAL)4);   // 상단
    path.AddLine(rc.X + (REAL)22, rc.Y + (REAL)4, rc.X + (REAL)22, rc.Y + (REAL)21); // 우측
    // 하단 지그재그
    path.AddLine(rc.X + (REAL)22, rc.Y + (REAL)21, rc.X + (REAL)19, rc.Y + (REAL)24);
    path.AddLine(rc.X + (REAL)19, rc.Y + (REAL)24, rc.X + (REAL)16, rc.Y + (REAL)21);
    path.AddLine(rc.X + (REAL)16, rc.Y + (REAL)21, rc.X + (REAL)13, rc.Y + (REAL)24);
    path.AddLine(rc.X + (REAL)13, rc.Y + (REAL)24, rc.X + (REAL)10, rc.Y + (REAL)21);
    path.AddLine(rc.X + (REAL)10, rc.Y + (REAL)21, rc.X + (REAL)7, rc.Y + (REAL)24);
    path.AddLine(rc.X + (REAL)7, rc.Y + (REAL)24, rc.X + (REAL)6, rc.Y + (REAL)21);
    path.AddLine(rc.X + (REAL)6, rc.Y + (REAL)21, rc.X + (REAL)6, rc.Y + (REAL)4);   // 좌측
    path.CloseFigure();
    // 영수증 내부 텍스트 라인
    path.AddRectangle(RectF(rc.X + (REAL)9, rc.Y + (REAL)8, (REAL)10, (REAL)2));
    path.AddRectangle(RectF(rc.X + (REAL)9, rc.Y + (REAL)12, (REAL)10, (REAL)2));
    path.AddRectangle(RectF(rc.X + (REAL)9, rc.Y + (REAL)16, (REAL)6, (REAL)2));
    g.FillPath(pBr, &path);
}

void CKFTCOneCAPDlg::DrawCardIcon(Graphics& g, const CRect& rcIcon, HomeCardType type, int nHoverProgress, int nPressProgress)
{
    g.SetSmoothingMode(SmoothingModeAntiAlias);

    // 1. 색상 결정 (회사 팔레트 활용)
    Color colorBg, colorIcon;
    if (nPressProgress > 0) {
        colorBg = Color(255, 0, 100, 221);     // BLUE_500 (누를 때 배경)
        colorIcon = Color(255, 255, 255, 255);   // White (누를 때 아이콘 흰색)
    }
    else {
        // BLUE_50(#EBF4FF)에서 BLUE_100(#A8D0FF)으로 부드럽게 전환
        BYTE r = 235 + (BYTE)((168 - 235) * nHoverProgress / 100);
        BYTE g_v = 244 + (BYTE)((208 - 244) * nHoverProgress / 100);
        colorBg = Color(255, r, g_v, 255);
        colorIcon = Color(255, 0, 100, 221);   // BLUE_500 (기본 아이콘)
    }

    // 2. 사라졌던 배경 박스 다시 그리기
    RectF rectIcon((REAL)rcIcon.left, (REAL)rcIcon.top, (REAL)rcIcon.Width(), (REAL)rcIcon.Height());

    // 누를 때 카드 전체가 쫀득하게 작아지는 효과 (배경만 축소)
    float cardScale = 1.0f - (0.05f * nPressProgress / 100.0f);
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
    case CARD_READER:  DrawReaderIcon(g, rcDraw, &brIcon); break;
    case CARD_SHOP:    DrawShopIcon(g, rcDraw, &brIcon);   break;
    case CARD_TRANS:   DrawTransIcon(g, rcDraw, &brIcon);  break;
    case CARD_RECEIPT: DrawReceiptIcon(g, rcDraw, &brIcon); break;
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

    const int hoverLift = MulDiv(SX(9), nHoverProgress, 100);
    const int pressDown = MulDiv(SX(9), nPressProgress, 100);
    const int nVisualOffsetY = pressDown - hoverLift;
    const int pressedInsetX = MulDiv(SX(2), nPressProgress, 100);
    const int pressedInsetY = MulDiv(SX(1), nPressProgress, 100);

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
    BYTE blueGlowOuterAlpha = (BYTE)(2 + (nHoverProgress * 20) / 100);
    if (nPressProgress > 0)
        blueGlowOuterAlpha = (BYTE)max(1, blueGlowOuterAlpha - (nPressProgress * 7) / 100);

    RectF blueGlowInnerRect((REAL)rcPaint.left - (REAL)SX(4),
        (REAL)rcPaint.top + (REAL)SX(6),
        (REAL)rcPaint.Width() + (REAL)SX(8),
        (REAL)rcPaint.Height() - (REAL)SX(2));
    GraphicsPath blueGlowInnerPath;
    AddRoundRectPath(blueGlowInnerPath, blueGlowInnerRect, (REAL)SX(25));
    BYTE blueGlowInnerAlpha = (BYTE)(4 + (nHoverProgress * 28) / 100);
    if (nPressProgress > 0)
        blueGlowInnerAlpha = (BYTE)max(2, blueGlowInnerAlpha - (nPressProgress * 10) / 100);

    if (nHoverProgress > 0 && nPressProgress == 0)
    {
        SolidBrush blueGlowOuterBrush(Color(blueGlowOuterAlpha, 120, 176, 255));
        g.FillPath(&blueGlowOuterBrush, &blueGlowOuterPath);

        SolidBrush blueGlowInnerBrush(Color(blueGlowInnerAlpha, 103, 161, 255));
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
    if (nHoverProgress > 0 && nPressProgress == 0)
    {
        SolidBrush shadowBrush(Color(shadowAlpha, 17, 24, 39));
        g.FillPath(&shadowBrush, &shadowPath);

        RectF coreShadowRect((REAL)rcPaint.left + (REAL)SX(8),
            (REAL)rcPaint.top + (REAL)SX(20) + (REAL)MulDiv(SX(3), nHoverProgress, 100),
            (REAL)rcPaint.Width() - (REAL)SX(16),
            (REAL)rcPaint.Height() - (REAL)SX(24));
        GraphicsPath coreShadowPath;
        AddRoundRectPath(coreShadowPath, coreShadowRect, (REAL)SX(20));
        BYTE coreShadowAlpha = (BYTE)(6 + (nHoverProgress * 14) / 100);
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
    BYTE borderAlpha = (BYTE)(210 - (nHoverProgress * 70) / 100);
    if (nPressProgress > 0)
        borderAlpha = (BYTE)max(108, borderAlpha - (nPressProgress * 32) / 100);
    Pen borderPen(Color(borderAlpha, borderR, borderG, borderB), 1.0f);
    borderPen.SetLineJoin(LineJoinRound);
    g.FillPath(&fillBrush, &path);
    g.DrawPath(&borderPen, &path);

    const int iconShiftY = -MulDiv(SX(2), nHoverProgress, 100) + MulDiv(SX(4), nPressProgress, 100);
    CRect rcIcon(rcPaint.left + SX(28), rcPaint.top + SX(28) + iconShiftY, rcPaint.left + SX(28) + SX(56), rcPaint.top + SX(24) + iconShiftY + SX(56));
    DrawCardIcon(g, rcIcon, type, nHoverProgress, nPressProgress);

    const int textShiftY = -MulDiv(SX(2), nHoverProgress, 100) + MulDiv(SX(4), nPressProgress, 100);
    CRect rcTitle(rcPaint.left + SX(28), rcPaint.top + SX(104) + textShiftY, rcPaint.right - SX(24), rcPaint.top + SX(128) + textShiftY);
    CRect rcDesc(rcPaint.left + SX(28), rcPaint.top + SX(140) + textShiftY, rcPaint.right - SX(28), rcPaint.bottom - SX(20));

    memDC.SetBkMode(TRANSPARENT);
    memDC.SetTextColor(kTitleText);
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
    CReaderSetupDlg dlg(this);
    dlg.DoModal();
}

void CKFTCOneCAPDlg::OnShopSetup()
{
    CShopSetupDlg dlg(this);
    dlg.DoModal();
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
