#include "stdafx.h"
#include "resource.h"
#include "KeyinDlg.h"
#include "RegistryUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace {
    static const COLORREF kDlgBg      = RGB(245, 247, 250);
    static const COLORREF kCardFill   = RGB(255, 255, 255);
    static const COLORREF kCardBorder = RGB(214, 228, 247);
    static const COLORREF kDispBg     = RGB(235, 244, 255);
    static const COLORREF kTextBlue   = RGB(0, 100, 221);
    static const COLORREF kTextDark   = RGB(18, 24, 40);
    static const COLORREF kTitleColor = RGB(6, 52, 109);

    static const LPCTSTR SEC_SERIALPORT      = _T("SERIALPORT");
    static const LPCTSTR CANCEL_HOTKEY_FIELD = _T("CANCEL_HOTKEY");
    static const LPCTSTR MSR_HOTKEY_FIELD    = _T("MSR_HOTKEY");
}

HHOOK CKeyinDlg::s_hKbHook  = NULL;
HWND  CKeyinDlg::s_hWndTarget = NULL;

BEGIN_MESSAGE_MAP(CKeyinDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_WM_WINDOWPOSCHANGING()
    ON_COMMAND_RANGE(IDC_KEYIN_BTN_0, IDC_KEYIN_BTN_CANCEL, OnBtnClicked)
    ON_CONTROL_RANGE(BN_DBLCLK, IDC_KEYIN_BTN_0, IDC_KEYIN_BTN_CANCEL, OnBtnClicked)
    ON_MESSAGE(WM_KEYIN_HOOK_KEY, OnHookKey)
END_MESSAGE_MAP()

CKeyinDlg::CKeyinDlg(int nKind, CWnd* pParent)
    : CDialog(CKeyinDlg::IDD, pParent)
    , m_keyinKind(nKind)
    , m_bCursorVisible(TRUE)
    , m_bInputEnabled(FALSE)
    , m_brDlgBg(kDlgBg)
{
}

CKeyinDlg::~CKeyinDlg()
{
}

void CKeyinDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

int CKeyinDlg::SX(int v) const
{
    return ModernUIDpi::Scale(m_hWnd, v);
}

void CKeyinDlg::EnsureFonts()
{
    if (m_fontTitle.GetSafeHandle()) return;
    ModernUIFont::EnsureFontsLoaded();

    LOGFONT lf = {};
    lf.lfCharSet = HANGUL_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    ModernUIFont::ApplyUIFontFace(lf);

    lf.lfHeight = -SX(16); lf.lfWeight = FW_BOLD;
    m_fontTitle.CreateFontIndirect(&lf);

    lf.lfHeight = -SX(14); lf.lfWeight = FW_NORMAL;
    m_fontBtn.CreateFontIndirect(&lf);
}

void CKeyinDlg::LoadHotkeySettings()
{
    if (!GetRegisterData(SEC_SERIALPORT, CANCEL_HOTKEY_FIELD, m_strCancelHotkey))
        m_strCancelHotkey = _T("NORMAL");
    if (!GetRegisterData(SEC_SERIALPORT, MSR_HOTKEY_FIELD, m_strMsrHotkey))
        m_strMsrHotkey = _T("NORMAL");
}

BOOL CKeyinDlg::HotkeyMatchesVK(const CString& strKey, UINT nVK) const
{
    struct { LPCTSTR name; UINT vk; } tbl[] = {
        {_T("VK_F1"),VK_F1},{_T("VK_F2"),VK_F2},{_T("VK_F3"),VK_F3},{_T("VK_F4"),VK_F4},
        {_T("VK_F5"),VK_F5},{_T("VK_F6"),VK_F6},{_T("VK_F7"),VK_F7},{_T("VK_F8"),VK_F8},
        {_T("VK_F9"),VK_F9},{_T("VK_F10"),VK_F10},{_T("VK_F11"),VK_F11},{_T("VK_F12"),VK_F12},
        {_T("VK_ESCAPE"),VK_ESCAPE},{_T("VK_RETURN"),VK_RETURN},{_T("VK_SPACE"),VK_SPACE},
        {_T("VK_TAB"),VK_TAB},{_T("VK_BACK"),VK_BACK},{_T("VK_INSERT"),VK_INSERT},
        {_T("VK_DELETE"),VK_DELETE},{_T("VK_HOME"),VK_HOME},{_T("VK_END"),VK_END},
        {_T("VK_PRIOR"),VK_PRIOR},{_T("VK_NEXT"),VK_NEXT},
        {_T("VK_UP"),VK_UP},{_T("VK_DOWN"),VK_DOWN},
        {_T("VK_LEFT"),VK_LEFT},{_T("VK_RIGHT"),VK_RIGHT},
    };
    if (strKey.CompareNoCase(_T("NORMAL")) == 0) return FALSE;
    for (auto& e : tbl)
        if (strKey.CompareNoCase(e.name) == 0 && nVK == e.vk) return TRUE;
    return FALSE;
}

int CKeyinDlg::GetMaxInputLen() const
{
    switch (m_keyinKind) {
    case 1: return 21;
    case 2: return 11;
    case 3: return 6;
    }
    return 20;
}

int CKeyinDlg::CountDigits() const
{
    int n = 0;
    for (int i = 0; i < m_strInputData.GetLength(); ++i)
        if (_istdigit(m_strInputData[i])) n++;
    return n;
}

void CKeyinDlg::UpdateOkButtonState()
{
    if (!::IsWindow(m_btnOk.GetSafeHwnd())) return;
    int digits = CountDigits();
    BOOL bEnable = FALSE;
    switch (m_keyinKind) {
    case 1: bEnable = (digits >= 15); break;
    case 2: bEnable = (digits >= 10); break;
    case 3: bEnable = (digits >= 4);  break;
    }
    m_btnOk.EnableWindow(bEnable);
}

CString CKeyinDlg::FormatDisplay() const
{
    if (m_strInputData.IsEmpty()) return _T("");

    if (m_keyinKind == 1) {
        // 카드번호: = 이전 4자리마다 -, = 이후 그대로
        int eqPos = m_strInputData.Find(_T('='));
        CString before = (eqPos >= 0) ? m_strInputData.Left(eqPos) : m_strInputData;
        CString after  = (eqPos >= 0) ? m_strInputData.Mid(eqPos)  : _T("");
        CString result;
        for (int i = 0; i < before.GetLength(); ++i) {
            if (i > 0 && i % 4 == 0) result += _T('-');
            result += before[i];
        }
        result += after;
        return result;
    }
    else if (m_keyinKind == 2) {
        // 휴대폰: 010-xxxx-xxxx (3-4-4), 사업자: 123-45-67890 (3-2-5)
        CString d = m_strInputData;
        int len = d.GetLength();
        if (len == 0) return _T("");
        BOOL bPhone = (d[0] == _T('0'));
        CString result;
        if (bPhone) {
            // 3-4-4 or 3-3-4
            if (len <= 3) {
                result = d;
            } else if (len <= 7) {
                result = d.Left(3) + _T("-") + d.Mid(3);
            } else {
                result = d.Left(3) + _T("-") + d.Mid(3, 4) + _T("-") + d.Mid(7);
            }
        } else {
            // 사업자 3-2-5
            if (len <= 3) {
                result = d;
            } else if (len <= 5) {
                result = d.Left(3) + _T("-") + d.Mid(3);
            } else {
                result = d.Left(3) + _T("-") + d.Mid(3, 2) + _T("-") + d.Mid(5);
            }
        }
        return result;
    }
    else {
        // 비밀번호: 마스킹
        CString result;
        for (int i = 0; i < m_strInputData.GetLength(); ++i)
            result += _T('*');
        return result;
    }
}

void CKeyinDlg::AppendDigit(TCHAR ch)
{
    if (m_strInputData.GetLength() >= GetMaxInputLen()) return;
    m_strInputData += ch;
    m_bCursorVisible = TRUE;
    UpdateOkButtonState();
    UpdateDisplay();
}

void CKeyinDlg::DeleteLast()
{
    if (m_strInputData.IsEmpty()) return;
    m_strInputData = m_strInputData.Left(m_strInputData.GetLength() - 1);
    m_bCursorVisible = TRUE;
    UpdateOkButtonState();
    UpdateDisplay();
}

void CKeyinDlg::UpdateDisplay()
{
    Invalidate(FALSE);
}

BOOL CKeyinDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    ModernUIGfx::EnsureGdiplusStartup();
    EnsureFonts();
    LoadHotkeySettings();

    // 타이틀 설정
    switch (m_keyinKind) {
    case 1: SetWindowText(_T("카드번호 입력")); break;
    case 2: SetWindowText(_T("번호 입력")); break;
    case 3: SetWindowText(_T("비밀번호 입력")); break;
    }

    const COLORREF underlayW = kCardFill;

    // 숫자 버튼 1-9 (RC 서브클래싱)
    for (int i = 1; i <= 9; ++i) {
        m_btnDigit[i].SubclassDlgItem(IDC_KEYIN_BTN_0 + i, this);
        m_btnDigit[i].SetButtonStyle(ButtonStyle::Default);
        m_btnDigit[i].SetUnderlayColor(underlayW);
        m_btnDigit[i].SetTextPx(20);
    }
    // 0 버튼
    m_btnDigit[0].SubclassDlgItem(IDC_KEYIN_BTN_0, this);
    m_btnDigit[0].SetButtonStyle(ButtonStyle::Default);
    m_btnDigit[0].SetUnderlayColor(underlayW);
    m_btnDigit[0].SetTextPx(20);

    // 삭제 버튼
    m_btnDel.SubclassDlgItem(IDC_KEYIN_BTN_DEL, this);
    m_btnDel.SetButtonStyle(ButtonStyle::Default);
    m_btnDel.SetUnderlayColor(underlayW);
    m_btnDel.SetFont(&m_fontBtn);

    // 특수 버튼 (= / MSR 전환 / 숨김)
    m_btnSpec.SubclassDlgItem(IDC_KEYIN_BTN_SPEC, this);
    m_btnSpec.SetButtonStyle(ButtonStyle::Default);
    m_btnSpec.SetUnderlayColor(underlayW);
    m_btnSpec.SetFont(&m_fontBtn);
    switch (m_keyinKind) {
    case 1: m_btnSpec.SetWindowText(_T("=")); break;
    case 2: m_btnSpec.SetWindowText(_T("MSR 전환")); break;
    case 3: m_btnSpec.ShowWindow(SW_HIDE); break;
    }

    // 확인 버튼 (Primary)
    m_btnOk.SubclassDlgItem(IDC_KEYIN_BTN_OK, this);
    m_btnOk.SetButtonStyle(ButtonStyle::Primary);
    m_btnOk.SetUnderlayColor(underlayW);
    m_btnOk.SetFont(&m_fontBtn);
    m_btnOk.EnableWindow(FALSE);

    // 취소 버튼 (Default)
    m_btnCancel.SubclassDlgItem(IDC_KEYIN_BTN_CANCEL, this);
    m_btnCancel.SetButtonStyle(ButtonStyle::Default);
    m_btnCancel.SetUnderlayColor(underlayW);
    m_btnCancel.SetFont(&m_fontBtn);

    // 창 크기 고정
    int clientW = SX(360);
    int clientH = SX(460);
    RECT rcW, rcC;
    ::GetWindowRect(m_hWnd, &rcW);
    ::GetClientRect(m_hWnd, &rcC);
    int ncW = (rcW.right - rcW.left) - (rcC.right - rcC.left);
    int ncH = (rcW.bottom - rcW.top) - (rcC.bottom - rcC.top);
    SetWindowPos(NULL, 0, 0, clientW + ncW, clientH + ncH,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    LayoutControls();
    CenterWindow();

    ::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SetForegroundWindow();
    ModernUIWindow::ApplyWhiteTitleBar(m_hWnd);
    SetTimer(2,300, NULL);

    UpdateOkButtonState();
    UpdateDisplay();
    return TRUE;
}

void CKeyinDlg::LayoutControls()
{
    if (!::IsWindow(m_hWnd)) return;

    CRect cl; GetClientRect(&cl);

    const int outerPad = SX(16);
    const int cardL = outerPad, cardT = outerPad;
    const int cardW = cl.Width() - outerPad * 2;
    const int cardH = cl.Height() - outerPad * 2;

    const int innerPad = SX(18);

    // 버튼 그리드 계산
    const int btnGap  = SX(8);
    const int numpadL = cardL + innerPad;
    const int numpadW = cardW - innerPad * 2;
    const int btnW    = (numpadW - btnGap * 2) / 3;
    const int btnH    = SX(50);
    const int bottomBtnH = SX(48);

    // 레이아웃 Y 위치 계산 (위에서 아래로)
    // 타이틀: cardT + innerPad (텍스트, 컨트롤 없음)
    // 표시 영역: OnPaint에서 그림 (컨트롤 없음)
    // numpad top
    int numpadTopY = cardT + SX(120); // 타이틀+표시영역 공간

    // Row 0: [1][2][3]
    // Row 1: [4][5][6]
    // Row 2: [7][8][9]
    // Row 3: [SPEC][0][DEL]
    int digitMap[4][3] = {
        {1,2,3}, {4,5,6}, {7,8,9}, {-1,0,-2}  // -1=SPEC, -2=DEL
    };

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 3; ++col) {
            int id = digitMap[row][col];
            int x = numpadL + col * (btnW + btnGap);
            int y = numpadTopY + row * (btnH + btnGap);
            if (id >= 0) {
                m_btnDigit[id].MoveWindow(x, y, btnW, btnH);
            } else if (id == -1) {
                m_btnSpec.MoveWindow(x, y, btnW, btnH);
            } else if (id == -2) {
                m_btnDel.MoveWindow(x, y, btnW, btnH);
            }
        }
    }

    // 하단 버튼 (확인 + 취소)
    int bottomY = cardT + cardH - innerPad - bottomBtnH;
    int okW     = numpadW * 2 / 3 - btnGap / 2;
    int cancelW = numpadW - okW - btnGap;
    m_btnOk.MoveWindow(numpadL, bottomY, okW, bottomBtnH);
    m_btnCancel.MoveWindow(numpadL + okW + btnGap, bottomY, cancelW, bottomBtnH);

    Invalidate(FALSE);
}

