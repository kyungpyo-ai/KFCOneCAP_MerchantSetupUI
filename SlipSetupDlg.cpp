// SlipSetupDlg.cpp
#include "stdafx.h"
#include "SlipSetupDlg.h"

IMPLEMENT_DYNAMIC(CSlipSetupDlg, CDialog)

static BOOL IsCompactScreen() { return ::GetSystemMetrics(SM_CYSCREEN) <= 800; }

BEGIN_MESSAGE_MAP(CSlipSetupDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_WM_NCHITTEST()
    ON_BN_CLICKED(IDC_SLIP_BTN_OK,         OnBtnOk)
    ON_BN_CLICKED(IDC_SLIP_BTN_CANCEL,     OnBtnCancel)
    ON_BN_CLICKED(IDC_SLIP_BTN_CLOSE,      OnBtnClose)
    ON_BN_CLICKED(IDC_SLIP_BTN_LAST_PRINT, OnBtnLastPrint)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_SLIP_BTN_PRINT_INFO, IDC_SLIP_BTN_SPEED_INFO, OnInfoBtnClicked)
END_MESSAGE_MAP()

CSlipSetupDlg::CSlipSetupDlg(CWnd* pParent)
    : CDialog(IDD_SLIP_SETUP_DLG, pParent)
    , m_hFontCardTitle(nullptr)
    , m_hFontHdrTitle(nullptr)
    , m_hFontHdrSub(nullptr)
    , m_uHoverTimer(0)
    , m_bUiInitialized(FALSE)
{}

CSlipSetupDlg::~CSlipSetupDlg() {}

void CSlipSetupDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

int CSlipSetupDlg::SX(int px) const { return ModernUIDpi::Scale(m_hWnd, px); }

void CSlipSetupDlg::SetClientSize(int cx, int cy)
{
    CRect rcWnd, rcCli;
    GetWindowRect(&rcWnd); GetClientRect(&rcCli);
    int ncW = rcWnd.Width()  - rcCli.Width();
    int ncH = rcWnd.Height() - rcCli.Height();
    SetWindowPos(nullptr, 0, 0, cx + max(0, ncW), cy + max(0, ncH), SWP_NOMOVE | SWP_NOZORDER);
}

LRESULT CSlipSetupDlg::OnNcHitTest(CPoint point)
{
    LRESULT hit = CDialog::OnNcHitTest(point);
    if (hit == HTCLIENT) return HTCAPTION;
    return hit;
}

