#include "stdafx.h"
#include "resource.h"
#include "TransDlg.h"
#include "ModernMessageBox.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ============================================================
// 색상 팔레트
// ============================================================
// ============================================================
// 색상 팔레트
// ============================================================
namespace {
    // [FIX 1] ShopSetupDlg와 완벽히 동일한 색상/명도 위계로 동기화
    static const COLORREF kDlgBg = RGB(249, 250, 252);
    static const COLORREF kMainCardBorder = RGB(228, 232, 240);
    static const COLORREF kCardFill = RGB(250, 251, 253); // 내부 카드 배경 (살짝 회색)
    static const COLORREF kCardBorder = RGB(228, 232, 240);
    static const COLORREF kDivider = RGB(241, 244, 248);
    static const COLORREF kLabelText = RGB(115, 125, 142); // 가맹점 설정 라벨 색상
    static const COLORREF kValueText = RGB(26, 32, 44);  // 가맹점 설정 본문 색상
    static const COLORREF kBlueText = RGB(0, 76, 168); // 가맹점 설정 포인트 블루
    static const COLORREF kRedText = RGB(220, 53, 69);
    static const COLORREF kSectionText = RGB(26, 32, 44);

    static void AddRRP(Gdiplus::GraphicsPath& p, const CRect& rc, float r) {
        ModernUIGfx::AddRoundRect(p,
            Gdiplus::RectF((Gdiplus::REAL)rc.left, (Gdiplus::REAL)rc.top,
                (Gdiplus::REAL)rc.Width(), (Gdiplus::REAL)rc.Height()), r);
    }
    static void SafeShow(CWnd* p, BOOL b) {
        if (p && ::IsWindow(p->GetSafeHwnd()))
            p->ShowWindow(b ? SW_SHOW : SW_HIDE);
    }
    static int ParseAmountText(const CString& s)
    {
        CString digits;
        for (int i = 0; i < s.GetLength(); ++i)
            if (_istdigit(s[i])) digits += s[i];
        return digits.IsEmpty() ? 0 : _ttoi(digits);
    }
    static CString FormatAmountWithCommas(long long n)
    {
        if (n < 0) n = 0;
        CString s; s.Format(_T("%I64d"), n);
        int len = s.GetLength();
        CString result;
        for (int i = 0; i < len; ++i) {
            result += s[i];
            int rem = len - 1 - i;
            if (rem > 0 && rem % 3 == 0) result += _T(',');
        }
        return result;
    }
}

// ============================================================
// CSegmentCtrl 구현
// ============================================================
BEGIN_MESSAGE_MAP(CSegmentCtrl, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

BOOL CSegmentCtrl::Create(CWnd* pParent, UINT nID, const CRect& rc)
{
    return CWnd::Create(NULL, NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, rc, pParent, nID);
}
void CSegmentCtrl::AddTab(LPCTSTR text) { m_tabs.push_back(CString(text)); }

void CSegmentCtrl::SetCurSel(int n)
{
    if (n < 0 || n >= (int)m_tabs.size()) return;
    m_nSel = n;
    if (::IsWindow(m_hWnd)) { Invalidate(FALSE); UpdateWindow(); }
    NotifyParent();
}
void CSegmentCtrl::SetCurSelSilent(int n)
{
    if (n < 0 || n >= (int)m_tabs.size()) return;
    m_nSel = n;
    if (::IsWindow(m_hWnd)) { Invalidate(FALSE); UpdateWindow(); }
}
void CSegmentCtrl::NotifyParent()
{
    NMHDR nm = {};
    nm.hwndFrom = GetSafeHwnd();
    nm.idFrom = (UINT_PTR)GetDlgCtrlID();
    nm.code = TCN_SELCHANGE;
    CWnd* p = GetParent();
    if (p && p->GetSafeHwnd())
        p->SendMessage(WM_NOTIFY, (WPARAM)GetDlgCtrlID(), (LPARAM)&nm);
}
int CSegmentCtrl::HitTab(CPoint pt) const
{
    if (m_tabs.empty()) return -1;
    CRect cl; ::GetClientRect(m_hWnd, &cl);
    int n = (int)m_tabs.size();
    int pad = Scale(3);
    float tw = (cl.Width() - 2 * pad) / (float)n;
    for (int i = 0; i < n; i++) {
        int x1 = pad + (int)(i * tw);
        int x2 = pad + (int)((i + 1) * tw);
        if (pt.x >= x1 && pt.x < x2) return i;
    }
    return -1;
}
void CSegmentCtrl::OnLButtonDown(UINT nFlags, CPoint pt)
{
    int idx = HitTab(pt);
    if (idx >= 0) {
        m_nPress = idx;
        Invalidate(FALSE); UpdateWindow();
        if (idx != m_nSel) SetCurSel(idx);
    }
    CWnd::OnLButtonDown(nFlags, pt);
}
void CSegmentCtrl::OnLButtonUp(UINT nFlags, CPoint pt)
{
    if (m_nPress != -1) { m_nPress = -1; Invalidate(FALSE); }
    CWnd::OnLButtonUp(nFlags, pt);
}
void CSegmentCtrl::OnMouseMove(UINT nFlags, CPoint pt)
{
    int newHover = HitTab(pt);
    if (newHover != m_nHover) {
        m_nHover = newHover;
        Invalidate(FALSE);
    }
    TRACKMOUSEEVENT tme = {};
    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = m_hWnd;
    ::TrackMouseEvent(&tme);
    CWnd::OnMouseMove(nFlags, pt);
}
void CSegmentCtrl::OnMouseLeave()
{
    if (m_nHover != -1 || m_nPress != -1) {
        m_nHover = -1;
        m_nPress = -1;
        Invalidate(FALSE);
    }
}
BOOL CSegmentCtrl::OnSetCursor(CWnd*, UINT nHitTest, UINT)
{
    if (nHitTest == HTCLIENT) {
        ::SetCursor(::LoadCursor(NULL, IDC_HAND));
        return TRUE;
    }
    return FALSE;
}
void CSegmentCtrl::OnPaint()
{
    CPaintDC dc(this);
    CRect cl; GetClientRect(&cl);
    CDC mem; mem.CreateCompatibleDC(&dc);
    CBitmap bmp; bmp.CreateCompatibleBitmap(&dc, cl.Width(), cl.Height());
    CBitmap* pOld = mem.SelectObject(&bmp);

    // 언더레이 (카드 흰색)
    mem.FillSolidRect(cl, m_crUnderlay);

    ModernUIGfx::EnsureGdiplusStartup();
    Gdiplus::Graphics g(mem.GetSafeHdc());
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    if (m_tabs.empty()) {
        dc.BitBlt(0, 0, cl.Width(), cl.Height(), &mem, 0, 0, SRCCOPY);
        mem.SelectObject(pOld); return;
    }

    int n = (int)m_tabs.size();
    int pad = Scale(3);
    // [FIX 3] 탭 모서리를 조금 더 둥글게 처리
    int rO = Scale(12);  // 외부 pill radius
    int rI = Scale(10);   // 내부 active 탭 radius

    // 회색 pill 배경 (HTML: background:#EEEFF1, border-radius:12px, padding:4px)
    {
        Gdiplus::GraphicsPath bgP;
        ModernUIGfx::AddRoundRect(bgP,
            Gdiplus::RectF(0, 0, (Gdiplus::REAL)cl.Width(), (Gdiplus::REAL)cl.Height()),
            (Gdiplus::REAL)rO);
        Gdiplus::SolidBrush bgBr(Gdiplus::Color(255, 238, 239, 241));
        g.FillPath(&bgBr, &bgP);
    }

    float tw = (cl.Width() - 2 * pad) / (float)n;
    float th = (float)(cl.Height() - 2 * pad);

    // Hover / Press overlay for non-selected tabs
    for (int i = 0; i < n; i++) {
        if (i == m_nSel) continue;
        BYTE alpha = 0;
        if (i == m_nPress)      alpha = 45;
        else if (i == m_nHover) alpha = 22;
        if (alpha == 0) continue;
        Gdiplus::GraphicsPath hp;
        ModernUIGfx::AddRoundRect(hp,
            Gdiplus::RectF(pad + i * tw, (float)pad, tw, th),
            (Gdiplus::REAL)rI);
        Gdiplus::SolidBrush hb(Gdiplus::Color(alpha, 0, 0, 0));
        g.FillPath(&hb, &hp);
    }

    // 선택된 탭 흰색 pill (그림자 포함)
    {
        float tx = pad + m_nSel * tw;
        float ty = (float)pad;

        // [FIX 4] 탁한 그림자를 제거하고, 아주 연하고 부드러운 드롭 섀도우로 변경
        for (int sh = 4; sh >= 1; sh--) {
            Gdiplus::GraphicsPath sp;
            ModernUIGfx::AddRoundRect(sp,
                Gdiplus::RectF(tx, ty + (float)(sh * 0.5f), tw, th), // y축으로 살짝만 내림
                (Gdiplus::REAL)rI);
            // 투명도를 확 낮춰서 은은하게
            Gdiplus::SolidBrush sb(Gdiplus::Color((BYTE)(15 - sh * 3), 0, 0, 0));
            g.FillPath(&sb, &sp);
        }
        Gdiplus::GraphicsPath fp;
        ModernUIGfx::AddRoundRect(fp,
            Gdiplus::RectF(tx, ty, tw, th), (Gdiplus::REAL)rI);
        Gdiplus::Color pillClr(255, 255, 255, 255);
        if (m_nSel == m_nPress)      pillClr = Gdiplus::Color(255, 232, 238, 252);
        else if (m_nSel == m_nHover) pillClr = Gdiplus::Color(255, 245, 249, 255);
        Gdiplus::SolidBrush fb(pillClr);
        g.FillPath(&fb, &fp);
    }

    // 각 탭 텍스트
    if (!m_fontNormal.GetSafeHandle()) {
        const BOOL bCmpTab = ::GetSystemMetrics(SM_CYSCREEN) <= 800;
        LOGFONT lf = {};
        lf.lfCharSet = HANGUL_CHARSET;
        lf.lfQuality = CLEARTYPE_QUALITY;
        lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        ModernUIFont::ApplyUIFontFace(lf);
        int fH = -ModernUIDpi::Scale(m_hWnd, bCmpTab ? 13 : 14);
        lf.lfHeight = fH; lf.lfWeight = FW_NORMAL; m_fontNormal.CreateFontIndirect(&lf);
        lf.lfHeight = fH; lf.lfWeight = FW_BOLD;   m_fontBold.CreateFontIndirect(&lf);
    }

    HDC hdc = g.GetHDC();
    ::SetBkMode(hdc, TRANSPARENT);
    for (int i = 0; i < n; i++) {
        bool bActive = (i == m_nSel);

        float tx = pad + i * tw;
        float ty = (float)pad;

        // 유니코드 변환 (CP949 → UTF-16)
        int wlen = ::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)m_tabs[(size_t)i], -1, NULL, 0);
        std::vector<wchar_t> wbuf((size_t)wlen, 0);
        ::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)m_tabs[(size_t)i], -1, wbuf.data(), wlen);

        HFONT hFont = (HFONT)(bActive ? m_fontBold : m_fontNormal).GetSafeHandle();
        HFONT hOld = (HFONT)::SelectObject(hdc, hFont);
        SIZE sz = {};
        ::GetTextExtentPoint32W(hdc, wbuf.data(), wlen > 0 ? wlen - 1 : 0, &sz);

        int cx = (int)(tx + tw / 2 - sz.cx / 2);
        int cy2 = (int)(ty + th / 2 - sz.cy / 2);
        RECT rcT = { cx, cy2, cx + sz.cx + 2, cy2 + sz.cy };
        COLORREF crText = bActive ? kBlueText :
            (i == m_nPress) ? RGB(55, 68, 90) :
            (i == m_nHover) ? RGB(90, 102, 122) :
            RGB(140, 150, 165);
        ::SetTextColor(hdc, crText);
        ::DrawTextW(hdc, wbuf.data(), wlen > 0 ? wlen - 1 : 0, &rcT,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        ::SelectObject(hdc, hOld);
    }
    g.ReleaseHDC(hdc);

    dc.BitBlt(0, 0, cl.Width(), cl.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(pOld);
}