void CKeyinDlg::DrawDisplayText(HDC hdc, const CRect& rcDisplay, const CString& strFormatted)
{
    // 커서를 제외한 텍스트만 측정/렌더링하고 커서는 별도 처리
    // → 커서 깜빡여도 텍스트 위치가 변하지 않음

    int availW = rcDisplay.Width() - SX(16);
    if (availW <= 0) return;

    // 텍스트 UTF-16 변환
    int wlenT = ::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)strFormatted, -1, NULL, 0);
    std::vector<wchar_t> wbufT((size_t)(wlenT > 0 ? wlenT : 1), 0);
    int charCountT = 0;
    if (wlenT > 1) {
        ::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)strFormatted, -1, wbufT.data(), wlenT);
        charCountT = wlenT - 1;
    }

    // 폰트 크기 결정 (텍스트+커서 너비 기준으로 축소)
    int fontSize = SX(22);
    const int minFontSize = SX(12);

    LOGFONT lf = {};
    lf.lfCharSet = HANGUL_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    ModernUIFont::ApplyUIFontFace(lf);
    lf.lfWeight = FW_BOLD;

    SIZE szText = {0, SX(20)};

    for (int attempt = 0; attempt < 5; ++attempt) {
        lf.lfHeight = -fontSize;
        HFONT hTmp = ::CreateFontIndirect(&lf);
        HFONT hOld2 = (HFONT)::SelectObject(hdc, hTmp);
        if (charCountT > 0)
            ::GetTextExtentPoint32W(hdc, wbufT.data(), charCountT, &szText);
        ::SelectObject(hdc, hOld2);
        ::DeleteObject(hTmp);

        int totalW = szText.cx;
        if (totalW <= availW || fontSize <= minFontSize) break;
        fontSize = max(minFontSize, (int)((float)fontSize * availW / totalW) - 1);
    }

    lf.lfHeight = -fontSize;
    HFONT hFont = ::CreateFontIndirect(&lf);
    HFONT hOld = (HFONT)::SelectObject(hdc, hFont);

    // 커서 너비 항상 예약 → 텍스트 중앙 위치 고정
    int reservedW = szText.cx;
    int startX = rcDisplay.left + (rcDisplay.Width() - reservedW) / 2;
    int startY = rcDisplay.top + (rcDisplay.Height() - szText.cy) / 2;

    ::SetBkMode(hdc, TRANSPARENT);

    // 텍스트 렌더링
    if (charCountT > 0) {
        ::SetTextColor(hdc, (m_keyinKind == 3) ? kTextDark : kTextBlue);
        ::TextOutW(hdc, startX, startY, wbufT.data(), charCountT);
    }

    ::SelectObject(hdc, hOld);
    ::DeleteObject(hFont);
}