void CSlipSetupDlg::EnsureFonts()
{
    if (m_fontTitle.GetSafeHandle()) return;
    LOGFONT lf = { 0 };
    ::GetObject((HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    ModernUIFont::ApplyUIFontFace(lf);

    lf.lfHeight = -SX(20); lf.lfWeight = FW_BOLD;
    m_fontTitle.CreateFontIndirect(&lf);

    lf.lfHeight = -SX(13); lf.lfWeight = FW_NORMAL;
    m_fontSubtitle.CreateFontIndirect(&lf);

    lf.lfHeight = -SX(13); lf.lfWeight = FW_BOLD;
    lf.lfQuality = CLEARTYPE_QUALITY;
    m_fontSection.CreateFontIndirect(&lf);

    lf.lfHeight = -SX(IsCompactScreen() ? 13 : 14); lf.lfWeight = FW_BOLD;
    m_fontLabel.CreateFontIndirect(&lf);

    lf.lfHeight = -SX(IsCompactScreen() ? 12 : 14); lf.lfWeight = FW_MEDIUM;
    m_fontCombo.CreateFontIndirect(&lf);

    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfHeight = -SX(18); lf.lfWeight = FW_BOLD;
    m_hFontHdrTitle = ::CreateFontIndirect(&lf);

    lf.lfHeight = -SX(13); lf.lfWeight = FW_BOLD;
    m_hFontHdrSub = ::CreateFontIndirect(&lf);

    lf.lfHeight = -SX(15); lf.lfWeight = FW_BOLD;
    m_hFontCardTitle = ::CreateFontIndirect(&lf);
}

BOOL CSlipSetupDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_BORDER | WS_DLGFRAME,
                WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE | WS_EX_STATICEDGE, 0);
    ModernUIGfx::EnsureGdiplusStartup();

    m_brBg.CreateSolidBrush(RGB(249, 250, 252));
    m_brWhite.CreateSolidBrush(RGB(255, 255, 255));
    EnsureFonts();
    {
        BOOL bCmp = IsCompactScreen();
        const int FIELD_H = SX(bCmp ? 32 : 40);
        const int capH    = SX(bCmp ? 16 : 18);
        const int rG      = SX(bCmp ? 12 : 20);
        const int cPadY   = SX(bCmp ? 10 : 16);
        const int cGapY   = SX(bCmp ?  8 : 12);
        const int cHdrH   = SX(bCmp ? 36 : 44);
        const int toggleH = SX(bCmp ? 28 : 32);
        const int btnH    = SX(bCmp ? 32 : 40);
        const int outerT  = SX(bCmp ?  6 : 10);
        const int divY0   = outerT + SX(bCmp ? 54 : 74);
        const int s1y = divY0 + cGapY;
        const int s1h = cHdrH + cPadY + toggleH + cPadY;
        const int s2y = s1y + s1h + cGapY;
        const int s2h = cHdrH + cPadY + capH + FIELD_H + rG + capH + FIELD_H + cPadY;
        const int s3y = s2y + s2h + cGapY;
        const int s3h = cHdrH + cPadY + FIELD_H * 3 + rG * 2 + cPadY;
        const int fdY = s3y + s3h + SX(bCmp ? 8 : 10);
        const int bY  = fdY + SX(bCmp ? 8 : 10);
        int dlgH = bY + btnH + SX(bCmp ? 14 : 20);
        int dlgW = SX(bCmp ? 520 : 600);
        int maxH = ::GetSystemMetrics(SM_CYMAXIMIZED) - 40;
        if (dlgH > maxH) dlgH = maxH;
        SetWindowPos(nullptr, 0, 0, dlgW, dlgH, SWP_NOMOVE | SWP_NOZORDER);
    }

    // Close btn
    m_btnClose.Create(_T("X"), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        CRect(0,0,0,0), this, IDC_SLIP_BTN_CLOSE);
    m_btnClose.SetButtonStyle(ButtonStyle::Auto);
    m_btnClose.SetColors(RGB(249,250,252), RGB(235,237,240), RGB(100,114,132));
    m_btnClose.SetHoverTextColor(RGB(25,31,40));
    m_btnClose.SetUnderlayColor(RGB(255,255,255));
    m_btnClose.SetTextPx(SX(14));

    // Toggle - 출력 활성화
    m_chkPrintEnable.Create(_T("출력 활성화 됨"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        CRect(0,0,0,0), this, IDC_SLIP_PRINT_ENABLE);
    m_chkPrintEnable.SetCheck(BST_CHECKED);
    m_chkPrintEnable.SetUnderlayColor(RGB(250,251,253));
    m_chkPrintEnable.SetFont(&m_fontLabel);

    // [핵심 해결책] MFC CBT Hook 크래시를 우회하는 안전한 콤보박스 생성 헬퍼
    auto CreateSafeCombo = [&](CSkinnedComboBox& combo, UINT id) {
        HWND hCombo = ::CreateWindowEx(0, _T("COMBOBOX"), _T(""),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS,
            0, 0, 0, 0, this->GetSafeHwnd(), (HMENU)(UINT_PTR)id, AfxGetInstanceHandle(), nullptr);
        if (hCombo) {
            combo.SubclassWindow(hCombo); // WM_CREATE가 완전히 끝난 안전한 상태에서 서브클래싱!
        }
        };

    // Combo - 전표 출력 매수
    CreateSafeCombo(m_comboPrintCount, IDC_SLIP_COMBO_PRINT_COUNT);
    m_comboPrintCount.AddString(_T("1장"));
    m_comboPrintCount.AddString(_T("2장"));
    m_comboPrintCount.AddString(_T("3장"));
    m_comboPrintCount.AddString(_T("4장"));
    m_comboPrintCount.AddString(_T("5장"));
    m_comboPrintCount.SetCurSel(1);
    m_comboPrintCount.SetFont(&m_fontCombo);
    m_comboPrintCount.SetUnderlayColor(RGB(255, 255, 255));

    // Combo - 프린터 속도
    CreateSafeCombo(m_comboSpeed, IDC_SLIP_COMBO_SPEED);
    m_comboSpeed.AddString(_T("9600bps"));
    m_comboSpeed.AddString(_T("19200bps"));
    m_comboSpeed.AddString(_T("38400bps"));
    m_comboSpeed.AddString(_T("57600bps"));
    m_comboSpeed.AddString(_T("115200bps"));
    m_comboSpeed.SetCurSel(4);
    m_comboSpeed.SetFont(&m_fontCombo);
    m_comboSpeed.SetUnderlayColor(RGB(255, 255, 255));

    // Edit - 프린터 포트번호
    m_editPort.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT,
        CRect(0,0,0,0), this, IDC_SLIP_EDIT_PORT);
    m_editPort.SetWindowText(_T("0"));
    m_editPort.SetFont(&m_fontCombo);
    m_editPort.SetUnderlayColor(RGB(255,255,255));


    // MSG edits
    static const WCHAR* kPlaceholders[6] = {
        L"MSG 1  메시지를 입력하세요",
        L"MSG 2", L"MSG 3", L"MSG 4", L"MSG 5", L"MSG 6"
    };
    static const int kIds[6] = {
        IDC_SLIP_EDIT_MSG1, IDC_SLIP_EDIT_MSG2, IDC_SLIP_EDIT_MSG3,
        IDC_SLIP_EDIT_MSG4, IDC_SLIP_EDIT_MSG5, IDC_SLIP_EDIT_MSG6
    };
    for (int i = 0; i < 6; i++) {
        m_editMsg[i].Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT,
            CRect(0,0,0,0), this, kIds[i]);
        ::SendMessage(m_editMsg[i].GetSafeHwnd(), EM_SETCUEBANNER, FALSE, (LPARAM)kPlaceholders[i]);
        m_editMsg[i].SetFont(&m_fontCombo);
        m_editMsg[i].SetUnderlayColor(RGB(255,255,255));
    }

    // Info buttons
    auto CreateInfo = [&](CInfoIconButton& btn, UINT id) {
        btn.Create(_T(""), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            CRect(0,0,SX(20),SX(20)), this, id);
    };
    CreateInfo(m_btnPrintInfo, IDC_SLIP_BTN_PRINT_INFO);
    CreateInfo(m_btnPortInfo,  IDC_SLIP_BTN_PORT_INFO);
    CreateInfo(m_btnSpeedInfo, IDC_SLIP_BTN_SPEED_INFO);

    // 직전거래 전표출력 버튼
    m_btnLastPrint.Create(_T("직전거래 전표출력"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        CRect(0,0,0,0), this, IDC_SLIP_BTN_LAST_PRINT);
    m_btnLastPrint.SetButtonStyle(ButtonStyle::Auto);
    m_btnLastPrint.SetColors(RGB(249,250,252), RGB(237,239,242), RGB(55,65,81));
    m_btnLastPrint.SetUnderlayColor(RGB(255,255,255));
    m_btnLastPrint.SetFont(&m_fontLabel);

    // 취소 버튼
    m_btnCancel.Create(_T("취소"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        CRect(0,0,0,0), this, IDC_SLIP_BTN_CANCEL);
    m_btnCancel.SetButtonStyle(ButtonStyle::Auto);
    m_btnCancel.SetColors(RGB(242,244,246), RGB(228,232,236), RGB(78,89,104));
    m_btnCancel.SetUnderlayColor(RGB(255,255,255));
    m_btnCancel.SetFont(&m_fontLabel);

    // 확인 버튼
    m_btnOk.Create(_T("확인"),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        CRect(0,0,0,0), this, IDC_SLIP_BTN_OK);
    m_btnOk.SetButtonStyle(ButtonStyle::Auto);
    m_btnOk.SetColors(RGB(27,100,242), RGB(20,90,220), RGB(255,255,255));
    m_btnOk.SetUnderlayColor(RGB(255,255,255));
    m_btnOk.SetFont(&m_fontLabel);

    LoadFromRegistry();
    LayoutControls();
    CenterWindow();
    m_bUiInitialized = TRUE;
    m_uHoverTimer = SetTimer(TIMER_HOVER, 16, nullptr);
    return TRUE;
}

void CSlipSetupDlg::LayoutControls()
{
    CRect rc; GetClientRect(&rc);
    int cw = rc.Width();

    m_btnClose.MoveWindow(cw - SX(8) - SX(14) - SX(28), SX(8) + SX(12), SX(28), SX(28));

    // Section 1 (인쇄 설정): card y=96, h=96  (hdrH=44 + 14 + toggle28 + 10)
    m_chkPrintEnable.MoveWindow(SX(28), SX(96) + SX(44) + SX(14), SX(220), SX(28));

    // Section 2 (기본 설정): card y=208, h=208  (hdrH=44 + top16 + rows + bot16)
    int halfW = (cw - SX(56)) / 2;
    int s2y        = SX(208);
    int row1LabelY = s2y + SX(44) + SX(16);
    int row1CtrlY  = row1LabelY + SX(20);
    int row2LabelY = row1CtrlY  + SX(36) + SX(20);
    int row2CtrlY  = row2LabelY + SX(20);
    int infoSz = SX(20);

    m_btnPrintInfo.MoveWindow(SX(28) + SX(80) + SX(2), row1LabelY, infoSz, infoSz);
    m_comboPrintCount.MoveWindow(SX(28), row1CtrlY, halfW, SX(36));
    m_btnPortInfo.MoveWindow(SX(28) + halfW + SX(16) + SX(88) + SX(2), row1LabelY, infoSz, infoSz);
    m_editPort.MoveWindow(SX(28) + halfW + SX(16), row1CtrlY, halfW, SX(36));

    m_btnSpeedInfo.MoveWindow(SX(28) + SX(64) + SX(2), row2LabelY, infoSz, infoSz);
    m_comboSpeed.MoveWindow(SX(28), row2CtrlY, halfW, SX(36));

    // Section 3 (전표 메시지): card y=432, h=228  (hdrH=44 + top16 + 3*(edit40+16) + bot16)
    int msgW1     = halfW;
    int msgW2     = cw - SX(56) - halfW;
    int msgStartY = SX(432) + SX(44) + SX(16);
    for (int r = 0; r < 3; r++) {
        int my = msgStartY + r * SX(56);
        m_editMsg[r*2    ].MoveWindow(SX(28), my, msgW1, SX(40));
        m_editMsg[r*2 + 1].MoveWindow(SX(28) + msgW1 + SX(12), my, msgW2, SX(40));
    }

    // Footer buttons: y=676
    int btnY = SX(676);
    int btnH = SX(40);
    m_btnLastPrint.MoveWindow(SX(20), btnY, SX(140), btnH);
    m_btnOk.MoveWindow(cw - SX(20) - SX(88), btnY, SX(88), btnH);
    m_btnCancel.MoveWindow(cw - SX(20) - SX(88) - SX(8) - SX(80), btnY, SX(80), btnH);
}

void CSlipSetupDlg::DrawBackground(CDC* pDC)
{
    CRect rc; GetClientRect(&rc);
    int cw = rc.Width(), ch = rc.Height();
    BOOL bCmp = IsCompactScreen();
    const int FIELD_H = SX(bCmp ? 32 : 40);
    const int capH    = SX(bCmp ? 16 : 18);
    const int rG      = SX(bCmp ? 12 : 20);
    const int cPadY   = SX(bCmp ? 10 : 16);
    const int cGapY   = SX(bCmp ?  8 : 12);
    const int cHdrH   = SX(bCmp ? 36 : 44);
    const int toggleH = SX(bCmp ? 28 : 32);
    const int btnH    = SX(bCmp ? 32 : 40);
    const int cardX   = SX(bCmp ? 12 : 20);
    const int ctrlX   = cardX + SX(8);
    const int outerT  = SX(bCmp ?  6 : 10);
    const int outerB  = SX(bCmp ? 12 : 20);
    const int divY    = outerT + SX(bCmp ? 54 : 74);
    const int s1y = divY + cGapY;
    const int s1h = cHdrH + cPadY + toggleH + cPadY;
    const int s2y = s1y + s1h + cGapY;
    const int s2h = cHdrH + cPadY + capH + FIELD_H + rG + capH + FIELD_H + cPadY;
    const int s3y = s2y + s2h + cGapY;
    const int s3h = cHdrH + cPadY + FIELD_H * 3 + rG * 2 + cPadY;
    const int footerDivY = s3y + s3h + SX(bCmp ? 8 : 10);
    const int btnY = footerDivY + SX(bCmp ? 8 : 10);
    const float kRadius = 12.0f;
    const float fL = (float)cardX, fT = (float)outerT;
    Gdiplus::RectF rf(fL, fT, (float)(cw - 2*cardX), (float)(ch - outerT - outerB));

    HDC hdc = pDC->GetSafeHdc();
    Gdiplus::Graphics g(hdc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    auto RR = [](Gdiplus::GraphicsPath& path, const Gdiplus::RectF& r, float rad) {
        const float d = rad * 2.0f;
        if (rad <= 0.0f) { path.AddRectangle(r); return; }
        Gdiplus::RectF arc(r.X, r.Y, d, d);
        path.AddArc(arc, 180, 90); arc.X = r.X + r.Width - d;
        path.AddArc(arc, 270, 90); arc.Y = r.Y + r.Height - d;
        path.AddArc(arc, 0, 90);   arc.X = r.X;
        path.AddArc(arc, 90, 90);  path.CloseFigure();
    };

    for (int sh = 3; sh >= 1; sh--) {
        Gdiplus::RectF shRf(rf.X, rf.Y + (float)sh, rf.Width, rf.Height);
        Gdiplus::GraphicsPath shPath; RR(shPath, shRf, kRadius);
        BYTE alpha = (BYTE)(sh == 3 ? 8 : sh == 2 ? 14 : 20);
        Gdiplus::SolidBrush shBrush(Gdiplus::Color(alpha, 10, 30, 70));
        g.FillPath(&shBrush, &shPath);
    }
    Gdiplus::GraphicsPath outerPath; RR(outerPath, rf, kRadius);
    Gdiplus::SolidBrush fillBrush(Gdiplus::Color(255, 255, 255, 255));
    g.FillPath(&fillBrush, &outerPath);

    const float hdrH = (float)cHdrH;
    const int cardYArr[3] = { s1y, s2y, s3y };
    const int cardHArr[3] = { s1h, s2h, s3h };
    const wchar_t* titles[3] = { L"인쇄 설정", L"기본 설정", L"전표 메시지" };

    for (int i = 0; i < 3; i++) {
        Gdiplus::RectF cr((float)cardX, (float)cardYArr[i], (float)(cw - 2*cardX), (float)cardHArr[i]);
        for (int sh = 1; sh <= 2; sh++) {
            Gdiplus::RectF sr(cr.X, cr.Y + (float)sh * 1.5f, cr.Width, cr.Height);
            Gdiplus::GraphicsPath sp; RR(sp, sr, 12.0f);
            Gdiplus::SolidBrush sb(Gdiplus::Color(8, 0, 0, 0));
            g.FillPath(&sb, &sp);
        }
        Gdiplus::GraphicsPath cp; RR(cp, cr, 12.0f);
        Gdiplus::SolidBrush cf(Gdiplus::Color(255, 250, 251, 253));
        g.FillPath(&cf, &cp);

        Gdiplus::Pen hl(Gdiplus::Color(255, 238, 241, 247), 1.0f);
        g.DrawLine(&hl,
            Gdiplus::PointF(cr.X + 16.0f, cr.Y + hdrH),
            Gdiplus::PointF(cr.X + cr.Width - 16.0f, cr.Y + hdrH));

        const float barX = cr.X + 16.0f, barW = 4.0f, barH = 14.0f, barR = 2.0f;
        const float barY = cr.Y + (hdrH - barH) * 0.5f, bd = barR * 2.0f;
        Gdiplus::GraphicsPath barPath;
        barPath.AddArc(barX, barY, bd, bd, 180, 90);
        barPath.AddArc(barX + barW - bd, barY, bd, bd, 270, 90);
        barPath.AddArc(barX + barW - bd, barY + barH - bd, bd, bd, 0, 90);
        barPath.AddArc(barX, barY + barH - bd, bd, bd, 90, 90);
        barPath.CloseFigure();
        Gdiplus::SolidBrush barBr(Gdiplus::Color(255, 0, 96, 210));
        g.FillPath(&barBr, &barPath);

        {
            HDC hdcCard = g.GetHDC();
            ::SetBkMode(hdcCard, TRANSPARENT);
            HFONT hOld = (HFONT)::SelectObject(hdcCard, m_hFontCardTitle);
            ::SetTextColor(hdcCard, RGB(26, 32, 44));
            RECT rcT = { (LONG)(barX + barW + 6.0f), (LONG)cr.Y,
                         (LONG)(cr.X + cr.Width - 16.0f), (LONG)(cr.Y + hdrH) };
            ::DrawTextW(hdcCard, titles[i], -1, &rcT, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
            ::SelectObject(hdcCard, hOld);
            g.ReleaseHDC(hdcCard);
        }
    }
}

void CSlipSetupDlg::OnPaint()
{
    CPaintDC dc(this);
    CRect cl; GetClientRect(&cl);
    int cw = cl.Width(), ch = cl.Height();

    CDC mem; mem.CreateCompatibleDC(&dc);
    CBitmap bmp; bmp.CreateCompatibleBitmap(&dc, cw, ch);
    CBitmap* pOld = mem.SelectObject(&bmp);

    mem.FillSolidRect(cl, RGB(249, 250, 252));
    EnsureFonts();

    DrawBackground(&mem);

    ModernUIHeader::Draw(mem.GetSafeHdc(),
        (float)SX(34), (float)SX(26), (float)SX(44),
        ModernUIHeader::IconType::CardTerminal,
        L"전표 출력 설정", L"단말기에서 전표 출력 옵션 및 메시지를 설정합니다",
        m_hFontHdrTitle, m_hFontHdrSub,
        SX(34), SX(84), cw - SX(34), 26.0f, 0.0f);

    mem.SetBkMode(TRANSPARENT);
    mem.SetTextColor(RGB(55, 65, 81));
    CFont* pLabelFont = mem.SelectObject(&m_fontLabel);
    {
        int halfW = (cw - SX(56)) / 2;
        int s2y        = SX(208);
        int row1LabelY = s2y + SX(44) + SX(16);
        int row1CtrlY  = row1LabelY + SX(20);
        int row2LabelY = row1CtrlY  + SX(36) + SX(20);

        CRect r1(SX(28), row1LabelY, SX(28) + halfW, row1LabelY + SX(18));
        mem.DrawText(_T("전표 매수"), &r1, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        CRect r2(SX(28) + halfW + SX(16), row1LabelY, SX(28) + halfW + SX(16) + halfW, row1LabelY + SX(18));
        mem.DrawText(_T("프린터 포트번호"), &r2, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        CRect r3(SX(28), row2LabelY, SX(28) + halfW, row2LabelY + SX(18));
        mem.DrawText(_T("프린터 속도"), &r3, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    mem.SelectObject(pLabelFont);

    {
        int badgeH = SX(20);
        int badgeX = SX(134);
        int badgeY = SX(432) + (SX(44) - badgeH) / 2;
        int badgeW = SX(92);
        {
            Gdiplus::Graphics gBadge(mem.GetSafeHdc());
            gBadge.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            Gdiplus::GraphicsPath bp;
            ModernUIGfx::AddRoundRect(bp,
                Gdiplus::RectF((Gdiplus::REAL)badgeX, (Gdiplus::REAL)badgeY,
                               (Gdiplus::REAL)badgeW, (Gdiplus::REAL)badgeH), 4.0f);
            Gdiplus::SolidBrush brBadge(Gdiplus::Color(255, 219, 234, 254));
            gBadge.FillPath(&brBadge, &bp);
        }
        mem.SetBkMode(TRANSPARENT);
        mem.SetTextColor(RGB(37, 99, 235));
        LOGFONT lfBadge = { 0 };
        m_fontSubtitle.GetLogFont(&lfBadge);
        lfBadge.lfWeight = FW_BOLD;
        CFont fBadge; fBadge.CreateFontIndirect(&lfBadge);
        CFont* pOldB = mem.SelectObject(&fBadge);
        CRect rcBadge(badgeX, badgeY, badgeX + badgeW, badgeY + badgeH);
        mem.DrawText(_T("최대 48bytes"), &rcBadge, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        mem.SelectObject(pOldB);
    }

    {
        Gdiplus::Graphics gDiv(mem.GetSafeHdc());
        Gdiplus::Pen penDiv(Gdiplus::Color(255, 229, 233, 240), 1.0f);
        gDiv.DrawLine(&penDiv, SX(20), SX(672), cw - SX(20), SX(672));
    }

    dc.BitBlt(0, 0, cw, ch, &mem, 0, 0, SRCCOPY);
    mem.SelectObject(pOld);
}


BOOL CSlipSetupDlg::OnEraseBkgnd(CDC* /*pDC*/) { return TRUE; }

HBRUSH CSlipSetupDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    pDC->SetBkMode(TRANSPARENT);
    int id = pWnd ? pWnd->GetDlgCtrlID() : 0;
    if (id == IDC_SLIP_PRINT_ENABLE) {
        pDC->SetBkColor(RGB(255, 255, 255));
        return m_brWhite;
    }
    pDC->SetBkColor(RGB(249, 250, 252));
    return m_brBg;
}

void CSlipSetupDlg::OnDestroy()
{
    KillTimer(TIMER_HOVER);
    if (m_hFontCardTitle) { ::DeleteObject(m_hFontCardTitle); m_hFontCardTitle = nullptr; }
    if (m_hFontHdrTitle) { ::DeleteObject(m_hFontHdrTitle); m_hFontHdrTitle = nullptr; }
    if (m_hFontHdrSub)   { ::DeleteObject(m_hFontHdrSub);   m_hFontHdrSub   = nullptr; }
    CDialog::OnDestroy();
}

void CSlipSetupDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_HOVER && m_bUiInitialized) UpdateInputHoverByCursor();
    CDialog::OnTimer(nIDEvent);
}

void CSlipSetupDlg::UpdateInputHoverByCursor() {}

void CSlipSetupDlg::ShowInfoPopover(CInfoIconButton& btn, LPCTSTR szTitle, LPCTSTR szBody)
{
    if (m_popover.GetSafeHwnd() && m_popover.IsVisible()) { m_popover.Hide(); return; }
    CRect rc;
    btn.GetWindowRect(&rc);
    m_popover.ShowAt(rc, szTitle, szBody, this);
}

void CSlipSetupDlg::OnInfoBtnClicked(UINT nID)
{
    if (nID == IDC_SLIP_BTN_PRINT_INFO)
        ShowInfoPopover(m_btnPrintInfo, _T("전표 출력 매수"), _T("1회 결제 시 출력할 전표 매수를 설정합니다."));
    else if (nID == IDC_SLIP_BTN_PORT_INFO)
        ShowInfoPopover(m_btnPortInfo, _T("프린터 포트번호"), _T("프린터가 연결된 COM 포트 번호를 입력합니다."));
    else if (nID == IDC_SLIP_BTN_SPEED_INFO)
        ShowInfoPopover(m_btnSpeedInfo, _T("프린터 속도"), _T("프린터와의 통신 속도(baud rate)를 설정합니다."));
}

void CSlipSetupDlg::LoadFromRegistry()
{
    CString strKey = _T("Software\\KFTC\\MerchantSetup\\SlipSetup");
    HKEY hKey = nullptr;
    if (::RegOpenKeyEx(HKEY_CURRENT_USER, strKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return;

    auto ReadDword = [&](LPCTSTR name, DWORD def) -> DWORD {
        DWORD val = def, sz = sizeof(DWORD), type = REG_DWORD;
        ::RegQueryValueEx(hKey, name, nullptr, &type, (LPBYTE)&val, &sz);
        return val;
    };
    auto ReadStr = [&](LPCTSTR name) -> CString {
        TCHAR buf[256] = {};
        DWORD sz = sizeof(buf), type = REG_SZ;
        ::RegQueryValueEx(hKey, name, nullptr, &type, (LPBYTE)buf, &sz);
        return CString(buf);
    };

    m_chkPrintEnable.SetCheck(ReadDword(_T("PrintEnable"), 1) ? BST_CHECKED : BST_UNCHECKED);
    int cnt = (int)ReadDword(_T("PrintCount"), 1);
    if (cnt >= 0 && cnt < 5) m_comboPrintCount.SetCurSel(cnt);
    CString strPort = ReadStr(_T("PrinterPort"));
    if (!strPort.IsEmpty()) m_editPort.SetWindowText(strPort);
    int spd = (int)ReadDword(_T("PrinterSpeed"), 4);
    if (spd >= 0 && spd < 5) m_comboSpeed.SetCurSel(spd);
    for (int i = 0; i < 6; i++) {
        CString key; key.Format(_T("Msg%d"), i+1);
        CString val = ReadStr(key);
        if (!val.IsEmpty()) m_editMsg[i].SetWindowText(val);
    }
    ::RegCloseKey(hKey);
}

void CSlipSetupDlg::SaveToRegistry()
{
    CString strKey = _T("Software\\KFTC\\MerchantSetup\\SlipSetup");
    HKEY hKey = nullptr;
    if (::RegCreateKeyEx(HKEY_CURRENT_USER, strKey, 0, nullptr, 0,
                         KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) return;

    auto WriteDword = [&](LPCTSTR name, DWORD val) {
        ::RegSetValueEx(hKey, name, 0, REG_DWORD, (const BYTE*)&val, sizeof(DWORD));
    };
    auto WriteStr = [&](LPCTSTR name, LPCTSTR val) {
        ::RegSetValueEx(hKey, name, 0, REG_SZ, (const BYTE*)val,
                        (DWORD)(_tcslen(val)+1)*sizeof(TCHAR));
    };

    WriteDword(_T("PrintEnable"), m_chkPrintEnable.GetCheck() == BST_CHECKED ? 1 : 0);
    WriteDword(_T("PrintCount"), (DWORD)m_comboPrintCount.GetCurSel());
    CString strPort; m_editPort.GetWindowText(strPort);
    WriteStr(_T("PrinterPort"), strPort);
    WriteDword(_T("PrinterSpeed"), (DWORD)m_comboSpeed.GetCurSel());
    for (int i = 0; i < 6; i++) {
        CString key; key.Format(_T("Msg%d"), i+1);
        CString val; m_editMsg[i].GetWindowText(val);
        WriteStr(key, val);
    }
    ::RegCloseKey(hKey);
}

void CSlipSetupDlg::OnBtnOk()       { SaveToRegistry(); EndDialog(IDOK); }
void CSlipSetupDlg::OnBtnCancel()   { EndDialog(IDCANCEL); }
void CSlipSetupDlg::OnBtnClose()    { EndDialog(IDCANCEL); }
void CSlipSetupDlg::OnBtnLastPrint(){ AfxMessageBox(_T("직전거래 전표를 출력합니다.")); }
void CSlipSetupDlg::OnOK()    {}
void CSlipSetupDlg::OnCancel() { EndDialog(IDCANCEL); }