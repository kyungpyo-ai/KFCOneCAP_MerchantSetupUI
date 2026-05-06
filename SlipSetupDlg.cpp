#include "stdafx.h"
#include "SlipSetupDlg.h"
#include "Resource.h"
#include "ModernMessageBox.h"
#include <gdiplus.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma comment(lib, "gdiplus.lib")

#define TIMER_HOVER 100

IMPLEMENT_DYNAMIC(CSlipSetupDlg, CDialog)

static BOOL IsCompactScreen() { return ::GetSystemMetrics(SM_CYSCREEN) <= 800; }

BEGIN_MESSAGE_MAP(CSlipSetupDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_SLIP_BTN_OK, OnBtnOk)
    ON_BN_CLICKED(IDC_SLIP_BTN_CANCEL, OnBtnCancel)
    ON_BN_CLICKED(IDC_SLIP_PRINT_ENABLE, OnPrintEnableToggled)
    ON_BN_CLICKED(IDC_SLIP_BTN_LAST_PRINT, OnBtnLastPrint)
    ON_EN_CHANGE(IDC_SLIP_EDIT_PORT, OnSlipPortChanged)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_SLIP_BTN_PRINT_INFO, IDC_SLIP_BTN_MSG_INFO, OnInfoBtnClicked)
END_MESSAGE_MAP()

CSlipSetupDlg::CSlipSetupDlg(CWnd* pParent)
    : CDialog(IDD_SLIP_SETUP_DLG, pParent)
    , m_hFontCardTitle(nullptr), m_hFontHdrTitle(nullptr), m_hFontHdrSub(nullptr)
    , m_uHoverTimer(0), m_bUiInitialized(FALSE)
{
}

CSlipSetupDlg::~CSlipSetupDlg() {}

void CSlipSetupDlg::DoDataExchange(CDataExchange* pDX) { CDialog::DoDataExchange(pDX); }

int CSlipSetupDlg::SX(int px) const { return ModernUIDpi::Scale(m_hWnd, px); }

// 레이아웃 계산 구조체
struct SlipLayoutData {
    CRect rcCard1, rcCard2, rcCard3;
    int lY1, cY1, lY2, cY2, msgStartY;
    int inX, inW, col2W, cG;
    int capH, capG, FIELD_H, rG;
    int btnY;
    CRect rcBtnLastPrint, rcBtnOk, rcBtnCancel;
    int kCardMarginL, kCardMarginT, kCardMarginR, kCardMarginB;
};

static void CalcLayout(HWND hWnd, const CRect& rc, SlipLayoutData& out) {
    BOOL bCmp = IsCompactScreen();
    auto ScaleX = [&](int px) { return ModernUIDpi::Scale(hWnd, px); };

    out.kCardMarginL = ScaleX(bCmp ? 12 : 20);
    out.kCardMarginT = ScaleX(bCmp ? 6 : 10);
    out.kCardMarginR = ScaleX(bCmp ? 12 : 20);
    out.kCardMarginB = ScaleX(bCmp ? 12 : 20);

    const int cOutX = bCmp ? 10 : 16;
    const int cPadX = bCmp ? 14 : 22;
    const int cPadY = bCmp ? 10 : 16;
    const int cGapY = bCmp ? 8 : 12;
    const int cHdrH = bCmp ? 36 : 44;
    out.capH = bCmp ? 16 : 18;
    out.capG = bCmp ? 4 : 7;
    out.rG = bCmp ? 10 : 20;
    out.cG = bCmp ? 12 : 20;
    out.FIELD_H = bCmp ? 32 : 40;

    int divY0 = out.kCardMarginT + ScaleX(bCmp ? 64 : 84);
    int curY = out.kCardMarginT + ScaleX(bCmp ? 72 : 86);

    int cardLeft = out.kCardMarginL + ScaleX(cOutX);
    int cardRight = rc.Width() - out.kCardMarginR - ScaleX(cOutX);
    out.inX = cardLeft + ScaleX(cPadX);
    out.inW = cardRight - cardLeft - ScaleX(cPadX * 2);
    out.col2W = (out.inW - ScaleX(out.cG)) / 2;

    // Card 1: 인쇄 설정
    int innerY = curY + ScaleX(cPadY) + ScaleX(cHdrH);
    out.rcCard1 = CRect(cardLeft, curY, cardRight, innerY + ScaleX(out.FIELD_H) + ScaleX(cPadY));
    curY = out.rcCard1.bottom + ScaleX(cGapY);

    // Card 2: 기본 설정
    int y2 = curY;
    innerY = y2 + ScaleX(cPadY) + ScaleX(cHdrH);
    out.lY1 = innerY;
    out.cY1 = out.lY1 + ScaleX(out.capH) + ScaleX(out.capG);
    out.lY2 = out.cY1 + ScaleX(out.FIELD_H) + ScaleX(out.rG);
    out.cY2 = out.lY2 + ScaleX(out.capH) + ScaleX(out.capG);
    out.rcCard2 = CRect(cardLeft, y2, cardRight, out.cY2 + ScaleX(out.FIELD_H) + ScaleX(cPadY));
    curY = out.rcCard2.bottom + ScaleX(cGapY);

    // --- [수정할 부분] CalcLayout() 함수 안의 Card 3 블록 ---
        // Card 3: 전표 메시지
    int y3 = curY;
    out.msgStartY = y3 + ScaleX(cPadY) + ScaleX(cHdrH);
    // 한 줄의 높이 = 라벨 높이 + 간격 + 에딧박스 높이 + 줄바꿈 간격
    int msgRowH = ScaleX(out.capH) + ScaleX(out.capG) + ScaleX(out.FIELD_H) + ScaleX(out.rG);
    out.rcCard3 = CRect(cardLeft, y3, cardRight, out.msgStartY + 3 * msgRowH - ScaleX(out.rG) + ScaleX(cPadY));
    curY = out.rcCard3.bottom + ScaleX(bCmp ? 10 : 16);

    // Buttons Layout
    out.btnY = curY;
    const int BUTTON_H = ScaleX(bCmp ? 32 : 36);
    const int BUTTON_GAP = ScaleX(12);
    const int BUTTON_W = ScaleX(110);

    int footerPadX = out.kCardMarginL + ScaleX(cOutX);
    out.rcBtnLastPrint = CRect(footerPadX, out.btnY, footerPadX + ScaleX(140), out.btnY + BUTTON_H);

    int btnCX = rc.Width() / 2;
    out.rcBtnOk     = CRect(btnCX - BUTTON_W - BUTTON_GAP / 2, out.btnY, btnCX - BUTTON_GAP / 2,     out.btnY + BUTTON_H);
    out.rcBtnCancel = CRect(btnCX + BUTTON_GAP / 2,             out.btnY, btnCX + BUTTON_GAP / 2 + BUTTON_W, out.btnY + BUTTON_H);
}

