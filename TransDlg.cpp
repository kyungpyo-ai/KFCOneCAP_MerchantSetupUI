#include "stdafx.h"
#include "resource.h"
#include "TransDlg.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ============================================================
// 색상 팔레트
// ============================================================
namespace {
    static const COLORREF kDlgBg         = RGB(238, 242, 249);
    static const COLORREF kMainCardBorder = RGB(218, 226, 240);
    static const COLORREF kCardFill       = RGB(255, 255, 255);
    static const COLORREF kCardBorder     = RGB(225, 232, 243);
    static const COLORREF kDivider        = RGB(241, 244, 248);
    static const COLORREF kLabelText      = RGB(140, 149, 163);
    static const COLORREF kValueText      = RGB(24,  31,  40);
    static const COLORREF kBlueText       = RGB(0,   100, 221);
    static const COLORREF kRedText        = RGB(200, 38,  38);
    static const COLORREF kSectionText    = RGB(0,   64,  160);

    static void AddRRP(Gdiplus::GraphicsPath& p, const CRect& rc, float r) {
        ModernUIGfx::AddRoundRect(p,
            Gdiplus::RectF((Gdiplus::REAL)rc.left, (Gdiplus::REAL)rc.top,
                           (Gdiplus::REAL)rc.Width(), (Gdiplus::REAL)rc.Height()), r);
    }
    static void SafeShow(CWnd* p, BOOL b) {
        if (p && ::IsWindow(p->GetSafeHwnd()))
            p->ShowWindow(b ? SW_SHOW : SW_HIDE);
    }
}

// ============================================================
// CSegmentCtrl 구현
// ============================================================
BEGIN_MESSAGE_MAP(CSegmentCtrl, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
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
    nm.idFrom   = (UINT_PTR)GetDlgCtrlID();
    nm.code     = TCN_SELCHANGE;
    CWnd* p = GetParent();
    if (p && p->GetSafeHwnd())
        p->SendMessage(WM_NOTIFY, (WPARAM)GetDlgCtrlID(), (LPARAM)&nm);
}
void CSegmentCtrl::OnLButtonDown(UINT, CPoint pt)
{
    if (m_tabs.empty()) return;
    CRect cl; GetClientRect(&cl);
    int n   = (int)m_tabs.size();
    int pad = Scale(3);
    float tw = (cl.Width() - 2*pad) / (float)n;
    for (int i = 0; i < n; i++) {
        int x1 = pad + (int)(i * tw);
        int x2 = pad + (int)((i+1) * tw);
        if (pt.x >= x1 && pt.x < x2) {
            if (i != m_nSel) SetCurSel(i);
            return;
        }
    }
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
        dc.BitBlt(0,0,cl.Width(),cl.Height(),&mem,0,0,SRCCOPY);
        mem.SelectObject(pOld); return;
    }

    int n   = (int)m_tabs.size();
    int pad = Scale(3);
    int rO  = Scale(11);  // 외부 pill radius
    int rI  = Scale(9);   // 내부 active 탭 radius

    // 회색 pill 배경 (HTML: background:#EEEFF1, border-radius:12px, padding:4px)
    {
        Gdiplus::GraphicsPath bgP;
        ModernUIGfx::AddRoundRect(bgP,
            Gdiplus::RectF(0,0,(Gdiplus::REAL)cl.Width(),(Gdiplus::REAL)cl.Height()),
            (Gdiplus::REAL)rO);
        Gdiplus::SolidBrush bgBr(Gdiplus::Color(255, 238, 239, 241));
        g.FillPath(&bgBr, &bgP);
    }

    float tw = (cl.Width() - 2*pad) / (float)n;
    float th = (float)(cl.Height() - 2*pad);

    // 선택된 탭 흰색 pill (그림자 포함)
    {
        float tx = pad + m_nSel * tw;
        float ty = (float)pad;
        for (int sh = 3; sh >= 1; sh--) {
            Gdiplus::GraphicsPath sp;
            ModernUIGfx::AddRoundRect(sp,
                Gdiplus::RectF(tx+0.5f, ty+(float)sh, tw-1.0f, th),
                (Gdiplus::REAL)rI);
            Gdiplus::SolidBrush sb(Gdiplus::Color((BYTE)(sh * 2 + 1), 0, 20, 80));
            g.FillPath(&sb, &sp);
        }
        Gdiplus::GraphicsPath fp;
        ModernUIGfx::AddRoundRect(fp,
            Gdiplus::RectF(tx, ty, tw, th), (Gdiplus::REAL)rI);
        Gdiplus::SolidBrush fb(Gdiplus::Color(255, 255, 255, 255));
        g.FillPath(&fb, &fp);
    }

    // 각 탭 텍스트
    UINT dpi = ModernUIDpi::GetDpiForHwnd(m_hWnd);
    LOGFONT lf = {};
    lf.lfCharSet = HANGUL_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    ModernUIFont::ApplyUIFontFace(lf);

    for (int i = 0; i < n; i++) {
        bool bActive = (i == m_nSel);
        lf.lfHeight = -MulDiv(10, dpi, 72);
        lf.lfWeight = bActive ? FW_BOLD : FW_NORMAL;
        CFont font; font.CreateFontIndirect(&lf);

        float tx = pad + i * tw;
        float ty = (float)pad;

        // 유니코드 변환 (CP949 → UTF-16)
        int wlen = ::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)m_tabs[(size_t)i], -1, NULL, 0);
        std::vector<wchar_t> wbuf((size_t)wlen, 0);
        ::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)m_tabs[(size_t)i], -1, wbuf.data(), wlen);

        HDC hdc = g.GetHDC();
        HFONT hOld = (HFONT)::SelectObject(hdc, (HFONT)font.GetSafeHandle());
        SIZE sz = {};
        ::GetTextExtentPoint32W(hdc, wbuf.data(), wlen > 0 ? wlen-1 : 0, &sz);

        int cx = (int)(tx + tw/2 - sz.cx/2);
        int cy2 = (int)(ty + th/2 - sz.cy/2);
        RECT rcT = { cx, cy2, cx + sz.cx + 2, cy2 + sz.cy };
        ::SetTextColor(hdc, bActive ? kBlueText : RGB(140, 150, 165));
        ::SetBkMode(hdc, TRANSPARENT);
        ::DrawTextW(hdc, wbuf.data(), wlen > 0 ? wlen-1 : 0, &rcT,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        ::SelectObject(hdc, hOld);
        g.ReleaseHDC(hdc);
    }

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
    , m_eMode(MODE_CREDIT_APPROVAL), m_bUiBuilt(FALSE), m_brBack(kDlgBg)
{}
CTransDlg::~CTransDlg() {}
void CTransDlg::DoDataExchange(CDataExchange* pDX) { CDialog::DoDataExchange(pDX); }
void CTransDlg::OnOK()     {}
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
#define MKF(px,wt,f) lf.lfHeight=-ModernUIDpi::Scale(m_hWnd,px); lf.lfWeight=wt; f.CreateFontIndirect(&lf)
    MKF(20, FW_BOLD,      m_fontTitle);
    MKF(11, FW_NORMAL,    m_fontSub);
    MKF(13, FW_BOLD,      m_fontSection);
    MKF(14, FW_NORMAL,    m_fontLabel);
    MKF(14, FW_BOLD,      m_fontEdit);
    MKF(18, FW_EXTRABOLD, m_fontAmount);
    MKF(11, FW_NORMAL,    m_fontResultLabel);
    MKF(13, FW_BOLD,      m_fontResultValue);
    MKF(14, FW_EXTRABOLD, m_fontResultBlue);
    MKF(13, FW_BOLD,      m_fontResultRed);
