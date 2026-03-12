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
    , m_bIgnoreMouse(FALSE)
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
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

void CHomeCardButton::ResetVisualState()
{
    m_bHover = FALSE;
    m_bPressed = FALSE;
    m_bTracking = FALSE;
    m_bIgnoreMouse = FALSE;
    m_nHoverProgress = 0;
    m_nPressProgress = 0;
    KillTimer(kCardAnimTimerId);
    if (::IsWindow(m_hWnd))
        Invalidate(FALSE);
}

// 2. 함수 전체 구현
BOOL CHomeCardButton::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (::IsWindowEnabled(m_hWnd))
    {
        ::SetCursor(::LoadCursor(NULL, IDC_HAND)); // 손가락 커서로 강제 설정
        return TRUE;
    }
    return CButton::OnSetCursor(pWnd, nHitTest, message);
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

void CHomeCardButton::ForceFadeOut()
{
    m_bIgnoreMouse = TRUE; // 마우스 이벤트를 차단합니다.
    m_bHover = FALSE;   // 마우스 위치와 상관없이 호버 상태 해제
    m_bPressed = FALSE; // 눌림 상태 해제
    StartAnimTimer();   // 애니메이션 타이머 돌려서 StepAnimation이 작동하게 함
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

    // [Hover 속도] /4 -> /3으로 변경 (더 즉각적으로 '툭' 떠오르는 느낌)
    if (m_nHoverProgress != nHoverTarget)
    {
        int diff = nHoverTarget - m_nHoverProgress;
        int step = diff / 3; // 숫자가 작을수록 빨라집니다.
        if (step == 0) step = (diff > 0) ? 1 : -1;
        m_nHoverProgress += step;
        bChanged = TRUE;
    }
    // [Press 속도] 눌렀다 뗄 때의 쫀득함 유지
    if (m_nPressProgress != nPressTarget)
    {
        int diff = nPressTarget - m_nPressProgress;
        int step = 0;
        if (diff > 0) step = (diff + 2) / 3; // 누를 때 더 빠르게
        else {
            step = diff / 4; // 뗄 때 살짝 끈적하게 복귀 (고급스러움)
            if (step == 0) step = -1;
        }
        m_nPressProgress += step;
        bChanged = TRUE;
    }

    if (bChanged && ::IsWindow(m_hWnd))
        Invalidate(FALSE);

    StopAnimTimerIfIdle();
}

void CHomeCardButton::OnMouseMove(UINT nFlags, CPoint point)
{

    // 강제 종료 중(Ignore)이라면 마우스 이벤트를 아예 무시합니다.
    if (m_bIgnoreMouse)
        return;

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
    // 이미 클릭되어 처리 중(Ignore)이라면 추가 클릭 무시
    if (m_bIgnoreMouse) return;

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
    , m_pGdiFontTitle(NULL) // 추가: GDI+ 폰트 포인터 초기화
    , m_pGdiFontDesc(NULL)  // 추가: GDI+ 폰트 포인터 초기화
{
}

CKFTCOneCAPDlg::~CKFTCOneCAPDlg()
{
    // 1. GDI+ 캐시 폰트 해제 (추가됨)
    if (m_pGdiFontTitle != NULL) {
        delete m_pGdiFontTitle;
        m_pGdiFontTitle = NULL;
    }
    if (m_pGdiFontDesc != NULL) {
        delete m_pGdiFontDesc;
        m_pGdiFontDesc = NULL;
    }

    // 2. 로고 비트맵 해제
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
    ON_WM_ACTIVATE()
    ON_WM_NCACTIVATE()
    ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CKFTCOneCAPDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

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
    ModernUIWindow::ApplyWhiteTitleBar(this->GetSafeHwnd());
    return TRUE;
}

BOOL CKFTCOneCAPDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    return CDialog::OnCommand(wParam, lParam);
}