// ============================================================
// CTransDlg 메시지 맵
// ============================================================
BEGIN_MESSAGE_MAP(CTransDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_CTLCOLOR()
    ON_NOTIFY(TCN_SELCHANGE, IDC_TRANS_SEG, OnTabSelChange)
END_MESSAGE_MAP()
CTransDlg::CTransDlg(CWnd* pParent)
    : CDialog(CTransDlg::IDD, pParent)
    , m_eMode(MODE_CREDIT_APPROVAL), m_bUiBuilt(FALSE)
    , m_brBack(kDlgBg), m_bBadgeOk(FALSE)
{
}
CTransDlg::~CTransDlg() {}
void CTransDlg::DoDataExchange(CDataExchange* pDX) { CDialog::DoDataExchange(pDX); }
void CTransDlg::OnOK() {}
void CTransDlg::OnCancel() { EndDialog(IDCANCEL); }
int  CTransDlg::SX(int v) const { return ModernUIDpi::Scale(m_hWnd, v); }

void CTransDlg::EnsureFonts()
{
    if (m_fontTitle.GetSafeHandle()) return;
    ModernUIFont::EnsureFontsLoaded();
    LOGFONT lf = {};
    lf.lfCharSet = HANGUL_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    ModernUIFont::ApplyUIFontFace(lf);
    const BOOL bCmp = (::GetSystemMetrics(SM_CYSCREEN) <= 800);
#define MKF(px,wt,f) lf.lfHeight=-ModernUIDpi::Scale(m_hWnd,px); lf.lfWeight=wt; f.CreateFontIndirect(&lf)
    MKF((bCmp ? 15 : 18), FW_BOLD, m_fontTitle);
    MKF((bCmp ? 11 : 13), FW_BOLD, m_fontSub);
    MKF((bCmp ? 13 : 15), FW_BOLD, m_fontSection);
    MKF(14, FW_BOLD, m_fontLabel);
    MKF(14, FW_NORMAL, m_fontEdit);
    MKF(24, FW_EXTRABOLD, m_fontAmount);
    MKF((bCmp ? 12 : 14), FW_NORMAL, m_fontResultLabel);
    MKF((bCmp ? 12 : 14), FW_BOLD, m_fontResultValue);
    MKF((bCmp ? 12 : 14), FW_EXTRABOLD, m_fontResultBlue);
    MKF((bCmp ? 11 : 13), FW_BOLD, m_fontResultRed);
    MKF(10, FW_BOLD, m_fontBadge);
#undef MKF
}

void CTransDlg::GetContentRects(CRect& rcForm, CRect& rcResult) const
{
    CRect cl; ::GetClientRect(m_hWnd, &cl);

    // [FIX 1] ShopSetupDlg와 동일한 여백(Margin) 공식 적용
    BOOL bCmp = (::GetSystemMetrics(SM_CYSCREEN) <= 800);
    const int mL = ModernUIDpi::Scale(m_hWnd, bCmp ? 12 : 20);
    const int mT = ModernUIDpi::Scale(m_hWnd, bCmp ? 6 : 10);
    const int mR = ModernUIDpi::Scale(m_hWnd, bCmp ? 12 : 20);
    const int mB = ModernUIDpi::Scale(m_hWnd, bCmp ? 12 : 20);

    const int cp = ModernUIDpi::Scale(m_hWnd, 18);
    const int hdrH = ModernUIDpi::Scale(m_hWnd, bCmp ? 60 : 74), ig = ModernUIDpi::Scale(m_hWnd, 12);

    // 새 여백을 적용하여 메인 카드 영역 도출
    CRect rcMain(mL, mT, cl.right - mR, cl.bottom - mB);
    int cL = rcMain.left + cp, cR = rcMain.right - cp;
    int cTop = rcMain.top + hdrH + ig, cBot = rcMain.bottom - ig;
    int halfW = (cR - cL - ig) / 2;
    rcForm = CRect(cL, cTop, cL + halfW, cBot);
    rcResult = CRect(cL + halfW + ig, cTop, cR, cBot);
}

void CTransDlg::ResizeWindow()
{
    if (!::IsWindow(m_hWnd)) return;
    int kRCH = SX(44) + kNumResults * SX(kResRowH) + SX(14);

    BOOL bCmp = (::GetSystemMetrics(SM_CYSCREEN) <= 800);
    int mT = SX(bCmp ? 6 : 10);
    int mB = SX(bCmp ? 12 : 20);

    int clientH = mT + SX(bCmp ? 60 : 74) + SX(12) + kRCH + SX(12) + mB;
    RECT rcW, rcC;
    ::GetWindowRect(m_hWnd, &rcW); ::GetClientRect(m_hWnd, &rcC);
    int ncH = (rcW.bottom - rcW.top) - (rcC.bottom - rcC.top);
    int ncW = (rcW.right - rcW.left) - (rcC.right - rcC.left);

    // 창의 기본 가로 크기를 900 -> 920으로 늘려서 내부 내용물 공간은 그대로 보존!
    int windowW = bCmp ? SX(900) : SX(920);
    SetWindowPos(NULL, 0, 0, windowW + ncW, clientH + ncH, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL CTransDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    SetRedraw(FALSE);
    ModernUIGfx::EnsureGdiplusStartup();
    EnsureFonts();
    SetWindowText(_T("결제"));
    CreateSegmentControl();
    CreateInputControls();
    CreateResultControls();
    CreateBottomButton();
    ApplyFonts();
    m_eMode = MODE_CREDIT_APPROVAL;
    m_segCtrl.SetCurSelSilent(0);
    ShowFieldsForMode();
    m_edtSupply.SetNumericOnly(TRUE);
    m_edtTax.SetNumericOnly(TRUE);
    m_edtTip.SetNumericOnly(TRUE);
    m_edtTaxFree.SetNumericOnly(TRUE);
    m_edtInstall.SetNumericOnly(TRUE);
    m_edtSupply.SetUnitText(_T("원"), 30);
    m_edtTax.SetUnitText(_T("원"), 30);
    m_edtTip.SetUnitText(_T("원"), 30);
    m_edtTaxFree.SetUnitText(_T("원"), 30);
    m_edtSupply.LimitText(11);
    m_edtTax.LimitText(11);
    m_edtTip.LimitText(11);
    m_edtTaxFree.LimitText(11);
    m_edtInstall.SetUnitText(_T("개월"), 40);
    m_edtInstall.LimitText(2);
    m_edtSupply.SetWindowText(FormatAmountWithCommas(1000));
    m_edtTax.SetWindowText(FormatAmountWithCommas(100));
    m_edtTip.SetWindowText(FormatAmountWithCommas(0));
    m_edtTaxFree.SetWindowText(FormatAmountWithCommas(0));
    m_edtInstall.SetWindowText(_T("00"));
    m_tabValues[(int)MODE_CASH_APPROVAL][0] = FormatAmountWithCommas(1000);
    m_tabValues[(int)MODE_CASH_APPROVAL][1] = FormatAmountWithCommas(100);
    m_tabValues[(int)MODE_CASH_APPROVAL][2] = FormatAmountWithCommas(0);
    m_tabValues[(int)MODE_CASH_APPROVAL][3] = FormatAmountWithCommas(0);
    m_tabValues[(int)MODE_CASH_APPROVAL][4] = _T("00");
    m_bUiBuilt = TRUE;
    ResizeWindow();
    LayoutControls();

    CenterWindow();
    ModernUIWindow::ApplyWhiteTitleBar(this->GetSafeHwnd());
    SetRedraw(TRUE);
    RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    m_segCtrl.SetFocus();
    return FALSE;
}

void CTransDlg::CreateSegmentControl()
{
    m_segCtrl.SubclassDlgItem(IDC_TRANS_SEG, this);
    m_segCtrl.SetUnderlayColor(kDlgBg);
    m_segCtrl.AddTab(_T("신용승인"));
    m_segCtrl.AddTab(_T("신용취소"));
    m_segCtrl.AddTab(_T("현금 승인"));
    m_segCtrl.AddTab(_T("현금 취소"));
}

void CTransDlg::CreateInputControls()
{
    // [FIX 2] 입력 컨트롤 언더레이를 하얀색에서 내부 카드색(kCardFill)으로 통일
    const COLORREF W = kCardFill;
    m_edtSupply.SubclassDlgItem(IDC_TRANS_EDIT_SUPPLY, this);      m_edtSupply.SetUnderlayColor(W);
    m_edtTax.SubclassDlgItem(IDC_TRANS_EDIT_TAX, this);            m_edtTax.SetUnderlayColor(W);
    m_edtTip.SubclassDlgItem(IDC_TRANS_EDIT_TIP, this);            m_edtTip.SetUnderlayColor(W);
    m_edtTaxFree.SubclassDlgItem(IDC_TRANS_EDIT_TAXFREE, this);    m_edtTaxFree.SetUnderlayColor(W);
    m_edtQr.SubclassDlgItem(IDC_TRANS_EDIT_QR, this);              m_edtQr.SetUnderlayColor(W);
    m_edtOrgDate.SubclassDlgItem(IDC_TRANS_EDIT_ORG_DATE, this);   m_edtOrgDate.SetUnderlayColor(W);
    m_edtOrgAppNo.SubclassDlgItem(IDC_TRANS_EDIT_ORG_APPNO, this); m_edtOrgAppNo.SetUnderlayColor(W);
    m_edtCashNo.SubclassDlgItem(IDC_TRANS_EDIT_CASH_NO, this);     m_edtCashNo.SetUnderlayColor(W);
    // [수정 1] 할부개월 Edit 컨트롤 생성 (기존 ID 재사용하여 오류 방지)
    m_edtInstall.SubclassDlgItem(IDC_TRANS_CMB_INSTALLMENT, this); m_edtInstall.SetUnderlayColor(W);

    // [수정 2] ae 배열에 m_edtInstall 추가 및 개수를 8에서 9로 변경



    m_cmbCashType.SubclassDlgItem(IDC_TRANS_CMB_CASH_TYPE, this);
    m_cmbCashType.SetUnderlayColor(W);
    m_cmbCashType.AddString(_T("소득공제용"));
    m_cmbCashType.AddString(_T("지출증빙용"));
    m_cmbCashType.SetCurSel(0);

    struct FI { LPCTSTR cap; CWnd* ctrl; BOOL full; UINT ct; };
    FI fi[kNumFields] = {
        {_T("공급가액"),        &m_edtSupply,  FALSE,1},
        {_T("세금"),            &m_edtTax,     FALSE,1},
        {_T("봉사료"),          &m_edtTip,     FALSE,1},
        {_T("비과세"),          &m_edtTaxFree, FALSE,1},
        {_T("할부개월"),        &m_edtInstall, FALSE,1},
        {_T("QR/바코드"),       &m_edtQr,      FALSE,1},
        {_T("원거래 일자"),     &m_edtOrgDate, FALSE,1},
        {_T("원거래 승인번호"), &m_edtOrgAppNo,FALSE,1},
        {_T("현금영수증 종류"), &m_cmbCashType,FALSE,2},
        {_T("현금영수증 번호"), &m_edtCashNo,  FALSE,1},
    };
    m_fields.clear();
    for (int i = 0; i < kNumFields; i++) {
        FieldPair fp; fp.caption = fi[i].cap; fp.pCtrl = fi[i].ctrl;
        fp.bFullRow = fi[i].full; fp.ctrlType = fi[i].ct;
        m_fields.push_back(fp);
        m_fieldLabels[i].SubclassDlgItem(IDC_TRANS_LABEL_BASE + i, this);
        m_fieldLabels[i].SetWindowText(fi[i].cap);
    }
}

void CTransDlg::CreateResultControls()
{
    static LPCTSTR lbl[15] = {
        _T("거래 일시"),_T("응답코드"),_T("응답내역"),_T("승인번호"),_T("알림"),
        _T("단말기ID"),_T("카드번호"),_T("카드사명"),_T("매입사 코드"),_T("매입사명"),
        _T("발급사 코드"),_T("발급사명"),_T("카드구분"),_T("간편결제구분자"),_T("거래고유번호")
    };
    m_results.clear();
    for (int i = 0; i < kNumResults; i++) {
        ResultPair rp = { lbl[i],_T("-"),FALSE,FALSE };
        m_results.push_back(rp);
        m_resultLabels[i].SubclassDlgItem(IDC_TRANS_RESULT_LBL_BASE + i, this);
        m_resultValues[i].SubclassDlgItem(IDC_TRANS_VALUE_BASE + i, this);
        m_resultLabels[i].SetWindowText(lbl[i]);
        m_resultValues[i].SetWindowText(_T("-"));
    }
}

void CTransDlg::CreateBottomButton()
{
    m_btnClose.SubclassDlgItem(IDC_TRANS_BTN_CLOSE, this);
    m_btnClose.SetWindowText(_T("닫기"));
    m_btnClose.SetButtonStyle(ButtonStyle::Default);
    m_btnClose.SetUnderlayColor(kCardFill);
    m_btnRun.SubclassDlgItem(IDC_TRANS_BTN_RUN, this);
    m_btnRun.SetWindowText(GetModeButtonText());
    m_btnRun.SetButtonStyle(ButtonStyle::Primary);
    m_btnRun.SetUnderlayColor(kCardFill);
}

void CTransDlg::ApplyFonts()
{
    m_segCtrl.SetFont(&m_fontLabel);
    for (int i = 0; i < kNumFields; i++) {
        m_fieldLabels[i].SetFont(&m_fontLabel);
        if (m_fields[(size_t)i].pCtrl) {
            m_fields[(size_t)i].pCtrl->SetFont(&m_fontEdit);
        }
    }
    m_btnClose.SetFont(&m_fontEdit);
    m_btnRun.SetFont(&m_fontEdit);
    for (int i = 0; i < kNumResults; i++) {
        m_resultLabels[i].SetFont(&m_fontResultLabel);
        m_resultValues[i].SetFont(&m_fontResultValue);
    }
}

void CTransDlg::SetMode(ETransMode mode)
{
    if (mode == m_eMode) return;

    // 1. Save current tab values before switching
    for (int i = 0; i < kNumFields; i++) {
        CWnd* p = m_fields[(size_t)i].pCtrl;
        if (!p) continue;
        if (m_fields[(size_t)i].ctrlType == 2) {
            // combo: save selected index as string
            LRESULT sel = p->SendMessage(CB_GETCURSEL);
            m_tabValues[(int)m_eMode][i].Format(_T("%d"), (int)sel);
        }
        else {
            p->GetWindowText(m_tabValues[(int)m_eMode][i]);
        }
    }

    // 2. Switch mode
    m_eMode = mode;
    if (m_segCtrl.GetCurSel() != (int)mode) m_segCtrl.SetCurSelSilent((int)mode);
    m_btnRun.SetWindowText(GetModeButtonText());
    SetRedraw(FALSE);
    ShowFieldsForMode();

    // 3. Restore new tab values
    for (int i = 0; i < kNumFields; i++) {
        CWnd* p = m_fields[(size_t)i].pCtrl;
        if (!p) continue;
        if (m_fields[(size_t)i].ctrlType == 2) {
            int sel = _ttoi(m_tabValues[(int)mode][i]);
            p->SendMessage(CB_SETCURSEL, (WPARAM)(sel >= 0 ? sel : 0));
        }
        else {
            p->SetWindowText(m_tabValues[(int)mode][i]);
        }
    }

    LayoutControls();
    SetRedraw(TRUE);
    RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

CString CTransDlg::GetModeButtonText() const
{
    switch (m_eMode) {
    case MODE_CREDIT_APPROVAL: return _T("신용 승인 요청");
    case MODE_CREDIT_CANCEL:   return _T("신용 취소 요청");
    case MODE_CASH_APPROVAL:   return _T("현금 승인 요청");
    case MODE_CASH_CANCEL:     return _T("현금 취소 요청");
    } return _T("");
}

CString CTransDlg::GetCurrentModeName() const
{
    switch (m_eMode) {
    case MODE_CREDIT_APPROVAL: return _T("신용 승인");
    case MODE_CREDIT_CANCEL:   return _T("신용 취소");
    case MODE_CASH_APPROVAL:   return _T("현금 승인");
    case MODE_CASH_CANCEL:     return _T("현금 취소");
    } return _T("");
}

void CTransDlg::ShowFieldsForMode()
{
    BOOL show[kNumFields] = {};
    switch (m_eMode) {
    case MODE_CREDIT_APPROVAL:
        show[F_SUPPLY] = show[F_TAX] = show[F_TIP] = show[F_TAXFREE] = show[F_INSTALL] = show[F_QR] = TRUE; break;
    case MODE_CREDIT_CANCEL:
        show[F_SUPPLY] = show[F_ORGDATE] = show[F_ORGAPPNO] = show[F_INSTALL] = show[F_QR] = TRUE; break;
    case MODE_CASH_APPROVAL:
        show[F_SUPPLY] = show[F_TAX] = show[F_TIP] = show[F_TAXFREE] = show[F_CASHTYPE] = show[F_CASHNO] = TRUE; break;
    case MODE_CASH_CANCEL:
        show[F_SUPPLY] = show[F_CASHTYPE] = show[F_ORGDATE] = show[F_ORGAPPNO] = show[F_CASHNO] = TRUE; break;
    }
    for (int i = 0; i < kNumFields; i++) {
        SafeShow(&m_fieldLabels[i], show[i]);
        SafeShow(m_fields[(size_t)i].pCtrl, show[i]);
    }
    bool bCancel = (m_eMode == MODE_CREDIT_CANCEL || m_eMode == MODE_CASH_CANCEL);
    CString supLbl = bCancel ? _T("금액") : _T("공급가액");
    m_fieldLabels[F_SUPPLY].SetWindowText(supLbl);
    m_fields[F_SUPPLY].caption = supLbl;
}

void CTransDlg::ResetSampleResult()
{
    bool bOk = (m_eMode != MODE_CASH_APPROVAL);
    SetResultValue(0, _T("260415153005"));
    SetResultValue(1, bOk ? _T("000") : _T("101"));
    SetResultValue(2, _T("정상승인"));
    SetResultValue(3, bOk ? _T("30018492") : _T(""));
    SetResultValue(4, _T("거래가 성공적으로 완료되었습니다."));
    SetResultValue(5, _T("KFTC_T001"));
    SetResultValue(6, _T("9410-****-****-1234"));
    SetResultValue(7, _T("신한카드"));
    SetResultValue(8, _T("04"));
    SetResultValue(9, _T("신한카드"));
    SetResultValue(10, _T("04"));
    SetResultValue(11, _T("신한카드"));
    SetResultValue(12, _T("개인 / 신용"));
    SetResultValue(13, _T("삼성페이"));
    SetResultValue(14, _T("20260415143005KF00182749"));
    if (!bOk) {
        SetResultValue(2, _T("한도초과"));
        SetResultValue(4, _T("한도를 초과한 거래입니다."));
    }
    ApplyResultColoring();
    UpdateResultControls();
}

void CTransDlg::ApplyResultColoring()
{
    if ((int)m_results.size() < 4) return;
    // index 1: 응답코드, index 2: 응답내역, index 3: 승인번호
    bool bOk = (m_results[1].value == _T("000"));
    m_results[1].bBlue = bOk ? TRUE : FALSE;
    m_results[1].bRed = bOk ? FALSE : TRUE;
    m_results[2].bBlue = bOk ? TRUE : FALSE;
    m_results[2].bRed = bOk ? FALSE : TRUE;
    m_results[3].bBlue = FALSE; m_results[3].bRed = FALSE;
    m_strBadge = m_results[2].value;
    m_bBadgeOk = bOk ? TRUE : FALSE;
}

void CTransDlg::SetResultValue(int idx, LPCTSTR v, BOOL bBlue, BOOL bRed)
{
    if (idx < 0 || idx >= (int)m_results.size()) return;
    m_results[(size_t)idx].value = v;
    m_results[(size_t)idx].bBlue = bBlue;
    m_results[(size_t)idx].bRed = bRed;
}

void CTransDlg::ClearResult()
{
    for (auto& r : m_results) { r.value = _T(""); r.bBlue = FALSE; r.bRed = FALSE; }
    m_strBadge = _T("");
    m_bBadgeOk = FALSE;
    UpdateResultControls();
    UpdateWindow();
}

void CTransDlg::UpdateResultControls()
{
    for (int i = 0; i < (int)m_results.size(); i++) {
        m_resultLabels[i].SetWindowText(m_results[(size_t)i].pszLabel);
        m_resultValues[i].SetWindowText(m_results[(size_t)i].value);
        if (m_results[(size_t)i].bBlue) m_resultValues[i].SetFont(&m_fontResultBlue);
        else if (m_results[(size_t)i].bRed)  m_resultValues[i].SetFont(&m_fontResultRed);
        else                                  m_resultValues[i].SetFont(&m_fontResultValue);
    }
    Invalidate(FALSE);
}

BOOL CTransDlg::ValidateCurrentMode(CString& e)
{
    CString s; m_edtSupply.GetWindowText(s); s.Trim();
    if (s.IsEmpty()) { e = _T("금액을 입력하세요."); m_edtSupply.SetFocus(); return FALSE; }
    if (m_eMode == MODE_CREDIT_CANCEL || m_eMode == MODE_CASH_CANCEL) {
        m_edtOrgDate.GetWindowText(s); s.Trim();
        if (s.IsEmpty()) { e = _T("원거래 일자를 입력하세요."); m_edtOrgDate.SetFocus(); return FALSE; }
        m_edtOrgAppNo.GetWindowText(s); s.Trim();
        if (s.IsEmpty()) { e = _T("원거래 승인번호를 입력하세요."); m_edtOrgAppNo.SetFocus(); return FALSE; }
    }
    return TRUE;
}

void CTransDlg::DrawRoundedCard(Gdiplus::Graphics& g, const CRect& rc, int radius,
    COLORREF fill, COLORREF border, int shadowAlpha)
{
    if (shadowAlpha > 0) {
        for (int sh = 1; sh <= 3; sh++) {
            Gdiplus::GraphicsPath sp; CRect sr = rc; sr.OffsetRect(0, sh);
            AddRRP(sp, sr, (float)(radius + sh / 2));
            Gdiplus::SolidBrush sb(Gdiplus::Color((BYTE)(shadowAlpha - sh + 1), 0, 20, 60));
            g.FillPath(&sb, &sp);
        }
    }
    Gdiplus::GraphicsPath fp; AddRRP(fp, rc, (float)radius);
    Gdiplus::SolidBrush fb(Gdiplus::Color(255, GetRValue(fill), GetGValue(fill), GetBValue(fill)));
    Gdiplus::Pen        bp(Gdiplus::Color(255, GetRValue(border), GetGValue(border), GetBValue(border)), 1.f);
    g.FillPath(&fb, &fp); g.DrawPath(&bp, &fp);
}

void CTransDlg::LayoutControls()
{
    if (!::IsWindow(m_hWnd)) return;
    CRect rcForm, rcResult; GetContentRects(rcForm, rcResult);

    // [FIX 5] lH(라벨 높이), cH(입력칸 높이), gLC(라벨-입력칸 간격), fGX/fGY(상하좌우 간격) 대폭 확대
    const BOOL bCmpLayout = (::GetSystemMetrics(SM_CYSCREEN) <= 800);
    const int lH = SX(16), cH = SX(bCmpLayout ? 32 : 40), gLC = SX(6), fGX = SX(16), fGY = SX(16);
    const int bH = SX(52), bW1 = SX(80), bW2 = SX(160);
    const int segH = SX(CSegmentCtrl::kBarH + 6); // 탭 바 높이 증가

    // segment control near top of form card
    int segX = rcForm.left + SX(20), segW = rcForm.Width() - SX(40), segY = rcForm.top + SX(14);
    if (::IsWindow(m_segCtrl.GetSafeHwnd()))
        m_segCtrl.MoveWindow(segX, segY, segW, segH);

    // input fields (below amount display area)
// input fields (below amount display area)
    int amtAreaH = SX(52); // [FIX 2] 파란색 배경 박스 높이를 52로 확 줄여서 슬림하게 만듭니다.
    int fieldsTop = segY + segH + SX(12) + amtAreaH + SX(12);
    int fl = rcForm.left + SX(14), fw = rcForm.Width() - SX(28);
    int colW = (fw - fGX) / 2;

    static const int kOrder[4][kNumFields] = {
        { F_SUPPLY, F_TAX,     F_TIP,      F_TAXFREE,   F_INSTALL, F_QR,      -1, -1, -1, -1 },
        { F_SUPPLY, F_INSTALL, F_ORGDATE,  F_ORGAPPNO,  F_QR,      -1,        -1, -1, -1, -1 },
        { F_SUPPLY, F_TAX,     F_TIP,      F_TAXFREE,   F_CASHTYPE,F_CASHNO,  -1, -1, -1, -1 },
        { F_SUPPLY, F_CASHTYPE, F_ORGDATE,  F_ORGAPPNO,  F_CASHNO,  -1,        -1, -1, -1, -1 },
    };
    std::vector<int> vis;
    for (int i = 0; i < kNumFields; i++) {
        int fi = kOrder[(int)m_eMode][i];
        if (fi < 0) break;
        if (m_fields[(size_t)fi].pCtrl) {
            DWORD dwStyle = ::GetWindowLong(m_fields[(size_t)fi].pCtrl->GetSafeHwnd(), GWL_STYLE);
            if (dwStyle & WS_VISIBLE) vis.push_back(fi);
        }
    }
    auto mvField = [&](int fi, int x, int y, int w, int h) {
        FieldPair& fp = m_fields[(size_t)fi];
        if (fp.ctrlType == 2) {
            fp.pCtrl->MoveWindow(x, y, w, h + SX(120));
            fp.pCtrl->SendMessage(CB_SETITEMHEIGHT, (WPARAM)-1, h - SX(4));
        }
        else {
            fp.pCtrl->MoveWindow(x, y, w, h);
        }
        };
    int curY = fieldsTop;
    for (int c = 0; c < (int)vis.size(); c += 2) {
        int idx1 = vis[(size_t)c];
        int idx2 = (c + 1 < (int)vis.size()) ? vis[(size_t)(c + 1)] : -1;
        m_fieldLabels[idx1].MoveWindow(fl, curY, colW, lH);
        mvField(idx1, fl, curY + lH + gLC, colW, cH);
        if (idx2 >= 0) {
            m_fieldLabels[idx2].MoveWindow(fl + colW + fGX, curY, colW, lH);
            mvField(idx2, fl + colW + fGX, curY + lH + gLC, colW, cH);
        }
        curY += lH + gLC + cH + fGY;
    }

    // buttons pinned to bottom of form card
    int btnY = rcForm.bottom - SX(14) - bH;

    // 1. [닫기] 버튼을 오른쪽 끝에 먼저 배치
    int closeX = rcForm.right - SX(14) - bW1;
    m_btnClose.MoveWindow(closeX, btnY, bW1, bH);

    // 2. [요청] 버튼을 [닫기] 버튼의 왼쪽(closeX)에서 8px 간격을 두고 배치
    int runX = closeX - SX(8) - bW2;
    m_btnRun.MoveWindow(runX, btnY, bW2, bH);

    // result labels
    const int rRowH = SX(kResRowH);
    int rTY = rcResult.top + SX(44);
    int rL = rcResult.left + SX(14), rR2 = rcResult.right - SX(14);
    int rLW = SX(90), vW = rR2 - rL - rLW;
    for (int i = 0; i < kNumResults; i++) {
        int ry = rTY + rRowH * i;
        m_resultLabels[i].MoveWindow(rL, ry + SX(5), rLW, rRowH - SX(10));
        m_resultValues[i].MoveWindow(rL + rLW, ry + SX(5), vW, rRowH - SX(10));
    }
    Invalidate(FALSE);
}

BOOL CTransDlg::OnEraseBkgnd(CDC*) { return TRUE; }

void CTransDlg::OnPaint()
{
    CPaintDC dc(this); EnsureFonts();
    CRect cl; GetClientRect(&cl);
    CDC mem; mem.CreateCompatibleDC(&dc);
    CBitmap bmp; bmp.CreateCompatibleBitmap(&dc, cl.Width(), cl.Height());
    CBitmap* pOld = mem.SelectObject(&bmp);
    mem.FillSolidRect(cl, kDlgBg);

    CRect rcForm, rcResult; GetContentRects(rcForm, rcResult);

    // [FIX 3] OnPaint에서도 새 여백 공식 적용
    BOOL bCmp = (::GetSystemMetrics(SM_CYSCREEN) <= 800);
    int mL = SX(bCmp ? 12 : 20);
    int mT = SX(bCmp ? 6 : 10);
    int mR = SX(bCmp ? 12 : 20);
    int mB = SX(bCmp ? 12 : 20);
    CRect rcMain(mL, mT, cl.right - mR, cl.bottom - mB);
    CRect rcHdr(rcMain.left, rcMain.top, rcMain.right, rcMain.top + SX(84));

    // --- Phase 1: GDI+ shapes ---
    {
        Gdiplus::Graphics g(mem.GetSafeHdc());
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

        // 1. 하얀색 메인 카드 배경 (테두리 선 제거 + 은은한 그림자 3단계)
        Gdiplus::GraphicsPath path;
        AddRRP(path, rcMain, (float)SX(12));

        for (int sh = 3; sh >= 1; sh--) {
            Gdiplus::GraphicsPath shPath;
            CRect sr = rcMain; sr.OffsetRect(0, sh);
            AddRRP(shPath, sr, (float)SX(12));
            BYTE alpha = (BYTE)(sh == 3 ? 8 : sh == 2 ? 14 : 20);
            Gdiplus::SolidBrush shBrush(Gdiplus::Color(alpha, 10, 30, 70));
            g.FillPath(&shBrush, &shPath);
        }

        // 테두리선(DrawPath) 없이 하얀색 바탕(FillPath)만 칠합니다.
        Gdiplus::SolidBrush fillBrush(Gdiplus::Color(255, 255, 255, 255));
        g.FillPath(&fillBrush, &path);

        // [FIX 3] 반전된 느낌을 주던 불필요한 회색 바탕(bgRc) 그리기 영역 완전히 제거
    // 2. 옅은 회색(kCardFill) 내부 카드 영역 (테두리 선 그리기 없이 배경색만 칠함)
        Gdiplus::SolidBrush cardBrush(Gdiplus::Color(255, GetRValue(kCardFill), GetGValue(kCardFill), GetBValue(kCardFill)));
        Gdiplus::GraphicsPath pForm, pResult;
        AddRRP(pForm, rcForm, (float)SX(12));
        AddRRP(pResult, rcResult, (float)SX(12));
        // ???? ??? ?????
        for (int sh = 1; sh <= 2; sh++) {
            Gdiplus::GraphicsPath sp;
            Gdiplus::RectF sr((Gdiplus::REAL)rcForm.left, (Gdiplus::REAL)rcForm.top + sh * 1.5f,
                (Gdiplus::REAL)rcForm.Width(), (Gdiplus::REAL)rcForm.Height());
            ModernUIGfx::AddRoundRect(sp, sr, (float)SX(12));
            Gdiplus::SolidBrush sb(Gdiplus::Color(8, 0, 0, 0));
            g.FillPath(&sb, &sp);
        }
        g.FillPath(&cardBrush, &pForm);
        g.FillPath(&cardBrush, &pResult);

        // ----------------------------------------------------
        // [여기에 아래 두 줄을 추가해 보세요!]
        // 오른쪽(응답 정보) 카드에만 아주 연한 경계선(kCardBorder)을 그려줍니다.
        COLORREF cBorder = m_strBadge.IsEmpty() ? kCardBorder : (m_bBadgeOk ? RGB(34, 139, 34) : RGB(200, 50, 50));
        Gdiplus::Pen borderPen(Gdiplus::Color(255, GetRValue(cBorder), GetGValue(cBorder), GetBValue(cBorder)), m_strBadge.IsEmpty() ? 1.0f : 1.5f);
        g.DrawPath(&borderPen, &pResult);


        // result section title bar (blue pill)
        {
            Gdiplus::GraphicsPath bp;
            ModernUIGfx::AddRoundRect(bp,
                Gdiplus::RectF((Gdiplus::REAL)(rcResult.left + SX(14)),
                    (Gdiplus::REAL)(rcResult.top + SX(15)),
                    (Gdiplus::REAL)SX(4), (Gdiplus::REAL)SX(14)), SX(2));
            COLORREF cPill = m_strBadge.IsEmpty() ? kBlueText : (m_bBadgeOk ? RGB(34, 139, 34) : RGB(200, 50, 50));
            Gdiplus::SolidBrush bb(Gdiplus::Color(255, GetRValue(cPill), GetGValue(cPill), GetBValue(cPill)));
            g.FillPath(&bb, &bp);
        }
        // amount display bg
        {
            int segY2 = rcForm.top + SX(14), amtY = segY2 + SX(CSegmentCtrl::kBarH + 6) + SX(12);
            CRect rcAmt(rcForm.left + SX(20), amtY, rcForm.right - SX(20), amtY + SX(52));
            Gdiplus::GraphicsPath ap; AddRRP(ap, rcAmt, SX(8));

            // [수정] 테두리(선)를 절대 그리지 않으며, 색상을 ShopSetupDlg처럼 
            // 맑고 경계선이 안 보이는 플랫한 연한 파스텔 블루톤으로 변경합니다.
            Gdiplus::SolidBrush amtBg(Gdiplus::Color(255, 240, 246, 255));
            g.FillPath(&amtBg, &ap);
        }
        // result dividers
        {
            const int rRowH2 = SX(kResRowH);
            int rTY2 = rcResult.top + SX(44);
            int rL2 = rcResult.left + SX(14), rR22 = rcResult.right - SX(14);
            Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(kDivider), GetGValue(kDivider), GetBValue(kDivider)), 1.f);
            for (int row = 1; row < kNumResults; row++) {
                float ly = (float)(rTY2 + rRowH2 * row);
                g.DrawLine(&pen, (float)rL2, ly, (float)rR22, ly);
            }
        }
    } // GDI+ scope ends, flushed to mem

    // --- Phase 2: GDI text ---
    HDC hRaw = mem.GetSafeHdc();
    ::SetBkMode(hRaw, TRANSPARENT);

    // header
    {
        wchar_t wT[64] = {}, wS[256] = {};
        ::MultiByteToWideChar(CP_ACP, 0, _T("결제"), -1, wT, 64);
        ::MultiByteToWideChar(CP_ACP, 0, _T("신용 및 현금영수증 거래를 진행합니다"), -1, wS, 256);
        ModernUIHeader::Draw(hRaw,
            (float)(rcMain.left + SX(14)), (float)(rcMain.top + SX(16)), (float)SX(::GetSystemMetrics(SM_CYSCREEN) <= 800 ? 36 : 44),
            ModernUIHeader::IconType::Transaction, wT, wS,
            (HFONT)m_fontTitle.GetSafeHandle(), (HFONT)m_fontSub.GetSafeHandle(),
            rcMain.left + SX(6), rcMain.top + SX(::GetSystemMetrics(SM_CYSCREEN) <= 800 ? 60 : 74), rcMain.right - SX(6),
            ::GetSystemMetrics(SM_CYSCREEN) <= 800 ? 23.0f : 26.0f, ::GetSystemMetrics(SM_CYSCREEN) <= 800 ? 3.0f : 0.0f);
    }
    // result section title
    {
        HFONT hO = (HFONT)::SelectObject(hRaw, m_fontSection.GetSafeHandle());
        ::SetTextColor(hRaw, kSectionText);
        ::SetBkMode(hRaw, TRANSPARENT);
        ::TextOut(hRaw, rcResult.left + SX(26), rcResult.top + SX(14), _T("응답 정보"), lstrlen(_T("응답 정보")));
        ::SelectObject(hRaw, hO);
    }
    // badge
    if (!m_strBadge.IsEmpty()) {
        HFONT hOB = (HFONT)::SelectObject(hRaw, m_fontBadge.GetSafeHandle());
        SIZE sz = {}; ::GetTextExtentPoint32(hRaw, m_strBadge, m_strBadge.GetLength(), &sz);
        ::SelectObject(hRaw, hOB);
        int bw = sz.cx + SX(16), bh = SX(20);
        int bx = rcResult.right - SX(14) - bw, by2 = rcResult.top + SX(12);
        {
            Gdiplus::Graphics g2(hRaw);
            g2.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            COLORREF bFill = m_bBadgeOk ? RGB(230, 245, 235) : RGB(255, 235, 235);
            Gdiplus::GraphicsPath bPath; AddRRP(bPath, CRect(bx, by2, bx + bw, by2 + bh), SX(6));
            Gdiplus::SolidBrush bBr(Gdiplus::Color(255, GetRValue(bFill), GetGValue(bFill), GetBValue(bFill)));
            g2.FillPath(&bBr, &bPath);
        }
        COLORREF bText = m_bBadgeOk ? RGB(30, 130, 60) : kRedText;
        hOB = (HFONT)::SelectObject(hRaw, m_fontBadge.GetSafeHandle());
        ::SetTextColor(hRaw, bText); ::SetBkMode(hRaw, TRANSPARENT);
        RECT rcBT = { bx,by2,bx + bw,by2 + bh };
        ::DrawText(hRaw, m_strBadge, m_strBadge.GetLength(), &rcBT, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        ::SelectObject(hRaw, hOB);
    }
    // amount display text (좌/우 1줄 슬림 배치)
    {
        int segY2 = rcForm.top + SX(14), amtY = segY2 + SX(CSegmentCtrl::kBarH + 6) + SX(12);
        CRect rcAmt(rcForm.left + SX(20), amtY, rcForm.right - SX(20), amtY + SX(52)); // 52로 슬림하게 맞춤

        // 라벨: 진하고 큰 폰트(m_fontEdit) 적용, 좌측 세로 중앙 정렬
        HFONT hOL = (HFONT)::SelectObject(hRaw, m_fontLabel.GetSafeHandle());
        ::SetTextColor(hRaw, RGB(51, 61, 75)); // 까맣고 진한 텍스트
        ::SetBkMode(hRaw, TRANSPARENT);
        RECT rcLbl = { rcAmt.left + SX(16), rcAmt.top, rcAmt.right, rcAmt.bottom };
        ::DrawText(hRaw, _T("최종 결제 예정 금액"), -1, &rcLbl, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        ::SelectObject(hRaw, hOL);

        // 금액: 우측 세로 중앙 정렬
        bool bCancelMode = (m_eMode == MODE_CREDIT_CANCEL || m_eMode == MODE_CASH_CANCEL);
        auto parseN = [](CWnd& e)->long long {
            CString s; e.GetWindowText(s); s.Trim(); s.Remove(_T(','));
            return s.IsEmpty() ? 0LL : (long long)_ttoi64(s);
            };
        long long nTotal = parseN(m_edtSupply);
        if (!bCancelMode)
            nTotal += parseN(m_edtTax) + parseN(m_edtTip) + parseN(m_edtTaxFree);
        CString sAmt; sAmt = FormatAmountWithCommas(nTotal) + _T(" 원");

        HFONT hOA = (HFONT)::SelectObject(hRaw, m_fontAmount.GetSafeHandle());
        ::SetTextColor(hRaw, RGB(49, 130, 246)); // 토스 스타일 프라이머리 블루
        ::SetBkMode(hRaw, TRANSPARENT);
        RECT rcAV = { rcAmt.left, rcAmt.top, rcAmt.right - SX(16), rcAmt.bottom };
        ::DrawText(hRaw, sAmt, sAmt.GetLength(), &rcAV, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        ::SelectObject(hRaw, hOA);
    }

    dc.BitBlt(0, 0, cl.Width(), cl.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(pOld);
}

void CTransDlg::OnSize(UINT t, int cx, int cy)
{
    CDialog::OnSize(t, cx, cy);
    if (t == SIZE_MINIMIZED || cx == 0 || cy == 0) return;
    if (m_bUiBuilt) LayoutControls();
}

HBRUSH CTransDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    UINT id = pWnd ? pWnd->GetDlgCtrlID() : 0;

    // [FIX 4] 바뀐 회색 바탕(kCardFill)에 맞춘 전용 브러시 생성
    static CBrush s_brCardFill;
    if (s_brCardFill.GetSafeHandle() == NULL) {
        s_brCardFill.CreateSolidBrush(kCardFill);
    }

    if (nCtlColor == CTLCOLOR_STATIC) {
        // 입력 필드 라벨들
        for (int i = 0; i < kNumFields; i++) {
            if (id == IDC_TRANS_LABEL_BASE + (UINT)i) {
                pDC->SetTextColor(kLabelText);
                pDC->SetBkMode(OPAQUE);
                pDC->SetBkColor(kCardFill);
                return s_brCardFill;
            }
        }

        // 응답 결과 라벨 및 값들 (글자 겹침 방지를 위해 OPAQUE + 회색 브러시 처리)
        // ???? ??? ?? ?? ???? (ES_READONLY CEdit?? CTLCOLOR_STATIC?? ???)
        for (int i = 0; i < kNumResults; i++) {
            if (id == IDC_TRANS_RESULT_LBL_BASE + i) {
                pDC->SetTextColor(kLabelText);
                pDC->SetBkMode(OPAQUE);
                pDC->SetBkColor(kCardFill);
                return s_brCardFill;
            }
            if (id == IDC_TRANS_VALUE_BASE + i) {
                if (i < (int)m_results.size() && m_results[(size_t)i].bBlue) pDC->SetTextColor(kBlueText);
                else if (i < (int)m_results.size() && m_results[(size_t)i].bRed) pDC->SetTextColor(kRedText);
                else pDC->SetTextColor(kValueText);
                pDC->SetBkMode(OPAQUE);
                pDC->SetBkColor(kCardFill);
                return s_brCardFill;
            }
        }
        pDC->SetBkMode(TRANSPARENT);
    }
    return hbr;
}

void CTransDlg::OnTabSelChange(NMHDR*, LRESULT* pResult)
{
    int sel = m_segCtrl.GetCurSel();
    if (sel >= 0 && sel <= 3) SetMode((ETransMode)sel);
    *pResult = 0;
}

void CTransDlg::OnRunCreditApproval()
{
    ClearResult();
    CString vSupply, vTax, vTip, vTaxFree, vInstall, vQr;
    m_fields[F_SUPPLY].pCtrl->GetWindowText(vSupply);   vSupply.Trim();
    m_fields[F_TAX].pCtrl->GetWindowText(vTax);         vTax.Trim();
    m_fields[F_TIP].pCtrl->GetWindowText(vTip);         vTip.Trim();
    m_fields[F_TAXFREE].pCtrl->GetWindowText(vTaxFree); vTaxFree.Trim();
    m_fields[F_INSTALL].pCtrl->GetWindowText(vInstall); vInstall.Trim();
    m_fields[F_QR].pCtrl->GetWindowText(vQr);           vQr.Trim();
    int nSupply = ParseAmountText(vSupply);
    int nTax = ParseAmountText(vTax);
    int nTip = ParseAmountText(vTip);
    int nTaxFree = ParseAmountText(vTaxFree);

    CString msg, ln;
    msg = _T("[") + GetCurrentModeName() + _T("]\r\n\r\n");
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_SUPPLY].caption, (LPCTSTR)vSupply);  msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_TAX].caption, (LPCTSTR)vTax);     msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_TIP].caption, (LPCTSTR)vTip);     msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_TAXFREE].caption, (LPCTSTR)vTaxFree); msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_INSTALL].caption, (LPCTSTR)vInstall); msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_QR].caption, (LPCTSTR)vQr);      msg += ln;
    AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
    ResetSampleResult();
    if (m_results[1].value == _T("000")) {
        long long nTotal = (long long)ParseAmountText(vSupply)
            + (long long)ParseAmountText(vTax)
            + (long long)ParseAmountText(vTip)
            + (long long)ParseAmountText(vTaxFree);
        m_tabValues[(int)MODE_CREDIT_CANCEL][F_SUPPLY] = FormatAmountWithCommas(nTotal);
        m_tabValues[(int)MODE_CREDIT_CANCEL][F_INSTALL] = vInstall;
        CString dateStr = m_results[0].value;
        m_tabValues[(int)MODE_CREDIT_CANCEL][F_ORGDATE] = dateStr.Mid(2, 4);
        m_tabValues[(int)MODE_CREDIT_CANCEL][F_ORGAPPNO] = m_results[3].value;
    }
}
void CTransDlg::OnRunCreditCancel()
{
    ClearResult();
    CString vSupply, vOrgDate, vOrgAppNo, vInstall, vQr;
    m_fields[F_SUPPLY].pCtrl->GetWindowText(vSupply);     vSupply.Trim();

    m_fields[F_ORGDATE].pCtrl->GetWindowText(vOrgDate);   vOrgDate.Trim();
    m_fields[F_ORGAPPNO].pCtrl->GetWindowText(vOrgAppNo); vOrgAppNo.Trim();
    m_fields[F_INSTALL].pCtrl->GetWindowText(vInstall);   vInstall.Trim();
    m_fields[F_QR].pCtrl->GetWindowText(vQr);             vQr.Trim();
    int nSupply = ParseAmountText(vSupply);

    CString msg, ln;
    msg = _T("[") + GetCurrentModeName() + _T("]\r\n\r\n");
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_SUPPLY].caption, (LPCTSTR)vSupply);   msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_ORGDATE].caption, (LPCTSTR)vOrgDate);  msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_ORGAPPNO].caption, (LPCTSTR)vOrgAppNo); msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_INSTALL].caption, (LPCTSTR)vInstall);  msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_QR].caption, (LPCTSTR)vQr);       msg += ln;
    AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
    ResetSampleResult();
}
void CTransDlg::OnRunCashApproval()
{
    ClearResult();
    CString vSupply, vTax, vTip, vTaxFree, vCashType, vCashNo;
    m_fields[F_SUPPLY].pCtrl->GetWindowText(vSupply);     vSupply.Trim();
    m_fields[F_TAX].pCtrl->GetWindowText(vTax);           vTax.Trim();
    m_fields[F_TIP].pCtrl->GetWindowText(vTip);           vTip.Trim();
    m_fields[F_TAXFREE].pCtrl->GetWindowText(vTaxFree);   vTaxFree.Trim();
    m_fields[F_CASHTYPE].pCtrl->GetWindowText(vCashType); vCashType.Trim();
    m_fields[F_CASHNO].pCtrl->GetWindowText(vCashNo);     vCashNo.Trim();
    int nSupply = ParseAmountText(vSupply);
    int nTax = ParseAmountText(vTax);
    int nTip = ParseAmountText(vTip);
    int nTaxFree = ParseAmountText(vTaxFree);

    CString msg, ln;
    msg = _T("[") + GetCurrentModeName() + _T("]\r\n\r\n");
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_SUPPLY].caption, (LPCTSTR)vSupply);   msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_TAX].caption, (LPCTSTR)vTax);      msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_TIP].caption, (LPCTSTR)vTip);      msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_TAXFREE].caption, (LPCTSTR)vTaxFree);  msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_CASHTYPE].caption, (LPCTSTR)vCashType); msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_CASHNO].caption, (LPCTSTR)vCashNo);   msg += ln;
    AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
    ResetSampleResult();
    if (m_results[1].value == _T("000")) {
        long long nTotal = (long long)ParseAmountText(vSupply)
            + (long long)ParseAmountText(vTax)
            + (long long)ParseAmountText(vTip)
            + (long long)ParseAmountText(vTaxFree);
        m_tabValues[(int)MODE_CASH_CANCEL][F_SUPPLY] = FormatAmountWithCommas(nTotal);
        CString dateStr = m_results[0].value;
        m_tabValues[(int)MODE_CASH_CANCEL][F_ORGDATE] = dateStr.Mid(2, 4);
        m_tabValues[(int)MODE_CASH_CANCEL][F_ORGAPPNO] = m_results[3].value;
        { LRESULT s = m_fields[F_CASHTYPE].pCtrl->SendMessage(CB_GETCURSEL); m_tabValues[(int)MODE_CASH_CANCEL][F_CASHTYPE].Format(_T("%d"), (int)s); }
        m_tabValues[(int)MODE_CASH_CANCEL][F_CASHNO] = vCashNo;
    }
}
void CTransDlg::OnRunCashCancel()
{
    ClearResult();
    CString vSupply, vCashType, vOrgDate, vOrgAppNo, vCashNo;
    m_fields[F_SUPPLY].pCtrl->GetWindowText(vSupply);     vSupply.Trim();
    m_fields[F_CASHTYPE].pCtrl->GetWindowText(vCashType); vCashType.Trim();
    m_fields[F_ORGDATE].pCtrl->GetWindowText(vOrgDate);   vOrgDate.Trim();
    m_fields[F_ORGAPPNO].pCtrl->GetWindowText(vOrgAppNo); vOrgAppNo.Trim();
    m_fields[F_CASHNO].pCtrl->GetWindowText(vCashNo);     vCashNo.Trim();
    int nSupply = ParseAmountText(vSupply);

    CString msg, ln;
    msg = _T("[") + GetCurrentModeName() + _T("]\r\n\r\n");
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_SUPPLY].caption, (LPCTSTR)vSupply);   msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_ORGDATE].caption, (LPCTSTR)vOrgDate);  msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_ORGAPPNO].caption, (LPCTSTR)vOrgAppNo); msg += ln;
    ln.Format(_T("%s: %s\r\n"), (LPCTSTR)m_fields[F_CASHNO].caption, (LPCTSTR)vCashNo);   msg += ln;
    AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);
    ResetSampleResult();
}
BOOL CTransDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT nID = LOWORD(wParam);
    if (HIWORD(wParam) == BN_CLICKED) {
        if (nID == IDC_TRANS_BTN_CLOSE) { EndDialog(IDCANCEL); return TRUE; }
        if (nID == IDC_TRANS_BTN_RUN) {
            CString err;
            if (!ValidateCurrentMode(err)) { CModernMessageBox::Warning(err, this); return TRUE; }
            switch (m_eMode) {
            case MODE_CREDIT_APPROVAL: OnRunCreditApproval(); break;
            case MODE_CREDIT_CANCEL:   OnRunCreditCancel();   break;
            case MODE_CASH_APPROVAL:   OnRunCashApproval();   break;
            case MODE_CASH_CANCEL:     OnRunCashCancel();     break;
            }
            return TRUE;
        }
    }
    if (HIWORD(wParam) == EN_CHANGE) {
        UINT cID = LOWORD(wParam);
        static const UINT kAmtIDs[] = {
            IDC_TRANS_EDIT_SUPPLY, IDC_TRANS_EDIT_TAX,
            IDC_TRANS_EDIT_TIP,    IDC_TRANS_EDIT_TAXFREE };
        CSkinnedEdit* kAmtEdits[] = {
            &m_edtSupply, &m_edtTax, &m_edtTip, &m_edtTaxFree };
        for (int _i = 0; _i < 4; _i++) {
            if (cID != kAmtIDs[_i]) continue;
            static BOOL s_bFmt = FALSE;
            if (s_bFmt) break;
            s_bFmt = TRUE;
            CSkinnedEdit& edt = *kAmtEdits[_i];
            CString raw; edt.GetWindowText(raw);
            int nStart = 0, nEnd = 0; edt.GetSel(nStart, nEnd);
            int digitsBefore = 0;
            for (int j = 0; j < nStart && j < raw.GetLength(); ++j)
                if (_istdigit(raw[j])) digitsBefore++;
            int nAmt = ParseAmountText(raw);
            if (nAmt > 999999999) nAmt = 999999999;
            CString formatted = FormatAmountWithCommas(nAmt);
            if (formatted != raw) {
                edt.SetWindowText(formatted);
                int newPos = 0, dc = 0;
                for (int j = 0; j < formatted.GetLength() && dc < digitsBefore; ++j) {
                    if (_istdigit(formatted[j])) dc++;
                    newPos = j + 1;
                }
                edt.SetSel(newPos, newPos);
            }
            s_bFmt = FALSE;
            Invalidate(FALSE);
            break;
        }
    }
    return CDialog::OnCommand(wParam, lParam);
}