BOOL CKeyinDlg::OnEraseBkgnd(CDC*) { return TRUE; }

void CKeyinDlg::OnPaint()
{
    CPaintDC dc(this);
    EnsureFonts();

    CRect cl; GetClientRect(&cl);
    CDC mem; mem.CreateCompatibleDC(&dc);
    CBitmap bmp; bmp.CreateCompatibleBitmap(&dc, cl.Width(), cl.Height());
    CBitmap* pOld = mem.SelectObject(&bmp);

    mem.FillSolidRect(cl, kDlgBg);

    const int outerPad = SX(16);
    const int innerPad = SX(18);
    CRect rcCard(outerPad, outerPad,
        cl.Width() - outerPad, cl.Height() - outerPad);

    {
        ModernUIGfx::EnsureGdiplusStartup();
        Gdiplus::Graphics g(mem.GetSafeHdc());
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

        for (int sh = 2; sh >= 1; sh--) {
            Gdiplus::GraphicsPath sp;
            CRect sr = rcCard; sr.OffsetRect(0, sh);
            ModernUIGfx::AddRoundRect(sp,
                Gdiplus::RectF((Gdiplus::REAL)sr.left, (Gdiplus::REAL)sr.top,
                    (Gdiplus::REAL)sr.Width(), (Gdiplus::REAL)sr.Height()), (Gdiplus::REAL)SX(16));
            Gdiplus::SolidBrush sb(Gdiplus::Color((BYTE)(sh == 2 ? 10 : 18), 0, 30, 80));
            g.FillPath(&sb, &sp);
        }

        Gdiplus::GraphicsPath cardPath;
        ModernUIGfx::AddRoundRect(cardPath,
            Gdiplus::RectF((Gdiplus::REAL)rcCard.left, (Gdiplus::REAL)rcCard.top,
                (Gdiplus::REAL)rcCard.Width(), (Gdiplus::REAL)rcCard.Height()), (Gdiplus::REAL)SX(16));
        Gdiplus::SolidBrush fillBr(Gdiplus::Color(255, 255, 255, 255));
        Gdiplus::Pen borderPen(Gdiplus::Color(255,
            GetRValue(kCardBorder), GetGValue(kCardBorder), GetBValue(kCardBorder)), 1.0f);
        g.FillPath(&fillBr, &cardPath);
        g.DrawPath(&borderPen, &cardPath);

        int dispL = rcCard.left + innerPad;
        int dispW = rcCard.Width() - innerPad * 2;
        int dispT = rcCard.top + SX(60);
        int dispH = SX(52);
        CRect rcDisp(dispL, dispT, dispL + dispW, dispT + dispH);
        Gdiplus::GraphicsPath dispPath;
        ModernUIGfx::AddRoundRect(dispPath,
            Gdiplus::RectF((Gdiplus::REAL)rcDisp.left, (Gdiplus::REAL)rcDisp.top,
                (Gdiplus::REAL)rcDisp.Width(), (Gdiplus::REAL)rcDisp.Height()), (Gdiplus::REAL)SX(8));
        Gdiplus::SolidBrush dispBr(Gdiplus::Color(255,
            GetRValue(kDispBg), GetGValue(kDispBg), GetBValue(kDispBg)));
        g.FillPath(&dispBr, &dispPath);
    }

    HDC hRaw = mem.GetSafeHdc();
    ::SetBkMode(hRaw, TRANSPARENT);

    // 타이틀 텍스트
    {
        LPCTSTR szTitle = _T("");
        switch (m_keyinKind) {
        case 1: szTitle = _T("카드번호를 입력해주세요"); break;
        case 2: szTitle = _T("번호를 입력해주세요"); break;
        case 3: szTitle = _T("비밀번호를 입력해주세요"); break;
        }
        HFONT hOld = (HFONT)::SelectObject(hRaw, m_fontTitle.GetSafeHandle());
        ::SetTextColor(hRaw, kTitleColor);
        RECT rcTitle = {
            rcCard.left + innerPad, rcCard.top + SX(22),
            rcCard.right - innerPad, rcCard.top + SX(22) + SX(24)
        };
        // UTF-16 변환
        int wlen = ::MultiByteToWideChar(CP_ACP, 0, szTitle, -1, NULL, 0);
        std::vector<wchar_t> wbuf((size_t)wlen, 0);
        ::MultiByteToWideChar(CP_ACP, 0, szTitle, -1, wbuf.data(), wlen);
        ::DrawTextW(hRaw, wbuf.data(), wlen-1, &rcTitle,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        ::SelectObject(hRaw, hOld);
    }

    // 표시 영역 텍스트 (자동 축소)
    {
        int dispL = rcCard.left + innerPad;
        int dispW = rcCard.Width() - innerPad * 2;
        int dispT = rcCard.top + SX(60);
        int dispH = SX(52);
        CRect rcDisp(dispL, dispT, dispL + dispW, dispT + dispH);
        CString strFormatted = FormatDisplay();
        DrawDisplayText(hRaw, rcDisp, strFormatted);
    }

    dc.BitBlt(0, 0, cl.Width(), cl.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(pOld);
}

HBRUSH CKeyinDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC) {
        pDC->SetBkMode(TRANSPARENT);
        return m_brDlgBg;
    }
    return hbr;
}

void CKeyinDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 2) {
        KillTimer(2);
        s_hWndTarget = m_hWnd;
        s_hKbHook = ::SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
        m_bInputEnabled = TRUE;
    }
    if (nIDEvent == 1) {
        m_bCursorVisible = !m_bCursorVisible;
        Invalidate(FALSE);
    }
    CDialog::OnTimer(nIDEvent);
}

void CKeyinDlg::OnBtnClicked(UINT nID)
{
    if (nID >= IDC_KEYIN_BTN_1 && nID <= IDC_KEYIN_BTN_9) {
        AppendDigit(_T('0') + (TCHAR)(nID - IDC_KEYIN_BTN_0));
        return;
    }
    if (nID == IDC_KEYIN_BTN_0) {
        AppendDigit(_T('0'));
        return;
    }
    if (nID == IDC_KEYIN_BTN_DEL) {
        DeleteLast();
        return;
    }
    if (nID == IDC_KEYIN_BTN_SPEC) {
        if (m_keyinKind == 1) {
            // = 삽입 (한 번만)
            if (m_strInputData.Find(_T('=')) < 0)
                AppendDigit(_T('='));
        } else if (m_keyinKind == 2) {
            EndDialog(RET_MSR);
        }
        return;
    }
    if (nID == IDC_KEYIN_BTN_OK) {
        OnOK();
        return;
    }
    if (nID == IDC_KEYIN_BTN_CANCEL) {
        OnCancel();
        return;
    }
}