void CKFTCOneCAPDlg::EnsureFonts()
{
    // [1] 이미 폰트가 생성되었다면 중복 생성을 방지합니다.
    if (m_bFontsReady)
        return;

    // [2] 시스템 기본 폰트 정보(메시지 폰트 등)를 가져옵니다.
    NONCLIENTMETRICS ncm = { 0 };
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    LOGFONT lf = ncm.lfMessageFont;
    lf.lfQuality = CLEARTYPE_QUALITY;       // 폰트 외곽선을 부드럽게 처리
    _tcscpy(lf.lfFaceName, _T("맑은 고딕")); // 기본 폰트 설정

    // 현재 창의 DPI 세팅을 가져옵니다.
    UINT dpi = ModernUIDpi::GetDpiForHwnd(m_hWnd);

    // --- (A) MFC CFont 객체 생성 섹션 ---

    // 1. 메인 타이틀 (KFTCOneCAP) - 18pt, Extra Bold(800)
    lf.lfHeight = -MulDiv(18, dpi, 72);
    lf.lfWeight = FW_EXTRABOLD;
    m_fontTitle.CreateFontIndirect(&lf);

    // 2. 서브 타이틀 (버전 정보 등) - 11pt, Normal
    lf.lfHeight = -MulDiv(11, dpi, 72);
    lf.lfWeight = FW_NORMAL;
    m_fontSubtitle.CreateFontIndirect(&lf);

    // 3. 카드 제목 (리더기 설정 등) - 14pt, Bold
    lf.lfHeight = -MulDiv(14, dpi, 72);
    lf.lfWeight = FW_BOLD;
    m_fontCardTitle.CreateFontIndirect(&lf);

    // 4. 카드 설명 (두 줄 설명) - 10pt, Normal
    lf.lfHeight = -MulDiv(10, dpi, 72);
    lf.lfWeight = FW_NORMAL;
    m_fontCardDesc.CreateFontIndirect(&lf);

    // 5. 푸터 버튼 및 기타 - 12pt, Normal
    lf.lfHeight = -MulDiv(12, dpi, 72);
    lf.lfWeight = FW_NORMAL;
    m_fontFooter.CreateFontIndirect(&lf);


    // --- (B) GDI+ 캐시 폰트 생성 섹션 (성능 최적화 핵심) ---

    // 현재 다이얼로그의 DC를 잠시 빌려 폰트 생성의 기준점(Resolution)으로 삼습니다.
    HDC hDC = ::GetDC(m_hWnd);
    if (hDC != NULL)
    {
        // 1. 카드 제목용 GDI+ 폰트 캐싱
        LOGFONT lfCardTitle;
        m_fontCardTitle.GetLogFont(&lfCardTitle);
        m_pGdiFontTitle = new Gdiplus::Font(hDC, &lfCardTitle);

        // 2. 카드 설명용 GDI+ 폰트 캐싱
        LOGFONT lfCardDesc;
        m_fontCardDesc.GetLogFont(&lfCardDesc);
        m_pGdiFontDesc = new Gdiplus::Font(hDC, &lfCardDesc);

        ::ReleaseDC(m_hWnd, hDC);
    }

    // [3] 모든 폰트 준비 완료 플래그 설정
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

// ==========================================
// [CKFTCOneCAPDlg::DrawHeader 함수 수정]
// ==========================================
void CKFTCOneCAPDlg::DrawHeader(CDC& dc)
{
    const int left = SX(56);
    const int top = SX(38);
    const int logoBox = SX(54);
    const int textLeft = left + logoBox + SX(15);

    Graphics g(dc.GetSafeHdc());
    // 최고 품질의 렌더링 설정
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    // --- 로고 그리기 ---
    RectF rcLogo((REAL)left, (REAL)(top + SX(6)), (REAL)logoBox, (REAL)logoBox);
    if (m_pLogoBitmap != NULL) {
        const REAL pad = (REAL)SX(1);
        g.DrawImage(m_pLogoBitmap, RectF(rcLogo.X + pad, rcLogo.Y + pad, rcLogo.Width - pad * 2.0f, rcLogo.Height - pad * 2.0f));
    }
    else {
        SolidBrush fillBrush(Color(255, 18, 148, 233));
        g.FillEllipse(&fillBrush, RectF(rcLogo.X + SX(10), rcLogo.Y + SX(10), rcLogo.Width - SX(20), rcLogo.Height - SX(20)));
    }

    // --- 타이틀/서브타이틀 그리기 (GDI+ 방식으로 교체) ---
    LOGFONT lfTitle, lfSub;
    m_fontTitle.GetLogFont(&lfTitle);
    m_fontSubtitle.GetLogFont(&lfSub);

    Font gdiFontTitle(dc.GetSafeHdc(), &lfTitle);
    Font gdiFontSub(dc.GetSafeHdc(), &lfSub);

    SolidBrush titleBrush(Color(255, GetRValue(kTitleText), GetGValue(kTitleText), GetBValue(kTitleText)));
    SolidBrush subBrush(Color(255, GetRValue(kSubText), GetGValue(kSubText), GetBValue(kSubText)));

    // 텍스트 출력
    g.DrawString(L"KFTCOneCAP", -1, &gdiFontTitle, PointF((REAL)textLeft, (REAL)(top + SX(2))), &titleBrush);
    g.DrawString(L"금융결제원 결제 솔루션 프로그램 v1.0.0.1", -1, &gdiFontSub, PointF((REAL)textLeft, (REAL)(top + SX(40))), &subBrush);
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
    CRect rc;
    GetClientRect(&rc);

    if (rc.Width() <= 0 || rc.Height() <= 0) return;

    // 1. 메모리 DC 및 비트맵 생성 (더블 버퍼링)
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&bmp);

    // 2. [그리기 순서 1] 배경색 채우기 (화면이 아닌 memDC에!)
    memDC.FillSolidRect(&rc, kHomeBg);

    // 3. [그리기 순서 2] 헤더 (로고 + 글자) 그리기
    // DrawHeader가 내부에서 Graphics g(dc.GetSafeHdc())를 쓰므로 memDC를 넘겨야 함
    DrawHeader(memDC);

    // 4. [그리기 순서 3] 푸터 구분선 그리기
    DrawFooterDivider(memDC);

    // 5. [최종 전송] 완성된 이미지를 화면에 한 번에 쏘기
    // 이 순간에 글자와 배경이 동시에 나타나므로 절대 깜빡이지 않습니다.
    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);

    memDC.SelectObject(pOldBmp);
    bmp.DeleteObject();
    memDC.DeleteDC();
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
    case CARD_TRANS:   return _T("결제");
    default:           return _T("전표 설정");
    }
}