void CSlipSetupDlg::EnsureFonts()
{
    if (m_fontTitle.GetSafeHandle()) return;
    LOGFONT lf = { 0 };
    ::GetObject((HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    ModernUIFont::ApplyUIFontFace(lf);

    BOOL bCmp = IsCompactScreen();
    lf.lfHeight = -SX(bCmp ? 16 : 20); lf.lfWeight = FW_BOLD;
    m_fontTitle.CreateFontIndirect(&lf);
    lf.lfHeight = -SX(bCmp ? 10 : 11); lf.lfWeight = FW_NORMAL;
    m_fontSubtitle.CreateFontIndirect(&lf);
    lf.lfHeight = -SX(bCmp ? 13 : 13); lf.lfWeight = FW_BOLD;
    m_fontSection.CreateFontIndirect(&lf);
    lf.lfHeight = -SX(bCmp ? 12 : 14); lf.lfWeight = FW_BOLD;
    m_fontLabel.CreateFontIndirect(&lf);
    lf.lfHeight = -SX(bCmp ? 12 : 14); lf.lfWeight = FW_MEDIUM;
    m_fontCombo.CreateFontIndirect(&lf);

    lf.lfHeight = -SX(bCmp ? 15 : 18); lf.lfWeight = FW_BOLD;
    m_hFontHdrTitle = ::CreateFontIndirect(&lf);
    lf.lfHeight = -SX(bCmp ? 11 : 13); lf.lfWeight = FW_BOLD;
    m_hFontHdrSub = ::CreateFontIndirect(&lf);
    lf.lfHeight = -SX(bCmp ? 13 : 15); lf.lfWeight = FW_BOLD;
    m_hFontCardTitle = ::CreateFontIndirect(&lf);
    lf.lfHeight = -SX(bCmp ? 12 : 13); lf.lfWeight = FW_BOLD;
    m_fontValidation.CreateFontIndirect(&lf);
}

BOOL CSlipSetupDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetRedraw(FALSE);
    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    ModernUIGfx::EnsureGdiplusStartup();

    m_brBg.CreateSolidBrush(RGB(249, 250, 252));
    m_brWhite.CreateSolidBrush(RGB(255, 255, 255));
    EnsureFonts();

    // 123번 줄 부근: 기존 코드를 아래 내용으로 교체
    {
        // 1. 클라이언트(안쪽) 영역의 너비와 충분한 높이 설정
        int targetClientW = SX(IsCompactScreen() ? 640 : 760);
        CRect rcTemp(0, 0, targetClientW, 2000);

        SlipLayoutData d;
        CalcLayout(m_hWnd, rcTemp, d);

        // 2. 실제 필요한 안쪽 높이 계산 (버튼 위치 + 버튼 높이 + 하단 여백)
        int btnH = SX(IsCompactScreen() ? 32 : 36);
        int bottomMargin = SX(IsCompactScreen() ? 22 : 40);
        int requiredClientH = d.btnY + btnH + bottomMargin;

        // 3. 타이틀바와 테두리 두께를 포함한 전체 창 크기로 변환
        CRect rcWindow(0, 0, targetClientW, requiredClientH);
        // 이 함수가 현재 스타일(WS_CAPTION 등)을 계산해서 rcWindow를 키워줍니다.
        AdjustWindowRectEx(&rcWindow, GetStyle(), FALSE, GetExStyle());

        int finalWinW = rcWindow.Width();
        int finalWinH = rcWindow.Height();

        // 4. 모니터 크기보다 커지지 않도록 방지
        int maxH = ::GetSystemMetrics(SM_CYMAXIMIZED) - 40;
        if (finalWinH > maxH) finalWinH = maxH;

        // 5. 최종 크기 적용 및 중앙 배치
        SetWindowPos(nullptr, 0, 0, finalWinW, finalWinH, SWP_NOMOVE | SWP_NOZORDER);
        CenterWindow();
    }

    // 컨트롤 생성 및 초기화
    m_chkPrintEnable.SubclassDlgItem(IDC_SLIP_PRINT_ENABLE, this);
    m_chkPrintEnable.SetWindowText(_T("전표 인쇄 사용"));
    m_chkPrintEnable.SetFont(&m_fontLabel);

    m_comboPrintCount.SubclassDlgItem(IDC_SLIP_COMBO_PRINT_COUNT, this);
    m_comboPrintCount.SetFont(&m_fontCombo);
    m_comboPrintCount.SetUnderlayColor(RGB(250, 251, 253));
    m_comboPrintCount.AddString(_T("1장"));
    m_comboPrintCount.AddString(_T("2장"));

    m_comboSpeed.SubclassDlgItem(IDC_SLIP_COMBO_SPEED, this);
    m_comboSpeed.SetFont(&m_fontCombo);
    m_comboSpeed.SetUnderlayColor(RGB(250, 251, 253));
    m_comboSpeed.AddString(_T("9600bps"));
    m_comboSpeed.AddString(_T("38400bps"));
    m_comboSpeed.AddString(_T("57600bps"));
    m_comboSpeed.AddString(_T("115200bps"));

    m_editPort.SubclassDlgItem(IDC_SLIP_EDIT_PORT, this);
    m_editPort.SetFont(&m_fontCombo);
    m_editPort.SetUnderlayColor(RGB(250, 251, 253));

    for (int i = 0; i < 6; i++) {
        m_editMsg[i].SubclassDlgItem(IDC_SLIP_EDIT_MSG1 + i, this);
        m_editMsg[i].SetFont(&m_fontCombo);
        m_editMsg[i].SetUnderlayColor(RGB(250, 251, 253));
    }

    m_btnPrintInfo.SubclassDlgItem(IDC_SLIP_BTN_PRINT_INFO, this);
    m_btnPortInfo.SubclassDlgItem(IDC_SLIP_BTN_PORT_INFO, this);
    m_btnSpeedInfo.SubclassDlgItem(IDC_SLIP_BTN_SPEED_INFO, this);
    m_btnMsgInfo.SubclassDlgItem(IDC_SLIP_BTN_MSG_INFO, this);

    // 포트번호 유효성 에러 Static
    {
        HWND hErr = ::GetDlgItem(GetSafeHwnd(), IDC_SLIP_STATIC_ERR_PORT);
        if (hErr) ::SendMessage(hErr, WM_SETFONT, (WPARAM)(HFONT)m_fontValidation.GetSafeHandle(), TRUE);
    }

    m_btnLastPrint.SubclassDlgItem(IDC_SLIP_BTN_LAST_PRINT, this);
    m_btnLastPrint.SetWindowText(_T("직전거래 전표출력"));
    m_btnLastPrint.SetButtonStyle(ButtonStyle::Reader);
    m_btnLastPrint.SetUnderlayColor(RGB(255, 255, 255));
    m_btnLastPrint.SetFont(&m_fontLabel);

    m_btnOk.SubclassDlgItem(IDC_SLIP_BTN_OK, this);
    m_btnOk.SetWindowText(_T("확인"));
    m_btnOk.SetButtonStyle(ButtonStyle::Primary);
    m_btnOk.SetUnderlayColor(RGB(255, 255, 255));
    m_btnOk.SetFont(&m_fontLabel);

    m_btnCancel.SubclassDlgItem(IDC_SLIP_BTN_CANCEL, this);
    m_btnCancel.SetWindowText(_T("취소"));
    m_btnCancel.SetButtonStyle(ButtonStyle::Default);
    m_btnCancel.SetUnderlayColor(RGB(255, 255, 255));
    m_btnCancel.SetFont(&m_fontLabel);

    LoadFromRegistry();
    LayoutControls();
    if (m_comboPrintCount.GetCurSel() == CB_ERR) m_comboPrintCount.SetCurSel(0);
    if (m_comboSpeed.GetCurSel() == CB_ERR) m_comboSpeed.SetCurSel(2);
    UpdateControlsState();
    SetRedraw(TRUE);
    RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);

    m_bUiInitialized = TRUE;
    m_uHoverTimer = SetTimer(TIMER_HOVER, 16, nullptr);
    ModernUIWindow::ApplyWhiteTitleBar(this->GetSafeHwnd());
    return TRUE;
}