BOOL CKeyinDlg::PreTranslateMessage(MSG* pMsg)
{
    if (!m_bInputEnabled) {
        if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) return TRUE;
        return CDialog::PreTranslateMessage(pMsg);
    }
    if (pMsg->message == WM_KEYDOWN) {
        UINT nVK = (UINT)pMsg->wParam;

        // 숫자 입력 (상단 숫자키)
        if (nVK >= '0' && nVK <= '9') {
            AppendDigit((TCHAR)nVK);
            return TRUE;
        }
        // 텐키패드
        if (nVK >= VK_NUMPAD0 && nVK <= VK_NUMPAD9) {
            AppendDigit((TCHAR)('0' + nVK - VK_NUMPAD0));
            return TRUE;
        }
        // = 키 (Mode 1)
        if (m_keyinKind == 1 && (nVK == VK_OEM_PLUS || nVK == (UINT)'=')) {
            if (m_strInputData.Find(_T('=')) < 0)
                AppendDigit(_T('='));
            return TRUE;
        }
        // 기본 키 (항상 동작)
        if (nVK == VK_BACK)   { DeleteLast(); return TRUE; }
        if (nVK == VK_RETURN) { OnOK();       return TRUE; }
        if (nVK == VK_ESCAPE) { EndDialog(RET_CANCEL); return TRUE; }

        // 외부 핫키
        if (HotkeyMatchesVK(m_strCancelHotkey, nVK)) {
            EndDialog(RET_CANCEL); return TRUE;
        }
        if (m_keyinKind == 2 && HotkeyMatchesVK(m_strMsrHotkey, nVK)) {
            EndDialog(RET_MSR); return TRUE;
        }
    }
    return CDialog::PreTranslateMessage(pMsg);
}

void CKeyinDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    if (!(lpwndpos->flags & SWP_NOZORDER))
        lpwndpos->hwndInsertAfter = HWND_TOPMOST;
    CDialog::OnWindowPosChanging(lpwndpos);
}

LRESULT CALLBACK CKeyinDlg::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && s_hWndTarget && ::IsWindow(s_hWndTarget))
    {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
            UINT vk = p->vkCode;

            BOOL bHandle =
                (vk >= '0' && vk <= '9') ||
                (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) ||
                (vk >= VK_F1 && vk <= VK_F12) ||
                vk == VK_BACK || vk == VK_RETURN || vk == VK_ESCAPE ||
                vk == VK_DELETE || vk == VK_INSERT ||
                vk == VK_HOME || vk == VK_END ||
                vk == VK_PRIOR || vk == VK_NEXT ||
                vk == VK_UP || vk == VK_DOWN ||
                vk == VK_LEFT || vk == VK_RIGHT ||
                vk == VK_OEM_PLUS || vk == VK_ADD;

            if (bHandle)
            {
                ::PostMessage(s_hWndTarget, WM_KEYIN_HOOK_KEY, (WPARAM)vk, 0);
                return 1;
            }
        }
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
        {
            KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
            UINT vk = p->vkCode;
            BOOL bHandle =
                (vk >= '0' && vk <= '9') ||
                (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) ||
                (vk >= VK_F1 && vk <= VK_F12) ||
                vk == VK_BACK || vk == VK_RETURN || vk == VK_ESCAPE ||
                vk == VK_OEM_PLUS || vk == VK_ADD;
            if (bHandle) return 1;
        }
    }
    return ::CallNextHookEx(s_hKbHook, nCode, wParam, lParam);
}