#undef MKF
}

// ------ 레이아웃 헬퍼 ------
int CTransDlg::GetVisibleFieldRowCount() const
{
    int rows = 0, c = 0;
    while (c < (int)m_fields.size()) {
        const FieldPair& f = m_fields[(size_t)c];
        if (!f.pCtrl || !::IsWindowVisible(f.pCtrl->GetSafeHwnd())) { ++c; continue; }
        ++rows;
        if (!f.bFullRow && c+1 < (int)m_fields.size()) {
            const FieldPair& f2 = m_fields[(size_t)(c+1)];
            if (f2.pCtrl && ::IsWindowVisible(f2.pCtrl->GetSafeHwnd()) && !f2.bFullRow) ++c;
        }
        ++c;
    }
    return rows > 0 ? rows : 1;
}
int CTransDlg::GetSetupCardHeight() const
{
    // 섹션타이틀 44 + 탭(46+12) + 행들 + 아래여백 16
    const int sectH = SX(44), tabH = SX(CSegmentCtrl::kBarH) + SX(12);
    const int rH = SX(50), rG = SX(10), bot = SX(16);
    int rows = GetVisibleFieldRowCount();
    return sectH + tabH + rows * rH + (rows-1) * rG + bot;
}
int CTransDlg::GetResultCardHeight() const
{
    // 2열 구성: 5행 (열 2개 × 행 5개)
    return SX(44) + 5 * SX(34) + SX(14);
}
void CTransDlg::ResizeForCurrentMode()
{
    if (!::IsWindow(m_hWnd)) return;
    int sH = GetSetupCardHeight();
    int rH = GetResultCardHeight();
    // clientH = om(10)+hdr(84)+gap(16)+sH+gap(16)+rH+footer(60)+om(10)
    int clientH = SX(10) + SX(84) + SX(16) + sH + SX(16) + rH + SX(60) + SX(10);
    RECT rcW, rcC;
    ::GetWindowRect(m_hWnd, &rcW);
    ::GetClientRect(m_hWnd, &rcC);
    int ncH = (rcW.bottom - rcW.top) - (rcC.bottom - rcC.top);
    int ncW = (rcW.right  - rcW.left) - (rcC.right  - rcC.left);
    SetWindowPos(NULL, 0, 0, SX(560) + ncW, clientH + ncH,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL CTransDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    // WS_CLIPCHILDREN 제거: 부모 BitBlt가 숨겨진 컨트롤 영역도 덮도록 함
    // (이게 없으면 SW_HIDE된 CStatic 영역에 ghost text가 남음)
    ModifyStyle(WS_CLIPCHILDREN, 0, 0);
    ModernUIGfx::EnsureGdiplusStartup();
    EnsureFonts();
    SetWindowText(_T("통합 결제 테스트"));
    CreateSegmentControl();
    CreateInputControls();
    CreateResultControls();
    CreateBottomButtons();
    ApplyFonts();
    m_eMode = MODE_CREDIT_APPROVAL;
    m_segCtrl.SetCurSelSilent(0);
    ShowFieldsForMode();
    m_bUiBuilt = TRUE;
    ResizeForCurrentMode();
    LayoutControls();
    ResetSampleResult();
    CenterWindow();
    return TRUE;
}

void CTransDlg::CreateSegmentControl()
{
    CRect rc(0, 0, 100, CSegmentCtrl::kBarH);
    m_segCtrl.Create(this, IDC_TRANS_SEG, rc);
    m_segCtrl.AddTab(_T("신용승인"));
    m_segCtrl.AddTab(_T("신용취소"));
    m_segCtrl.AddTab(_T("현금승인"));
    m_segCtrl.AddTab(_T("현금취소"));
}

void CTransDlg::CreateInputControls()
{
    DWORD dwE = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL;
    DWORD dwC = CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS |
                WS_VSCROLL | WS_CHILD | WS_TABSTOP;
    const COLORREF W = RGB(255,255,255);

    m_edtAmount.Create(dwE,    CRect(0,0,0,0), this, IDC_TRANS_EDIT_AMOUNT);
    m_edtTax.Create(dwE,       CRect(0,0,0,0), this, IDC_TRANS_EDIT_TAX);
    m_edtTip.Create(dwE,       CRect(0,0,0,0), this, IDC_TRANS_EDIT_TIP);
    m_edtTaxFree.Create(dwE,   CRect(0,0,0,0), this, IDC_TRANS_EDIT_TAXFREE);
    m_edtOrgDate.Create(dwE,   CRect(0,0,0,0), this, IDC_TRANS_EDIT_ORG_DATE);
    m_edtOrgApproval.Create(dwE, CRect(0,0,0,0), this, IDC_TRANS_EDIT_ORG_APPROVAL);
    m_edtCashNo.Create(dwE,    CRect(0,0,0,0), this, IDC_TRANS_EDIT_CASH_NO);

    CSkinnedEdit* ae[] = { &m_edtAmount, &m_edtTax, &m_edtTip, &m_edtTaxFree,
                            &m_edtOrgDate, &m_edtOrgApproval, &m_edtCashNo };
    for (int i = 0; i < 7; i++) ae[i]->SetUnderlayColor(W);

    auto mkCmb = [&](CSkinnedComboBox& c, UINT id) {
        HWND h = ::CreateWindowEx(0, _T("COMBOBOX"), _T(""), dwC,
            0,0,100,200, m_hWnd,(HMENU)(UINT_PTR)id, AfxGetInstanceHandle(), NULL);
        c.SubclassWindow(h); c.SetUnderlayColor(W);
    };
    mkCmb(m_cmbInstallment,  IDC_TRANS_COMBO_INSTALLMENT);
    mkCmb(m_cmbCashType,     IDC_TRANS_COMBO_CASH_TYPE);
    mkCmb(m_cmbCancelReason, IDC_TRANS_COMBO_CANCEL_REASON);

    m_cmbInstallment.AddString(_T("일시불"));
    m_cmbInstallment.AddString(_T("02개월"));
    m_cmbInstallment.AddString(_T("03개월"));
    m_cmbInstallment.AddString(_T("06개월"));
    m_cmbInstallment.AddString(_T("12개월"));
    m_cmbInstallment.SetCurSel(0);
    m_cmbCashType.AddString(_T("소득공제용"));
    m_cmbCashType.AddString(_T("지출증빙용"));
    m_cmbCashType.SetCurSel(0);
    m_cmbCancelReason.AddString(_T("구매자 요청"));
    m_cmbCancelReason.AddString(_T("오류 취소"));
    m_cmbCancelReason.AddString(_T("기타"));
    m_cmbCancelReason.SetCurSel(0);

    struct FI { LPCTSTR cap; CWnd* ctrl; BOOL full; UINT ct; };
    FI fi[10] = {
        { _T("판매금액"),             &m_edtAmount,       TRUE,  1 },
        { _T("세금"),                 &m_edtTax,          FALSE, 1 },
        { _T("봉사료"),               &m_edtTip,          FALSE, 1 },
        { _T("비과세"),               &m_edtTaxFree,      FALSE, 1 },
        { _T("할부개월"),             &m_cmbInstallment,  FALSE, 2 },
        { _T("원거래 일자"),          &m_edtOrgDate,      FALSE, 1 },
        { _T("원거래 승인번호"),      &m_edtOrgApproval,  TRUE,  1 },
        { _T("현금영수증 번호"),      &m_edtCashNo,       TRUE,  1 },
        { _T("현금영수증 종류"),      &m_cmbCashType,     FALSE, 2 },
        { _T("현금영수증 취소 사유"), &m_cmbCancelReason, TRUE,  2 },
    };
    m_fields.clear();
    for (int i = 0; i < 10; i++) {
        FieldPair fp; fp.caption=fi[i].cap; fp.pCtrl=fi[i].ctrl;
        fp.bFullRow=fi[i].full; fp.ctrlType=fi[i].ct;
        m_fields.push_back(fp);
        m_fieldLabels[i].Create(fi[i].cap, WS_CHILD|WS_VISIBLE,
            CRect(0,0,0,0), this, IDC_TRANS_LABEL_BASE+(UINT)i);
    }
}

void CTransDlg::CreateResultControls()
{
    static LPCTSTR lbl[10] = {
        _T("거래일시"), _T("단말기ID"), _T("응답코드"), _T("카드번호"), _T("응답내역"),
        _T("카드사명"), _T("승인번호"), _T("매입사명"), _T("카드구분"), _T("알림")
    };
    m_results.clear();
    for (int i = 0; i < 10; i++) {
        ResultPair rp = { lbl[i], _T("-"), FALSE, FALSE };
        m_results.push_back(rp);
        m_resultLabels[i].Create(lbl[i], WS_CHILD|WS_VISIBLE,
            CRect(0,0,0,0), this, IDC_TRANS_RESULT_LBL_BASE+i);
        m_resultValues[i].Create(_T("-"), WS_CHILD|WS_VISIBLE|SS_RIGHT,
            CRect(0,0,0,0), this, IDC_TRANS_VALUE_BASE+i);
    }
}

void CTransDlg::CreateBottomButtons()
{
    DWORD s = WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW;
    m_btnClose.Create(_T("닫기"), s, CRect(0,0,0,0), this, IDC_TRANS_BTN_CLOSE);
    m_btnClose.SetButtonStyle(ButtonStyle::Default);
    m_btnRun.Create(_T("테스트 실행"), s, CRect(0,0,0,0), this, IDC_TRANS_BTN_RUN);
    m_btnRun.SetButtonStyle(ButtonStyle::Primary);
}

void CTransDlg::ApplyFonts()
{
    m_segCtrl.SetFont(&m_fontLabel);
    for (size_t i = 0; i < m_fields.size(); i++) {
        m_fieldLabels[i].SetFont(&m_fontLabel);
        if (m_fields[i].pCtrl)
            m_fields[i].pCtrl->SetFont(
                m_fields[i].pCtrl == &m_edtAmount ? &m_fontAmount : &m_fontEdit);
    }
    m_btnClose.SetFont(&m_fontEdit);
    m_btnRun.SetFont(&m_fontEdit);
    for (int i = 0; i < 10; i++) {
        m_resultLabels[i].SetFont(&m_fontResultLabel);
        m_resultValues[i].SetFont(&m_fontResultValue);
    }
}

void CTransDlg::SetMode(ETransMode mode)
{
    m_eMode = mode;
    // 재귀 방지: SetCurSelSilent로 선택 변경 (알림 없음)
    if (m_segCtrl.GetCurSel() != (int)mode)
        m_segCtrl.SetCurSelSilent((int)mode);
    ShowFieldsForMode();
    ResizeForCurrentMode();
    LayoutControls();
    // WS_CLIPCHILDREN 우회: 숨겨진 컨트롤 영역까지 강제 즉시 리페인트
    RedrawWindow(NULL, NULL,
        RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

void CTransDlg::ShowFieldsForMode()
{
    enum { F_AMT=0,F_TAX,F_TIP,F_TAXFREE,F_INSTALL,F_ORGDATE,F_ORGAPP,F_CASHNO,F_CASHTYPE,F_REASON };
    BOOL show[10] = {};
    switch (m_eMode) {
    case MODE_CREDIT_APPROVAL:
        show[F_AMT]=show[F_TAX]=show[F_TIP]=show[F_TAXFREE]=show[F_INSTALL]=TRUE; break;
    case MODE_CREDIT_CANCEL:
        show[F_AMT]=show[F_INSTALL]=show[F_ORGDATE]=show[F_ORGAPP]=TRUE; break;
    case MODE_CASH_APPROVAL:
        show[F_AMT]=show[F_TAX]=show[F_TIP]=show[F_TAXFREE]=show[F_CASHNO]=show[F_CASHTYPE]=TRUE; break;
    case MODE_CASH_CANCEL:
        show[F_AMT]=show[F_ORGDATE]=show[F_ORGAPP]=show[F_CASHNO]=show[F_REASON]=TRUE; break;
    }
    // 숨겨질 컨트롤의 영역을 부모에 무효화 (WS_CLIPCHILDREN 잔상 방지)
    for (int i = 0; i < 10; i++) {
        auto invHide = [&](CWnd* p) {
            if (!show[i] && p && ::IsWindow(p->GetSafeHwnd())
                && (p->GetStyle() & WS_VISIBLE)) {
                CRect r; p->GetWindowRect(&r); ScreenToClient(&r);
                InvalidateRect(&r, TRUE);
            }
        };
        invHide(&m_fieldLabels[i]);
        invHide(m_fields[(size_t)i].pCtrl);
    }
    for (int i = 0; i < 10; i++) {
        SafeShow(&m_fieldLabels[i], show[i]);
        SafeShow(m_fields[(size_t)i].pCtrl, show[i]);
    }
}

void CTransDlg::ResetSampleResult()
{
    SetResultValue(0, _T("2026-03-20 21:12:06"));
    SetResultValue(1, _T("12345678"));
    SetResultValue(2, _T("0000 (정상)"), FALSE, TRUE);
    SetResultValue(3, _T("5243-34**-****-****"));
    SetResultValue(4, _T("정상승인 되었습니다."));
    SetResultValue(5, _T("삼성카드"));
    SetResultValue(6, _T("30012345"), TRUE, FALSE);
    SetResultValue(7, _T("비씨카드"));
    SetResultValue(8, _T("개인신용"));
    SetResultValue(9, _T("-"));
    UpdateResultControls();
}
void CTransDlg::SetResultValue(int idx, LPCTSTR v, BOOL bBlue, BOOL bRed)
{
    if (idx < 0 || idx >= (int)m_results.size()) return;
    m_results[(size_t)idx].value = v;
    m_results[(size_t)idx].bBlue = bBlue;
    m_results[(size_t)idx].bRed  = bRed;
}
void CTransDlg::UpdateResultControls()
{
    for (int i = 0; i < (int)m_results.size(); i++) {
        m_resultLabels[i].SetWindowText(m_results[(size_t)i].pszLabel);
        m_resultValues[i].SetWindowText(m_results[(size_t)i].value);
        if      (m_results[(size_t)i].bBlue) m_resultValues[i].SetFont(&m_fontResultBlue);
        else if (m_results[(size_t)i].bRed)  m_resultValues[i].SetFont(&m_fontResultRed);
        else                                  m_resultValues[i].SetFont(&m_fontResultValue);
    }
    Invalidate(TRUE);
}
CString CTransDlg::GetCurrentModeName() const
{
    switch (m_eMode) {
    case MODE_CREDIT_APPROVAL: return _T("신용승인");
    case MODE_CREDIT_CANCEL:   return _T("신용취소");
    case MODE_CASH_APPROVAL:   return _T("현금승인");
    case MODE_CASH_CANCEL:     return _T("현금취소");
    }
    return _T("");
}
CString CTransDlg::BuildSampleMessage() const
{
    switch (m_eMode) {
    case MODE_CREDIT_APPROVAL: return _T("정상승인 되었습니다.");
    case MODE_CREDIT_CANCEL:   return _T("정상취소 되었습니다.");
    case MODE_CASH_APPROVAL:   return _T("현금영수증 발급 완료.");
    case MODE_CASH_CANCEL:     return _T("현금영수증 취소 완료.");
    }
    return _T("");
}
BOOL CTransDlg::ValidateCurrentMode(CString& e)
{
    CString s;
    m_edtAmount.GetWindowText(s); s.Trim();
    if (s.IsEmpty()) { e=_T("판매금액을 입력하세요."); m_edtAmount.SetFocus(); return FALSE; }
    if (m_eMode==MODE_CREDIT_CANCEL || m_eMode==MODE_CASH_CANCEL) {
        m_edtOrgDate.GetWindowText(s); s.Trim();
        if (s.IsEmpty()) { e=_T("원거래 일자를 입력하세요."); m_edtOrgDate.SetFocus(); return FALSE; }
        m_edtOrgApproval.GetWindowText(s); s.Trim();
        if (s.IsEmpty()) { e=_T("원거래 승인번호를 입력하세요."); m_edtOrgApproval.SetFocus(); return FALSE; }
    }
    if (m_eMode==MODE_CASH_APPROVAL || m_eMode==MODE_CASH_CANCEL) {
        m_edtCashNo.GetWindowText(s); s.Trim();
        if (s.IsEmpty()) { e=_T("현금영수증 번호를 입력하세요."); m_edtCashNo.SetFocus(); return FALSE; }
    }
    return TRUE;
}

void CTransDlg::DrawRoundedCard(Gdiplus::Graphics& g, const CRect& rc, int radius,
                                 COLORREF fill, COLORREF border, int shadowAlpha)
{
    if (shadowAlpha > 0) {
        for (int sh = 1; sh <= 3; sh++) {
            Gdiplus::GraphicsPath sp; CRect sr=rc; sr.OffsetRect(0,sh);
            AddRRP(sp, sr, (float)(radius+sh/2));
            Gdiplus::SolidBrush sb(Gdiplus::Color((BYTE)(shadowAlpha-sh+1), 0, 20, 60));
            g.FillPath(&sb, &sp);
        }
    }
    Gdiplus::GraphicsPath fp; AddRRP(fp, rc, (float)radius);
    Gdiplus::SolidBrush fb(Gdiplus::Color(255,GetRValue(fill),GetGValue(fill),GetBValue(fill)));
    Gdiplus::Pen        bp(Gdiplus::Color(255,GetRValue(border),GetGValue(border),GetBValue(border)),1.f);
    g.FillPath(&fb, &fp); g.DrawPath(&bp, &fp);
}

void CTransDlg::DrawSectionTitle(CDC& dc, const CRect& rc, LPCTSTR text)
{
    dc.SetBkMode(TRANSPARENT);
    Gdiplus::Graphics g(dc.GetSafeHdc());
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    Gdiplus::GraphicsPath bp;
    ModernUIGfx::AddRoundRect(bp,
        Gdiplus::RectF((Gdiplus::REAL)rc.left,
                       (Gdiplus::REAL)(rc.top + (rc.Height()-SX(14))/2),
                       (Gdiplus::REAL)SX(4), (Gdiplus::REAL)SX(14)),
        (Gdiplus::REAL)SX(2));
    Gdiplus::SolidBrush bb(Gdiplus::Color(255,GetRValue(kBlueText),GetGValue(kBlueText),GetBValue(kBlueText)));
    g.FillPath(&bb, &bp);
    CFont* pOld = dc.SelectObject(&m_fontSection);
    dc.SetTextColor(kSectionText);
    dc.TextOut(rc.left+SX(12), rc.top+(rc.Height()-SX(14))/2, text);
    dc.SelectObject(pOld);
}

void CTransDlg::LayoutControls()
{
    if (!::IsWindow(m_hWnd)) return;
    CRect cl; GetClientRect(&cl);

    const int om=SX(10), cp=SX(18), ig=SX(14);
    const int lH=SX(14), cH=SX(32), gLC=SX(4);
    const int fGX=SX(12), fGY=SX(10);
    const int bH=SX(42), bW1=SX(90), bW2=SX(140);
    const int rRowH=SX(34);

    CRect rcMain(om, om, cl.right-om, cl.bottom-om);
    CRect rcHdr(rcMain.left, rcMain.top, rcMain.right, rcMain.top+SX(84));
    int cL=rcMain.left+cp, cR=rcMain.right-cp;

    int sH  = GetSetupCardHeight();
    int rCH = GetResultCardHeight();
    CRect rcS(cL, rcHdr.bottom+SX(16), cR, rcHdr.bottom+SX(16)+sH);
    CRect rcR(cL, rcS.bottom+ig, cR, rcS.bottom+ig+rCH);
    CRect rcFtr(cL, rcMain.bottom-SX(54), cR, rcMain.bottom-SX(10));

    // 세그먼트 컨트롤
    int segY = rcS.top + SX(14) + SX(20) + SX(10);
    int segH = SX(CSegmentCtrl::kBarH);
    if (::IsWindow(m_segCtrl.GetSafeHwnd()))
        m_segCtrl.MoveWindow(rcS.left+SX(16), segY, rcS.Width()-SX(32), segH);

    // 입력 필드
    int fTop = segY + segH + SX(12);
    int fl=rcS.left+SX(16), fw=rcS.Width()-SX(32);
    int colW=(fw-fGX)/2, curY=fTop;

    std::vector<int> vis;
    for (int i=0; i<(int)m_fields.size(); i++)
        if (m_fields[(size_t)i].pCtrl &&
            ::IsWindowVisible(m_fields[(size_t)i].pCtrl->GetSafeHwnd()))
            vis.push_back(i);

    for (int c=0; c<(int)vis.size(); ) {
        int idx=vis[(size_t)c]; FieldPair& f=m_fields[(size_t)idx];
        if (f.bFullRow) {
            m_fieldLabels[(size_t)idx].MoveWindow(fl, curY, fw, lH);
            f.pCtrl->MoveWindow(fl, curY+lH+gLC, fw, cH);
            curY += lH+gLC+cH+fGY; ++c;
        } else {
            int idx2=-1;
            if (c+1<(int)vis.size() && !m_fields[(size_t)vis[(size_t)(c+1)]].bFullRow)
                idx2=vis[(size_t)(++c)];
            m_fieldLabels[(size_t)idx].MoveWindow(fl, curY, colW, lH);
            f.pCtrl->MoveWindow(fl, curY+lH+gLC, colW, cH);
            if (idx2>=0) {
                FieldPair& f2=m_fields[(size_t)idx2];
                m_fieldLabels[(size_t)idx2].MoveWindow(fl+colW+fGX, curY, colW, lH);
                f2.pCtrl->MoveWindow(fl+colW+fGX, curY+lH+gLC, colW, cH);
            }
            curY += lH+gLC+cH+fGY; ++c;
        }
    }

    // 결과 행 (2열 × 5행)
    int rTY = rcR.top + SX(44);
    int rL=rcR.left+SX(16), rR2=rcR.right-SX(16);
    int rTW = rR2 - rL;                 // 전체 가용 너비
    int rColGap = SX(16);               // 열 간격
    int rColW = (rTW - rColGap) / 2;   // 열 너비
    int rLW = SX(60);                   // 레이블 너비
    for (int i=0; i<10; i++) {
        int col = i / 5;                // 0=좌열(0~4), 1=우열(5~9)
        int row = i % 5;
        int cx = rL + col * (rColW + rColGap);
        int ry = rTY + rRowH * row;
        int vW = rColW - rLW;
        m_resultLabels[i].MoveWindow(cx, ry+(rRowH-SX(13))/2, rLW, SX(13));
        m_resultValues[i].MoveWindow(cx+rLW, ry+(rRowH-SX(14))/2, vW, SX(14));
    }

    // 버튼
    m_btnRun.MoveWindow(rcFtr.right-bW2, rcFtr.top, bW2, bH);
    m_btnClose.MoveWindow(rcFtr.right-bW2-SX(8)-bW1, rcFtr.top, bW1, bH);
    Invalidate(TRUE);
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

    Gdiplus::Graphics g(mem.GetSafeHdc());
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    const int om=SX(10), cp=SX(18), ig=SX(14);
    CRect rcMain(om, om, cl.right-om, cl.bottom-om);
    CRect rcHdr(rcMain.left, rcMain.top, rcMain.right, rcMain.top+SX(84));
    int cL=rcMain.left+cp, cR=rcMain.right-cp;
    int sH=GetSetupCardHeight(), rCH=GetResultCardHeight();
    CRect rcS(cL, rcHdr.bottom+SX(16), cR, rcHdr.bottom+SX(16)+sH);
    CRect rcR(cL, rcS.bottom+ig, cR, rcS.bottom+ig+rCH);

    // 메인 카드
    DrawRoundedCard(g, rcMain, SX(20), RGB(255,255,255), kMainCardBorder, 12);
    // 거래 설정 카드 (회색 내부 배경 영역)
    {
        Gdiplus::SolidBrush gbg(Gdiplus::Color(255, 248, 249, 250));
        Gdiplus::GraphicsPath gp; AddRRP(gp, CRect(rcMain.left+SX(10), rcHdr.bottom+SX(8),
            rcMain.right-SX(10), rcR.bottom+SX(8)), SX(14));
        g.FillPath(&gbg, &gp);
    }
    DrawRoundedCard(g, rcS, SX(14), kCardFill, kCardBorder, 0);
    DrawRoundedCard(g, rcR, SX(14), kCardFill, kCardBorder, 0);

    // 헤더
    {
        wchar_t wT[64]={}, wS[256]={};
        ::MultiByteToWideChar(CP_ACP,0,_T("통합 결제 테스트"),-1,wT,64);
        ::MultiByteToWideChar(CP_ACP,0,
            _T("신용 및 현금영수증 거래 기능을 실시간으로 테스트합니다."),-1,wS,256);
        ModernUIHeader::Draw(mem.GetSafeHdc(),
            (float)(rcMain.left+SX(20)), (float)(rcMain.top+SX(16)), (float)SX(46),
            ModernUIHeader::IconType::CardTerminal, wT, wS,
            (HFONT)m_fontTitle.GetSafeHandle(), (HFONT)m_fontSub.GetSafeHandle(),
            rcMain.left+SX(12), rcMain.top+SX(78), rcMain.right-SX(12));
    }

    // 섹션 타이틀
    DrawSectionTitle(mem,
        CRect(rcS.left+SX(16), rcS.top+SX(14), rcS.left+SX(220), rcS.top+SX(34)),
        _T("거래 설정"));
    DrawSectionTitle(mem,
        CRect(rcR.left+SX(16), rcR.top+SX(14), rcR.left+SX(220), rcR.top+SX(34)),
        _T("거래 결과 정보"));

    // 결과 구분선 (2열 × 5행)
    {
        const int rRowH=SX(34);
        int rTY=rcR.top+SX(44);
        int rL=rcR.left+SX(16), rR2=rcR.right-SX(16);
        int rTW=rR2-rL, rColGap=SX(16), rColW=(rTW-rColGap)/2;
        Gdiplus::Pen pen(Gdiplus::Color(255,
            GetRValue(kDivider),GetGValue(kDivider),GetBValue(kDivider)), 1.f);
        // 각 열 내부 수평 구분선 (행 사이)
        for (int col=0; col<2; col++) {
            int cx = rL + col * (rColW + rColGap);
            for (int row=1; row<5; row++) {
                float ly = (float)(rTY + rRowH * row);
                g.DrawLine(&pen,(float)cx, ly,(float)(cx+rColW), ly);
            }
        }
        // 열 사이 수직 구분선
        float vx = (float)(rL + rColW + rColGap/2);
        g.DrawLine(&pen, vx,(float)rTY, vx,(float)(rTY+5*rRowH));
    }

    dc.BitBlt(0,0,cl.Width(),cl.Height(),&mem,0,0,SRCCOPY);
    mem.SelectObject(pOld);
}

void CTransDlg::OnSize(UINT t, int cx, int cy)
{
    CDialog::OnSize(t, cx, cy);
    if (m_bUiBuilt) LayoutControls();
}

HBRUSH CTransDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    UINT id = pWnd ? pWnd->GetDlgCtrlID() : 0;
    if (nCtlColor == CTLCOLOR_STATIC) {
        // 필드 레이블: 흰색 불투명 배경 (탭 전환 시 ghost text 방지)
        // TRANSPARENT 모드면 이전 텍스트 위에 새 텍스트를 그려 글씨가 겹침
        for (size_t i=0; i<m_fields.size(); i++)
            if (id == IDC_TRANS_LABEL_BASE+(UINT)i) {
                pDC->SetTextColor(RGB(90, 100, 115));
                pDC->SetBkColor(RGB(255, 255, 255));
                return (HBRUSH)GetStockObject(WHITE_BRUSH);
            }
        // 결과 레이블/값: 투명 배경 (위치 고정이라 ghost text 없음)
        pDC->SetBkMode(TRANSPARENT);
        for (int i=0; i<10; i++) {
            if (id == IDC_TRANS_RESULT_LBL_BASE+i)
                { pDC->SetTextColor(kLabelText); return (HBRUSH)GetStockObject(NULL_BRUSH); }
            if (id == IDC_TRANS_VALUE_BASE+i) {
                if      (i<(int)m_results.size()&&m_results[(size_t)i].bBlue) pDC->SetTextColor(kBlueText);
                else if (i<(int)m_results.size()&&m_results[(size_t)i].bRed)  pDC->SetTextColor(kRedText);
                else    pDC->SetTextColor(kValueText);
                return (HBRUSH)GetStockObject(NULL_BRUSH);
            }
        }
    }
    return hbr;
}

void CTransDlg::OnTabSelChange(NMHDR*, LRESULT* pResult)
{
    int sel = m_segCtrl.GetCurSel();
    if (sel >= 0 && sel <= 3) SetMode((ETransMode)sel);
    *pResult = 0;
}

BOOL CTransDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UINT nID = LOWORD(wParam);
    if (HIWORD(wParam) == BN_CLICKED) {
        if (nID == IDC_TRANS_BTN_CLOSE) { EndDialog(IDCANCEL); return TRUE; }
        if (nID == IDC_TRANS_BTN_RUN) {
            CString err;
            if (!ValidateCurrentMode(err)) { AfxMessageBox(err,MB_ICONEXCLAMATION|MB_OK); return TRUE; }
            CString amt; m_edtAmount.GetWindowText(amt); amt.Trim();
            bool bCash = (m_eMode==MODE_CASH_APPROVAL || m_eMode==MODE_CASH_CANCEL);
            SetResultValue(0, _T("2026-03-20 21:12:06"));
            SetResultValue(1, _T("12345678"));
            SetResultValue(2, _T("0000 (정상)"), FALSE, TRUE);
            SetResultValue(3, bCash ? _T("010-12**-****") : _T("5243-34**-****-****"));
            SetResultValue(4, BuildSampleMessage());
            SetResultValue(5, bCash ? _T("현금영수증") : _T("삼성카드"));
            SetResultValue(6, _T("30012345"), TRUE);
            SetResultValue(7, bCash ? _T("현금영수증") : _T("비씨카드"));
            SetResultValue(8, bCash ? _T("현금") : _T("개인신용"));
            { CString n; n.Format(_T("[%s] %s"),(LPCTSTR)GetCurrentModeName(),(LPCTSTR)amt);
              SetResultValue(9, n); }
            UpdateResultControls(); return TRUE;
        }
    }
    return CDialog::OnCommand(wParam, lParam);
}