void CSlipSetupDlg::LayoutControls()
{
    CRect rc; GetClientRect(&rc);
    SlipLayoutData d;
    CalcLayout(m_hWnd, rc, d);

    auto SetupCombo = [&](CWnd& cb, int x, int y, int w, int h) {
        cb.MoveWindow(x, y, w, SX(220));
        ::SendMessage(cb.GetSafeHwnd(), CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)(h - 2));
        };

    CClientDC measureDC(this);
    auto PlaceInfoBtn = [&](CInfoIconButton& btn, int lx, int ly, LPCTSTR szLabel) {
        const int BtnSz = SX(18);
        const int BtnGap = SX(4);
        int by = ly + (SX(d.capH) - BtnSz) / 2;
        CFont* pOld = measureDC.SelectObject(&m_fontLabel);
        CSize sz = measureDC.GetTextExtent(szLabel);
        measureDC.SelectObject(pOld);
        btn.SetWindowPos(nullptr, lx + sz.cx + BtnGap, by, BtnSz, BtnSz, SWP_NOZORDER);
        };

    m_chkPrintEnable.MoveWindow(d.inX, d.rcCard1.top + SX(IsCompactScreen() ? 46 : 60), d.col2W, SX(d.FIELD_H));

    SetupCombo(m_comboSpeed, d.inX, d.cY1, d.col2W, SX(d.FIELD_H));
    m_editPort.MoveWindow(d.inX + d.col2W + SX(d.cG), d.cY1, d.col2W, SX(d.FIELD_H));
    SetupCombo(m_comboPrintCount, d.inX, d.cY2, d.col2W, SX(d.FIELD_H));

    PlaceInfoBtn(m_btnSpeedInfo, d.inX, d.lY1, _T("프린터 속도"));
    PlaceInfoBtn(m_btnPortInfo, d.inX + d.col2W + SX(d.cG), d.lY1, _T("프린터 포트번호"));
    // 에러 Static: 포트번호 라벨 우측 정렬
    { CWnd* pErr = GetDlgItem(IDC_SLIP_STATIC_ERR_PORT);
      if (pErr && pErr->GetSafeHwnd()) {
          int errW = SX(72);
          int errX = d.inX + d.col2W + SX(d.cG) + d.col2W - errW;
          pErr->SetWindowPos(nullptr, errX, d.lY1, errW, SX(d.capH), SWP_NOZORDER | SWP_NOACTIVATE);
      } }
    PlaceInfoBtn(m_btnPrintInfo, d.inX, d.lY2, _T("전표 매수"));

    // --- [수정할 부분] LayoutControls() 함수 안의 for문 ---
    for (int r = 0; r < 3; r++) {
        // 라벨을 위한 공간을 띄우고 에딧박스 배치
        int msgRowH = SX(d.capH) + SX(d.capG) + SX(d.FIELD_H) + SX(d.rG);
        int my = d.msgStartY + r * msgRowH;
        int editY = my + SX(d.capH) + SX(d.capG); // 라벨(capH)과 간격(capG) 아래에 에딧박스 시작

        m_editMsg[r * 2].MoveWindow(d.inX, editY, d.col2W, SX(d.FIELD_H));
        m_editMsg[r * 2 + 1].MoveWindow(d.inX + d.col2W + SX(d.cG), editY, d.col2W, SX(d.FIELD_H));
    }

    // 전표 메시지 카드 타이틀 옆 인포 버튼 배치
    {
        const int BtnSz = SX(18);
        int hdrH = SX(IsCompactScreen() ? 36 : 44);
        int lx = d.rcCard3.left + SX(26);
        HFONT hOld = (HFONT)::SelectObject(measureDC.GetSafeHdc(), m_hFontCardTitle);
        CSize sz = measureDC.GetTextExtent(_T("전표 메시지"));
        ::SelectObject(measureDC.GetSafeHdc(), hOld);
        int bx = lx + sz.cx + SX(4);
        int by = d.rcCard3.top + (hdrH - BtnSz) / 2;
        m_btnMsgInfo.SetWindowPos(nullptr, bx, by, BtnSz, BtnSz, SWP_NOZORDER);
    }

    m_btnLastPrint.MoveWindow(d.rcBtnLastPrint);
    m_btnOk.MoveWindow(d.rcBtnOk);
    m_btnCancel.MoveWindow(d.rcBtnCancel);
}