LRESULT CKeyinDlg::OnHookKey(WPARAM wParam, LPARAM lParam)
{
    UINT nVK = (UINT)wParam;
    if (nVK >= '0' && nVK <= '9') {
        AppendDigit((TCHAR)nVK);
        return 0;
    }
    if (nVK >= VK_NUMPAD0 && nVK <= VK_NUMPAD9) {
        AppendDigit((TCHAR)('0' + nVK - VK_NUMPAD0));
        return 0;
    }
    if (m_keyinKind == 1 && (nVK == VK_OEM_PLUS || nVK == VK_ADD)) {
        if (m_strInputData.Find(_T('=')) < 0)
            AppendDigit(_T('='));
        return 0;
    }
    if (nVK == VK_BACK)   { DeleteLast();          return 0; }
    if (nVK == VK_RETURN) { OnOK();                return 0; }
    if (nVK == VK_ESCAPE) { EndDialog(RET_CANCEL); return 0; }
    if (HotkeyMatchesVK(m_strCancelHotkey, nVK)) {
        EndDialog(RET_CANCEL); return 0;
    }
    if (m_keyinKind == 2 && HotkeyMatchesVK(m_strMsrHotkey, nVK)) {
        EndDialog(RET_MSR); return 0;
    }
    return 0;
}

void CKeyinDlg::OnOK()
{
    if (m_strInputData.IsEmpty()) return;
    m_cardnum = m_strInputData;
    if (s_hKbHook) { ::UnhookWindowsHookEx(s_hKbHook); s_hKbHook = NULL; s_hWndTarget = NULL; }
    KillTimer(1);
    CDialog::OnOK();
}

void CKeyinDlg::OnCancel()
{
    if (s_hKbHook) { ::UnhookWindowsHookEx(s_hKbHook); s_hKbHook = NULL; s_hWndTarget = NULL; }
    KillTimer(1);
    EndDialog(RET_CANCEL);
}

void CKeyinDlg::OnDestroy()
{
    if (s_hKbHook) {
        ::UnhookWindowsHookEx(s_hKbHook);
        s_hKbHook = NULL;
        s_hWndTarget = NULL;
    }
    KillTimer(1);
    KillTimer(2);
    CDialog::OnDestroy();
}