CString CKFTCOneCAPDlg::GetCardDescription(HomeCardType type) const
{
    switch (type)
    {
    case CARD_READER:
        return _T("리더기 연결 상태를\r\n설정하고 점검합니다.");
    case CARD_SHOP:
        return _T("가맹점 정보와 결제 환경을\r\n설정합니다.");
    case CARD_TRANS:
        return _T("결제 화면을 실행하고\r\n결제을 진행합니다.");
    default:
        return _T("프린터 포트와 출력 옵션을\r\n설정합니다.");
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
    // [중요] 현재 버튼 전체의 움직임(Transform) 상태를 저장합니다.
    GraphicsState state = g.Save();

    g.SetSmoothingMode(SmoothingModeAntiAlias);

    // 1. 색상 결정 로직 (기존과 동일)
    Color colorBg, colorIcon;
    {
        int hBgR = 235 + (168 - 235) * nHoverProgress / 100;
        int hBgG = 244 + (208 - 244) * nHoverProgress / 100;
        int hBgB = 255;
        int bgR = hBgR + (0 - hBgR) * nPressProgress / 100;
        int bgG = hBgG + (100 - hBgG) * nPressProgress / 100;
        int bgB = hBgB + (221 - hBgB) * nPressProgress / 100;
        colorBg = Color(255, (BYTE)max(0, min(255, bgR)), (BYTE)max(0, min(255, bgG)), (BYTE)max(0, min(255, bgB)));

        int iconR = 0 + (255 - 0) * nPressProgress / 100;
        int iconG = 100 + (255 - 100) * nPressProgress / 100;
        int iconB = 221 + (255 - 221) * nPressProgress / 100;
        int iconA = 255 - (35 * nPressProgress / 100);
        colorIcon = Color((BYTE)iconA, (BYTE)min(255, iconR), (BYTE)min(255, iconG), (BYTE)min(255, iconB));
    }

    // 2. 배경 박스 그리기
    RectF rectIcon((REAL)rcIcon.left, (REAL)rcIcon.top, (REAL)rcIcon.Width(), (REAL)rcIcon.Height());
    PointF center(rectIcon.X + rectIcon.Width / 2.0f, rectIcon.Y + rectIcon.Height / 2.0f);

    // [아이콘 전용 로컬 변환] 누를 때 아이콘 배경만 살짝 더 작아지는 효과
    float localScale = 1.0f - (0.03f * nPressProgress / 100.0f);
    g.TranslateTransform(center.X, center.Y);
    g.ScaleTransform(localScale, localScale);
    g.TranslateTransform(-center.X, -center.Y);

    SolidBrush brBg(colorBg);
    GraphicsPath pathBg;
    REAL rad = 12.0f;
    pathBg.AddArc(rectIcon.X, rectIcon.Y, rad * 2, rad * 2, 180, 90);
    pathBg.AddArc(rectIcon.GetRight() - rad * 2, rectIcon.Y, rad * 2, rad * 2, 270, 90);
    pathBg.AddArc(rectIcon.GetRight() - rad * 2, rectIcon.GetBottom() - rad * 2, rad * 2, rad * 2, 0, 90);
    pathBg.AddArc(rectIcon.X, rectIcon.GetBottom() - rad * 2, rad * 2, rad * 2, 90, 90);
    pathBg.CloseFigure();
    g.FillPath(&brBg, &pathBg);

    // 3. 실제 아이콘 모양 그리기 (호버 시 살짝 커짐)
    float iconScale = 1.0f + (0.05f * nHoverProgress / 100.0f);
    g.TranslateTransform(center.X, center.Y);
    g.ScaleTransform(iconScale, iconScale);
    g.TranslateTransform(-center.X, -center.Y);

    RectF rcDraw(center.X - 14.0f, center.Y - 14.0f, 28.0f, 28.0f);
    switch (type) {
    case CARD_READER:  DrawReaderIcon(g, rcDraw, colorIcon); break;
    case CARD_SHOP:    DrawShopIcon(g, rcDraw, colorIcon);   break;
    case CARD_TRANS:   DrawTransIcon(g, rcDraw, colorIcon);  break;
    case CARD_RECEIPT: DrawReceiptIcon(g, rcDraw, colorIcon); break;
    }

    // [가장 중요] 아이콘 전용 변환을 종료하고, 아까 저장했던 버튼 전체 움직임 상태로 복구합니다.
    g.Restore(state);
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

    // [1] 배경 초기화
    memDC.FillSolidRect(0, 0, rc.Width(), rc.Height(), kHomeBg);

    int nHoverProgress = 0, nPressProgress = 0;
    CHomeCardButton* pCard = (CHomeCardButton*)GetDlgItem((int)lpDIS->CtlID);
    if (pCard != NULL && ::IsWindow(pCard->m_hWnd)) {
        nHoverProgress = pCard->GetHoverProgress();
        nPressProgress = pCard->GetPressProgress();
    }

    Graphics g(memDC.GetSafeHdc());
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    // [2] 기준 사각형 고정
    CRect rcPaint(0, SX(18), rc.Width(), rc.Height());
    rcPaint.DeflateRect(SX(2), SX(2));
    rcPaint.bottom -= SX(16);

    // [3] 물리 변화량 계산
    REAL cardScale = 1.0f - (0.04f * nPressProgress / 100.0f); // 배경은 4% 축소
    REAL textScale = 1.0f - (0.02f * nPressProgress / 100.0f); // 글자는 2%만 축소 (선명도 유지)

    REAL liftY = (REAL)SX(5) * nHoverProgress / 100.0f;
    REAL pushY = (REAL)SX(6) * nPressProgress / 100.0f;
    REAL totalOffsetY = pushY - liftY;

    PointF cardCenter((REAL)rcPaint.left + rcPaint.Width() / 2.0f, (REAL)rcPaint.top + rcPaint.Height() / 2.0f);

    // ---------------------------------------------------------
    // [4] 카드 본체 및 아이콘 그리기 (4% 축소)
    // ---------------------------------------------------------
    GraphicsState cardState = g.Save();

    // 이동 + 카드 전용 축소
    g.TranslateTransform(cardCenter.X, cardCenter.Y + totalOffsetY);
    g.ScaleTransform(cardScale, cardScale);
    g.TranslateTransform(-cardCenter.X, -cardCenter.Y);

    int blurSpread = 8 + (nHoverProgress * 8) / 100; // 퍼짐 정도를 조금 더 키움 (6 -> 8)
    BYTE maxAlpha = (BYTE)(12 + (nHoverProgress * 20) / 100); // 평소엔 연하게, 호버 시 진하게

    // [수정] 눌렀을 때(Press) 그림자가 10% 정도는 남도록 (완전히 사라짐 방지)
    if (nPressProgress > 0)
        maxAlpha = (BYTE)(maxAlpha * (100 - (nPressProgress * 0.9f)) / 100);
    if (maxAlpha > 0) {
        // 그림자가 아래로 내려가는 거리 (호버 시 더 멀어짐)
        REAL glowOffsetY = (REAL)SX(10) + (REAL)MulDiv(SX(8), nHoverProgress, 100);

        RectF baseGlowRect((REAL)rcPaint.left + (REAL)SX(4), (REAL)rcPaint.top + glowOffsetY,
            (REAL)rcPaint.Width() - (REAL)SX(8), (REAL)rcPaint.Height() - (REAL)SX(14));

        GraphicsPath glowPath;
        AddRoundRectPath(glowPath, baseGlowRect, (REAL)SX(22));

        for (int i = blurSpread; i >= 1; i -= 2) {
            BYTE currentAlpha = (BYTE)(maxAlpha * (blurSpread - i + 2) / (blurSpread * 2));

            // [색상 수정] 0, 100, 221(순수 파랑) 대신 20, 40, 100 정도의 짙은 네이비를 섞으면 훨씬 고급스럽습니다.
            Pen glowPen(Color(currentAlpha, 20, 60, 160), (REAL)(i * 2));
            glowPen.SetLineJoin(LineJoinRound);
            g.DrawPath(&glowPen, &glowPath);
        }
    }

    // --- 카드 배경색 채우기 ---
    int fillR = GetRValue(kCardBg) + ((GetRValue(kCardFillHover) - GetRValue(kCardBg)) * nHoverProgress) / 100;
    int fillG = GetGValue(kCardBg) + ((GetGValue(kCardFillHover) - GetGValue(kCardBg)) * nHoverProgress) / 100;
    int fillB = GetBValue(kCardBg) + ((GetBValue(kCardFillHover) - GetBValue(kCardBg)) * nHoverProgress) / 100;
    fillR += ((GetRValue(kCardFillPressed) - fillR) * nPressProgress) / 100;
    fillG += ((GetGValue(kCardFillPressed) - fillG) * nPressProgress) / 100;
    fillB += ((GetBValue(kCardFillPressed) - fillB) * nPressProgress) / 100;

    RectF cardRect((REAL)rcPaint.left + 0.5f, (REAL)rcPaint.top + 0.5f, (REAL)rcPaint.Width() - 1.0f, (REAL)rcPaint.Height() - 1.0f);
    GraphicsPath path;
    AddRoundRectPath(path, cardRect, (REAL)SX(20));
    SolidBrush fillBrush(Color(255, (BYTE)fillR, (BYTE)fillG, (BYTE)fillB));
    g.FillPath(&fillBrush, &path);

    // --- 테두리 ---
    BYTE borderAlpha = (BYTE)((40 * (100 - nHoverProgress) / 100) * (100 - nPressProgress) / 100);
    if (borderAlpha > 0) {
        Pen borderPen(Color(borderAlpha, 226, 232, 240), 1.0f);
        g.DrawPath(&borderPen, &path);
    }

    // --- 아이콘 ---
    CRect rcIcon(rcPaint.left + SX(28), rcPaint.top + SX(28), rcPaint.left + SX(28) + SX(56), rcPaint.top + SX(28) + SX(56));
    DrawCardIcon(g, rcIcon, type, nHoverProgress, nPressProgress);

    g.Restore(cardState); // 카드 축소 변환 종료

    // ---------------------------------------------------------
    // [5] 텍스트 그리기 (2%만 축소하여 선명도와 눌림감 모두 확보)
    // ---------------------------------------------------------
    GraphicsState textState = g.Save();

    // 텍스트는 카드보다 아주 미세하게 더 아래로 내려가게 해서 깊이감을 줍니다. (Parallax)
    REAL textOffsetY = totalOffsetY + (pushY * 0.2f);
    g.TranslateTransform(cardCenter.X, cardCenter.Y + textOffsetY);
    g.ScaleTransform(textScale, textScale);
    g.TranslateTransform(-cardCenter.X, -cardCenter.Y);

    g.SetTextRenderingHint(TextRenderingHintAntiAlias);
    if (m_pGdiFontTitle == NULL || m_pGdiFontDesc == NULL)
        EnsureFonts(); // 혹시

    SolidBrush titleBrush(Color(255, GetRValue(kCardTitleText), GetGValue(kCardTitleText), GetBValue(kCardTitleText)));
    SolidBrush descBrush(Color(255, GetRValue(kSubText), GetGValue(kSubText), GetBValue(kSubText)));

    StringFormat format;
    format.SetAlignment(StringAlignmentNear);
    format.SetLineAlignment(StringAlignmentNear);
    format.SetTrimming(StringTrimmingEllipsisCharacter);

    RectF layoutTitle((REAL)rcPaint.left + SX(28), (REAL)rcPaint.top + SX(104), (REAL)rcPaint.Width() - SX(56), (REAL)SX(24));
    RectF layoutDesc((REAL)rcPaint.left + SX(28), (REAL)rcPaint.top + SX(136), (REAL)rcPaint.Width() - SX(56), (REAL)rcPaint.Height() - SX(150));

    // 캐싱된 폰트 포인터(*m_pGdiFontTitle)를 사용하여 그리기
    g.DrawString(CT2W(GetCardTitle(type)), -1, m_pGdiFontTitle, layoutTitle, &format, &titleBrush);
    g.DrawString(CT2W(GetCardDescription(type)), -1, m_pGdiFontDesc, layoutDesc, &format, &descBrush);

    g.Restore(textState);

    // [6] 최종 출력
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

BOOL CKFTCOneCAPDlg::OnNcActivate(BOOL bActive)
{
    // [중요] 기본 클래스 호출을 막아 OS가 타이틀바나 배경을 다시 그리는 것을 방지합니다.
    return TRUE;
}

void CKFTCOneCAPDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    // [중요] 기본 클래스 호출(CDialog::OnActivate)을 생략합니다.
    // 이렇게 하면 모달창이 뜰 때 부모 창이 불필요하게 다시 그려지지 않습니다.
}

void CKFTCOneCAPDlg::OnReaderSetup()
{
    m_ePendingOpen = PENDING_READER;
    m_btnReaderCard.ForceFadeOut(); //
    SetTimer(kTimerWaitRelease, 16, NULL);
}

void CKFTCOneCAPDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == kTimerWaitRelease)
    {
        CHomeCardButton* pBtn = (m_ePendingOpen == PENDING_SHOP) ? &m_btnShopCard :
            (m_ePendingOpen == PENDING_READER) ? &m_btnReaderCard : NULL;

        if (pBtn && pBtn->GetPressProgress() <= 12 && pBtn->GetHoverProgress() <= 12)
        {
            KillTimer(kTimerWaitRelease);

            EPendingOpen ePending = m_ePendingOpen;
            m_ePendingOpen = PENDING_NONE;

            // [1] 현재 상태를 메모리 DC 기반으로 깨끗하게 한 번 그립니다.
            this->Invalidate(FALSE);
            this->UpdateWindow(); // 즉시 OnPaint 호출

            this->EnableWindow(FALSE);

            // [2] 모달 실행
            if (ePending == PENDING_SHOP) {
                CShopSetupDlg dlg(this);
                dlg.DoModal();
            }
            else if (ePending == PENDING_READER) {
                CReaderSetupDlg dlg(this);
                dlg.DoModal();
            }

            this->EnableWindow(TRUE);
            if (pBtn) pBtn->ResetVisualState();

            // [3] 돌아왔을 때 배경을 지우지 않고(RDW_NOERASE) 전체 자식 버튼까지 갱신
            this->RedrawWindow(NULL, NULL,
                RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_NOERASE);
        }
    }
    else
    {
        CDialog::OnTimer(nIDEvent);
    }
}

void CKFTCOneCAPDlg::OnShopSetup()
{
    m_ePendingOpen = PENDING_SHOP;
    m_btnShopCard.ForceFadeOut(); // <-- "자연스럽게 호버 풀어!"라고 명령
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