void CSlipSetupDlg::DrawBackground(CDC* pDC)
{
    CRect rc; GetClientRect(&rc);
    SlipLayoutData d;
    CalcLayout(m_hWnd, rc, d);

    Gdiplus::Graphics g(pDC->GetSafeHdc());
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    auto RR = [](Gdiplus::GraphicsPath& path, const Gdiplus::RectF& r, float rad) {
        float d = rad * 2.0f;
        path.AddArc(r.X, r.Y, d, d, 180, 90);
        path.AddArc(r.X + r.Width - d, r.Y, d, d, 270, 90);
        path.AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0, 90);
        path.AddArc(r.X, r.Y + r.Height - d, d, d, 90, 90);
        path.CloseFigure();
        };

    // 메인 카드 배경
    CRect contentRect(d.kCardMarginL, d.kCardMarginT, rc.Width() - d.kCardMarginR, rc.bottom - d.kCardMarginB);
    Gdiplus::RectF rf((float)contentRect.left, (float)contentRect.top, (float)contentRect.Width(), (float)contentRect.Height());
    for (int sh = 3; sh >= 1; sh--) {
        Gdiplus::RectF shRf(rf.X, rf.Y + (float)sh, rf.Width, rf.Height);
        Gdiplus::GraphicsPath shPath; RR(shPath, shRf, 12.0f);
        BYTE alpha = (BYTE)(sh == 3 ? 8 : sh == 2 ? 14 : 20);
        Gdiplus::SolidBrush shBrush(Gdiplus::Color(alpha, 10, 30, 70));
        g.FillPath(&shBrush, &shPath);
    }
    Gdiplus::GraphicsPath outerPath; RR(outerPath, rf, 12.0f);
    Gdiplus::SolidBrush whiteBrush(Gdiplus::Color::White);
    g.FillPath(&whiteBrush, &outerPath);

    // 내부 세션 카드들 (인쇄 설정, 기본 설정, 전표 메시지)
    auto DrawCard = [&](const CRect& rcSec, const wchar_t* title) {
        if (rcSec.IsRectEmpty()) return;
        Gdiplus::RectF cr((float)rcSec.left, (float)rcSec.top, (float)rcSec.Width(), (float)rcSec.Height());
        for (int sh = 1; sh <= 2; sh++) {
            Gdiplus::RectF sr(cr.X, cr.Y + (float)sh * 1.5f, cr.Width, cr.Height);
            Gdiplus::GraphicsPath sp; RR(sp, sr, 12.0f);
            Gdiplus::SolidBrush sb(Gdiplus::Color(8, 0, 0, 0));
            g.FillPath(&sb, &sp);
        }
        Gdiplus::GraphicsPath cp; RR(cp, cr, 12.0f);
        Gdiplus::SolidBrush cardBr(Gdiplus::Color(255, 250, 251, 253));
        g.FillPath(&cardBr, &cp);

        // 타이틀 바 (파란색 세로선)
        float hdrH = (float)SX(IsCompactScreen() ? 36 : 44);
        Gdiplus::SolidBrush barBr(Gdiplus::Color(255, 0, 96, 210));
        {
            const float barX = cr.X + 16.0f, barW = 4.0f, barH = 14.0f, barR = 2.0f;
            const float barY = cr.Y + (hdrH - barH) * 0.5f;
            const float bd = barR * 2.0f;
            Gdiplus::GraphicsPath barPath;
            barPath.AddArc(barX,          barY,          bd, bd, 180, 90);
            barPath.AddArc(barX + barW - bd, barY,          bd, bd, 270, 90);
            barPath.AddArc(barX + barW - bd, barY + barH - bd, bd, bd,   0, 90);
            barPath.AddArc(barX,          barY + barH - bd, bd, bd,  90, 90);
            barPath.CloseFigure();
            g.FillPath(&barBr, &barPath);
        }

        HDC hdc = g.GetHDC();
        ::SetBkMode(hdc, TRANSPARENT);
        HFONT hOld = (HFONT)::SelectObject(hdc, m_hFontCardTitle);
        ::SetTextColor(hdc, RGB(26, 32, 44));
        RECT rcT = { (LONG)(cr.X + 26.0f), (LONG)cr.Y, (LONG)(cr.X + cr.Width), (LONG)(cr.Y + hdrH) };
        ::DrawTextW(hdc, title, -1, &rcT, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        ::SelectObject(hdc, hOld);
        g.ReleaseHDC(hdc);

        // --- [여기부터 추가] 카드 타이틀 아래 옅은 회색 가로선 그리기 ---
        Gdiplus::Pen linePen(Gdiplus::Color(255, 238, 241, 247), 1.0f);
        float lineY = cr.Y + hdrH;
        g.DrawLine(&linePen, cr.X + 16.0f, lineY, cr.X + cr.Width - 16.0f, lineY);
        //
        };

    DrawCard(d.rcCard1, L"출력 설정");
    DrawCard(d.rcCard2, L"장치 설정");
    DrawCard(d.rcCard3, L"전표 메시지");
}

void CSlipSetupDlg::OnPaint()
{
    CPaintDC dc(this);
    CRect cl; GetClientRect(&cl);
    CDC mem; mem.CreateCompatibleDC(&dc);
    CBitmap bmp; bmp.CreateCompatibleBitmap(&dc, cl.Width(), cl.Height());
    CBitmap* pOld = mem.SelectObject(&bmp);

    mem.FillSolidRect(cl, RGB(249, 250, 252));
    DrawBackground(&mem);

    SlipLayoutData d; CalcLayout(m_hWnd, cl, d);
    BOOL bCmp = IsCompactScreen();

    // 헤더 그리기
    ModernUIHeader::Draw(mem.GetSafeHdc(), (float)(d.kCardMarginL + SX(14)), (float)(d.kCardMarginT + SX(16)), (float)SX(bCmp ? 36 : 44),
        ModernUIHeader::IconType::Receipt, L"전표 설정", L"전표 출력 옵션 및 메시지를 설정합니다",
        m_hFontHdrTitle, m_hFontHdrSub, d.kCardMarginL + SX(6), d.kCardMarginT + SX(bCmp ? 60 : 74),
        cl.Width() - d.kCardMarginR - SX(6), bCmp ? 23.0f : 26.0f, bCmp ? 3.0f : 0.0f);

    // 라벨 텍스트
    mem.SetBkMode(TRANSPARENT);
    mem.SetTextColor(RGB(115, 125, 142));
    mem.SelectObject(&m_fontLabel);
    CRect r1(d.inX, d.lY1, d.inX + d.col2W, d.lY1 + SX(d.capH));
    mem.DrawText(_T("프린터 속도"), &r1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    CRect r2(d.inX + d.col2W + SX(d.cG), d.lY1, d.inX + d.inW, d.lY1 + SX(d.capH));
    mem.DrawText(_T("프린터 포트번호"), &r2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    CRect r3(d.inX, d.lY2, d.inX + d.col2W, d.lY2 + SX(d.capH));
    mem.DrawText(_T("전표 매수"), &r3, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // --- [추가할 부분] OnPaint() 함수 하단, dc.BitBlt(...) 바로 위쪽 ---
    // 전표 매수, 프린터 속도 등을 그렸던 코드 바로 밑에 이어서 작성하세요.

    for (int r = 0; r < 3; r++) {
        int msgRowH = SX(d.capH) + SX(d.capG) + SX(d.FIELD_H) + SX(d.rG);
        int my = d.msgStartY + r * msgRowH;

        CRect rLeft(d.inX, my, d.inX + d.col2W, my + SX(d.capH));
        CRect rRight(d.inX + d.col2W + SX(d.cG), my, d.inX + d.inW, my + SX(d.capH));

        CString sLeft, sRight;
        sLeft.Format(_T("MSG %d"), r * 2 + 1);
        sRight.Format(_T("MSG %d"), r * 2 + 2);

        mem.DrawText(sLeft, &rLeft, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        mem.DrawText(sRight, &rRight, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }


    dc.BitBlt(0, 0, cl.Width(), cl.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(pOld);
}

BOOL CSlipSetupDlg::OnEraseBkgnd(CDC*) { return TRUE; }

HBRUSH CSlipSetupDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    static CBrush s_brCard(RGB(250, 251, 253));
    if (nCtlColor == CTLCOLOR_DLG) return m_brBg;
    if (pWnd && pWnd->GetDlgCtrlID() == IDC_SLIP_STATIC_ERR_PORT) {
        pDC->SetTextColor(RGB(220, 53, 69));
        pDC->SetBkMode(OPAQUE);
        pDC->SetBkColor(RGB(250, 251, 253));
        return s_brCard;
    }
    if (nCtlColor == CTLCOLOR_STATIC || nCtlColor == CTLCOLOR_EDIT) {
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(RGB(80, 90, 100));
        return s_brCard;
    }
    return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CSlipSetupDlg::OnDestroy() { KillTimer(TIMER_HOVER); CDialog::OnDestroy(); }
void CSlipSetupDlg::OnTimer(UINT_PTR nIDEvent) { if (nIDEvent == TIMER_HOVER) UpdateInputHoverByCursor(); CDialog::OnTimer(nIDEvent); }
void CSlipSetupDlg::UpdateInputHoverByCursor() { /* Hover 효과 필요 시 구현 */ }

void CSlipSetupDlg::OnInfoBtnClicked(UINT nID)
{
    if (nID == IDC_SLIP_BTN_PRINT_INFO) ShowInfoPopover(m_btnPrintInfo, _T("전표 출력 매수"), _T("결제 시 출력할 전표 매수를 설정"));
    else if (nID == IDC_SLIP_BTN_PORT_INFO) ShowInfoPopover(m_btnPortInfo, _T("프린터 포트번호"), _T("프린터가 연결된 COM 포트 번호를 입력"));
    else if (nID == IDC_SLIP_BTN_SPEED_INFO) ShowInfoPopover(m_btnSpeedInfo, _T("프린터 속도"), _T("프린터 통신 속도 선택"));
    else if (nID == IDC_SLIP_BTN_MSG_INFO) ShowInfoPopover(m_btnMsgInfo, _T("전표 메시지"), _T("전표 하단에 출력할 안내 메시지 입력\n※MSG당 최대 48bytes"));
}
void CSlipSetupDlg::ShowInfoPopover(CInfoIconButton& btn, LPCTSTR szTitle, LPCTSTR szBody)
{
    CRect rc; btn.GetWindowRect(&rc);
    m_popover.ShowAt(rc, szTitle, szBody, this);
}

void CSlipSetupDlg::LoadFromRegistry()
{
    static LPCTSTR SEC = _T("SERIALPORT");
    CString s;

    // 전표 출력 활성화
    s = AfxGetApp()->GetProfileString(SEC, _T("PRINTER_CHECK"), _T("0"));
    m_chkPrintEnable.SetToggled(s == _T("1"));

    // 프린터 포트번호
    s = AfxGetApp()->GetProfileString(SEC, _T("PRINTER"), _T(""));
    m_editPort.SetWindowText(s);

    // 전표 매수 (1장=index 0, 2장=index 1)
    s = AfxGetApp()->GetProfileString(SEC, _T("PRINTER_SLIP_NUM"), _T("1"));
    {
        CString target = s + _T("장");
        int found = -1;
        for (int i = 0; i < m_comboPrintCount.GetCount(); i++) {
            CString item; m_comboPrintCount.GetLBText(i, item);
            if (item.CompareNoCase(target) == 0) { found = i; break; }
        }
        m_comboPrintCount.SetCurSel(found >= 0 ? found : 0);
    }

    // 프린터 속도: 저장값(숫자)에 "bps" 붙여 콤보 항목과 비교 후 선택
    s = AfxGetApp()->GetProfileString(SEC, _T("PRINTER_SPEED"), _T("57600"));
    {
        CString target = s + _T("bps");
        int found = -1;
        for (int i = 0; i < m_comboSpeed.GetCount(); i++) {
            CString item; m_comboSpeed.GetLBText(i, item);
            if (item.CompareNoCase(target) == 0) { found = i; break; }
        }
        m_comboSpeed.SetCurSel(found >= 0 ? found : 2); // 기본값 57600bps(index 2)
    }

    // 전표 메시지 MSG1~MSG6
    static LPCTSTR msgFields[] = {
        _T("PRINTER_MSG1"), _T("PRINTER_MSG2"), _T("PRINTER_MSG3"),
        _T("PRINTER_MSG4"), _T("PRINTER_MSG5"), _T("PRINTER_MSG6")
    };
    for (int i = 0; i < 6; i++) {
        s = AfxGetApp()->GetProfileString(SEC, msgFields[i], _T(""));
        m_editMsg[i].SetWindowText(s);
    }
}
void CSlipSetupDlg::SaveToRegistry()
{
    static LPCTSTR SEC = _T("SERIALPORT");
    CString s;

    // 전표 출력 활성화: ON="1", OFF="0"
    AfxGetApp()->WriteProfileString(SEC, _T("PRINTER_CHECK"),
        m_chkPrintEnable.IsToggled() ? _T("1") : _T("0"));

    // 프린터 포트번호
    m_editPort.GetWindowText(s);
    AfxGetApp()->WriteProfileString(SEC, _T("PRINTER"), s);

    // 전표 매수
    // 전표 매수: 콤보 텍스트에서 "장" 제거 후 숫자만 저장
    {
        int idx = m_comboPrintCount.GetCurSel();
        if (idx >= 0) {
            m_comboPrintCount.GetLBText(idx, s);
            s.Replace(_T("장"), _T(""));
            s.TrimRight();
        } else { s = _T("1"); }
        AfxGetApp()->WriteProfileString(SEC, _T("PRINTER_SLIP_NUM"), s);
    }

    // 프린터 속도: 콤보 텍스트에서 "bps" 제거 후 숫자만 저장
    {
        int idx = m_comboSpeed.GetCurSel();
        if (idx >= 0) {
            m_comboSpeed.GetLBText(idx, s);
            s.Replace(_T("bps"), _T(""));
            s.TrimRight();
        } else { s = _T("57600"); }
        AfxGetApp()->WriteProfileString(SEC, _T("PRINTER_SPEED"), s);
    }

    // 전표 메시지 MSG1~MSG6
    static LPCTSTR msgFields[] = {
        _T("PRINTER_MSG1"), _T("PRINTER_MSG2"), _T("PRINTER_MSG3"),
        _T("PRINTER_MSG4"), _T("PRINTER_MSG5"), _T("PRINTER_MSG6")
    };
    for (int i = 0; i < 6; i++) {
        m_editMsg[i].GetWindowText(s);
        AfxGetApp()->WriteProfileString(SEC, msgFields[i], s);
    }
}
void CSlipSetupDlg::OnBtnOk()
{
    if (m_popover.GetSafeHwnd()) m_popover.Hide();
    if (!ValidateInputs()) return;
    SaveToRegistry();
    EndDialog(IDOK);
}
void CSlipSetupDlg::OnBtnCancel()
{
    if (m_popover.GetSafeHwnd()) m_popover.Hide();
    if (HasChanges()) {
        int ret = CModernMessageBox::Question(_T("변경사항이 있습니다.\n저장하지 않고 닫으시겠습니까?"), this);
        if (ret != IDYES) return;
    }
    EndDialog(IDCANCEL);
}
void CSlipSetupDlg::OnBtnLastPrint() { AfxMessageBox(_T("직전거래 전표를 출력합니다.")); }
void CSlipSetupDlg::UpdateControlsState()
{
    BOOL bOn = m_chkPrintEnable.IsToggled();
    m_comboPrintCount.EnableWindow(bOn);
    m_editPort.EnableWindow(bOn);
    m_comboSpeed.EnableWindow(bOn);
    for (int i = 0; i < 6; i++) m_editMsg[i].EnableWindow(bOn);
    UpdatePortValidationUI();
}
void CSlipSetupDlg::OnPrintEnableToggled() { UpdateControlsState(); }
void CSlipSetupDlg::OnOK() {}
void CSlipSetupDlg::OnCancel() { EndDialog(IDCANCEL); }

BOOL CSlipSetupDlg::ValidateInputs()
{
    if (m_chkPrintEnable.IsToggled()) {
        CString sPort;
        m_editPort.GetWindowText(sPort);
        sPort.Trim();
        if (sPort.IsEmpty()) {
            UpdatePortValidationUI();
            CModernMessageBox::Warning(_T("입력값을 확인해주세요."), this);
            m_editPort.SetFocus();
            ((CEdit*)GetDlgItem(IDC_SLIP_EDIT_PORT))->SetSel(0, -1);
            return FALSE;
        }
    }
    UpdatePortValidationUI();
    return TRUE;
}

void CSlipSetupDlg::OnSlipPortChanged()
{
    if (!m_bUiInitialized) return;
    UpdatePortValidationUI();
}

void CSlipSetupDlg::UpdatePortValidationUI()
{
    if (!m_bUiInitialized) return;
    BOOL bShowError = FALSE;
    if (m_chkPrintEnable.IsToggled()) {
        CString s; m_editPort.GetWindowText(s); s.Trim();
        bShowError = s.IsEmpty();
    }
    CWnd* pErr = GetDlgItem(IDC_SLIP_STATIC_ERR_PORT);
    if (!pErr || !pErr->GetSafeHwnd()) return;
    CString cur; pErr->GetWindowText(cur);
    BOOL bVisNow = pErr->IsWindowVisible();
    if (bShowError) {
        if (cur != _T("입력 필요")) pErr->SetWindowText(_T("입력 필요"));
        if (!bVisNow) { pErr->ShowWindow(SW_SHOW); pErr->Invalidate(FALSE); }
    } else {
        if (!cur.IsEmpty()) pErr->SetWindowText(_T(""));
        if (bVisNow) pErr->ShowWindow(SW_HIDE);
    }
}

BOOL CSlipSetupDlg::HasChanges()
{
    static LPCTSTR SEC = _T("SERIALPORT");
    CString sReg, sCur;

    sReg = AfxGetApp()->GetProfileString(SEC, _T("PRINTER_CHECK"), _T("0"));
    sCur = m_chkPrintEnable.IsToggled() ? _T("1") : _T("0");
    if (sReg != sCur) return TRUE;

    sReg = AfxGetApp()->GetProfileString(SEC, _T("PRINTER"), _T(""));
    m_editPort.GetWindowText(sCur);
    if (sReg != sCur) return TRUE;

    sReg = AfxGetApp()->GetProfileString(SEC, _T("PRINTER_SLIP_NUM"), _T("1"));
    { int idx = m_comboPrintCount.GetCurSel();
      if (idx >= 0) { m_comboPrintCount.GetLBText(idx, sCur); sCur.Replace(_T("장"), _T("")); sCur.TrimRight(); }
      else { sCur = _T("1"); }
      if (sReg != sCur) return TRUE; }

    sReg = AfxGetApp()->GetProfileString(SEC, _T("PRINTER_SPEED"), _T("57600"));
    { int idx = m_comboSpeed.GetCurSel();
      if (idx >= 0) { m_comboSpeed.GetLBText(idx, sCur); sCur.Replace(_T("bps"), _T("")); sCur.TrimRight(); }
      else { sCur = _T("57600"); }
      if (sReg != sCur) return TRUE; }

    static LPCTSTR msgFields[] = { _T("PRINTER_MSG1"), _T("PRINTER_MSG2"), _T("PRINTER_MSG3"),
        _T("PRINTER_MSG4"), _T("PRINTER_MSG5"), _T("PRINTER_MSG6") };
    for (int i = 0; i < 6; i++) {
        sReg = AfxGetApp()->GetProfileString(SEC, msgFields[i], _T(""));
        m_editMsg[i].GetWindowText(sCur);
        if (sReg != sCur) return TRUE;
    }
    return FALSE;
}
