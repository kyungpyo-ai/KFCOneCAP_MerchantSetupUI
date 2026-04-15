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
    nm.idFrom   = (UINT_PTR)GetDlgCtrlID();
    nm.code     = TCN_SELCHANGE;
    CWnd* p = GetParent();
    if (p && p->GetSafeHwnd())
        p->SendMessage(WM_NOTIFY, (WPARAM)GetDlgCtrlID(), (LPARAM)&nm);
}
int CSegmentCtrl::HitTab(CPoint pt) const
{
    if (m_tabs.empty()) return -1;
    CRect cl; ::GetClientRect(m_hWnd, &cl);
    int n   = (int)m_tabs.size();
    int pad = Scale(3);
    float tw = (cl.Width() - 2*pad) / (float)n;
    for (int i = 0; i < n; i++) {
        int x1 = pad + (int)(i * tw);
        int x2 = pad + (int)((i+1) * tw);
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
    tme.cbSize    = sizeof(tme);
    tme.dwFlags   = TME_LEAVE;
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
        dc.BitBlt(0,0,cl.Width(),cl.Height(),&mem,0,0,SRCCOPY);
        mem.SelectObject(pOld); return;
    }

    int n   = (int)m_tabs.size();
    int pad = Scale(3);
    // [FIX 3] 탭 모서리를 조금 더 둥글게 처리
    int rO = Scale(12);  // 외부 pill radius
    int rI = Scale(10);   // 내부 active 탭 radius

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

    // Hover / Press overlay for non-selected tabs
    for (int i = 0; i < n; i++) {
        if (i == m_nSel) continue;
        BYTE alpha = 0;
        if (i == m_nPress)      alpha = 45;
        else if (i == m_nHover) alpha = 22;
        if (alpha == 0) continue;
        Gdiplus::GraphicsPath hp;
        ModernUIGfx::AddRoundRect(hp,
            Gdiplus::RectF(pad + i*tw, (float)pad, tw, th),
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
        COLORREF crText = bActive       ? kBlueText :
                          (i == m_nPress) ? RGB(55,  68,  90)  :
                          (i == m_nHover) ? RGB(90, 102, 122)  :
                                            RGB(140, 150, 165);
        ::SetTextColor(hdc, crText);
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
    , m_eMode(MODE_CREDIT_APPROVAL), m_bUiBuilt(FALSE)
    , m_brBack(kDlgBg), m_bBadgeOk(FALSE)
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
    MKF(18, FW_BOLD,      m_fontTitle);
    MKF(13, FW_BOLD,      m_fontSub);
    MKF(13, FW_BOLD,      m_fontSection);
    MKF(14, FW_BOLD,      m_fontLabel);
    MKF(14, FW_NORMAL,    m_fontEdit);
    // [FIX 1] 금액 폰트 크기를 다시 적당한 크기(24)로 줄입니다.
    MKF(24, FW_EXTRABOLD, m_fontAmount);
    MKF(11, FW_NORMAL, m_fontResultLabel);
    MKF(12, FW_BOLD,      m_fontResultValue);
    MKF(13, FW_EXTRABOLD, m_fontResultBlue);
    MKF(12, FW_BOLD,      m_fontResultRed);
    MKF(10, FW_BOLD,      m_fontBadge);
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
    const int hdrH = ModernUIDpi::Scale(m_hWnd, 84), ig = ModernUIDpi::Scale(m_hWnd, 12);

    // 새 여백을 적용하여 메인 카드 영역 도출
    CRect rcMain(mL, mT, cl.right - mR, cl.bottom - mB);
    int cL=rcMain.left+cp, cR=rcMain.right-cp;
    int cTop=rcMain.top+hdrH+ig, cBot=rcMain.bottom-ig;
    int halfW=(cR-cL-ig)/2;
    rcForm   = CRect(cL, cTop, cL+halfW, cBot);
    rcResult = CRect(cL+halfW+ig, cTop, cR, cBot);
}

void CTransDlg::ResizeWindow()
{
    if (!::IsWindow(m_hWnd)) return;
    int kRCH = SX(44) + kNumResults * SX(kResRowH) + SX(14);

    // [FIX 2] 여백이 커진 만큼 클라이언트 높이와 윈도우 폭을 확장
    BOOL bCmp = (::GetSystemMetrics(SM_CYSCREEN) <= 800);
    int mT = SX(bCmp ? 6 : 10);
    int mB = SX(bCmp ? 12 : 20);

    int clientH = mT + SX(84) + SX(12) + kRCH + SX(12) + mB;
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
    m_bUiBuilt = TRUE;
    ResizeWindow();
    LayoutControls();
    ResetSampleResult();
    CenterWindow();
    ModernUIWindow::ApplyWhiteTitleBar(this->GetSafeHwnd());
    return TRUE;
}

void CTransDlg::CreateSegmentControl()
{
    CRect rc(0,0,100,CSegmentCtrl::kBarH);
    m_segCtrl.Create(this, IDC_TRANS_SEG, rc);
    m_segCtrl.AddTab(_T("신용승인"));
    m_segCtrl.AddTab(_T("신용취소"));
    m_segCtrl.AddTab(_T("현금 승인"));
    m_segCtrl.AddTab(_T("현금 취소"));
}

void CTransDlg::CreateInputControls()
{
    DWORD dwE = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL;
    // [FIX 2] 입력 컨트롤 언더레이를 하얀색에서 내부 카드색(kCardFill)으로 통일
    const COLORREF W = kCardFill;
    m_edtSupply.Create(dwE, CRect(0, 0, 0, 0), this, IDC_TRANS_EDIT_SUPPLY);
    m_edtTax.Create(dwE,      CRect(0,0,0,0),this,IDC_TRANS_EDIT_TAX);
    m_edtTip.Create(dwE,      CRect(0,0,0,0),this,IDC_TRANS_EDIT_TIP);
    m_edtTaxFree.Create(dwE,  CRect(0,0,0,0),this,IDC_TRANS_EDIT_TAXFREE);
    m_edtQr.Create(dwE,       CRect(0,0,0,0),this,IDC_TRANS_EDIT_QR);
    m_edtOrgDate.Create(dwE,  CRect(0,0,0,0),this,IDC_TRANS_EDIT_ORG_DATE);
    m_edtOrgAppNo.Create(dwE, CRect(0,0,0,0),this,IDC_TRANS_EDIT_ORG_APPNO);
    m_edtCashNo.Create(dwE, CRect(0, 0, 0, 0), this, IDC_TRANS_EDIT_CASH_NO);
    // [수정 1] 할부개월 Edit 컨트롤 생성 (기존 ID 재사용하여 오류 방지)
    m_edtInstall.Create(dwE, CRect(0, 0, 0, 0), this, IDC_TRANS_CMB_INSTALLMENT);

    // [수정 2] ae 배열에 m_edtInstall 추가 및 개수를 8에서 9로 변경
    CSkinnedEdit* ae[] = { &m_edtSupply,&m_edtTax,&m_edtTip,&m_edtTaxFree,
                        &m_edtQr,&m_edtOrgDate,&m_edtOrgAppNo,&m_edtCashNo, &m_edtInstall };
    for (int i = 0; i < 9; i++) ae[i]->SetUnderlayColor(W);

    DWORD dwC = CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_CHILD | WS_TABSTOP;
    auto mkCmb = [&](CSkinnedComboBox& c, UINT id) {
        HWND h = ::CreateWindowEx(0, _T("COMBOBOX"), _T(""), dwC, -2000, -2000, 100, 200,
            m_hWnd, (HMENU)(UINT_PTR)id, AfxGetInstanceHandle(), NULL);
        c.SubclassWindow(h); c.SetUnderlayColor(W);
        };

    // 할부개월 콤보박스 관련 코드 삭제됨. 현금영수증 종류 콤보박스만 남김.
    mkCmb(m_cmbCashType, IDC_TRANS_CMB_CASH_TYPE);
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
    for (int i=0; i<kNumFields; i++) {
        FieldPair fp; fp.caption=fi[i].cap; fp.pCtrl=fi[i].ctrl;
        fp.bFullRow=fi[i].full; fp.ctrlType=fi[i].ct;
        m_fields.push_back(fp);
        m_fieldLabels[i].Create(fi[i].cap,WS_CHILD|WS_VISIBLE,
            CRect(0,0,0,0),this,IDC_TRANS_LABEL_BASE+(UINT)i);
    }
}

void CTransDlg::CreateResultControls()
{
    static LPCTSTR lbl[15]={
        _T("거래 일시"),_T("응답코드"),_T("응답내역"),_T("승인번호"),_T("알림"),
        _T("단말기ID"),_T("카드번호"),_T("카드사명"),_T("매입사 코드"),_T("매입사명"),
        _T("발급사명"),_T("발급사 코드"),_T("카드구분"),_T("간편결제구분자"),_T("거래고유번호")
    };
    m_results.clear();
    for (int i=0; i<kNumResults; i++) {
        ResultPair rp={lbl[i],_T("-"),FALSE,FALSE};
        m_results.push_back(rp);
        m_resultLabels[i].Create(lbl[i],WS_CHILD|WS_VISIBLE,
            CRect(0,0,0,0),this,IDC_TRANS_RESULT_LBL_BASE+i);
        m_resultValues[i].Create(_T("-"),WS_CHILD|WS_VISIBLE|SS_RIGHT,
            CRect(0,0,0,0),this,IDC_TRANS_VALUE_BASE+i);
    }
}

void CTransDlg::CreateBottomButton()
{
    DWORD s=WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW;
    m_btnClose.Create(_T("닫기"),s,CRect(0,0,0,0),this,IDC_TRANS_BTN_CLOSE);
    m_btnClose.SetButtonStyle(ButtonStyle::Default);
    m_btnClose.SetUnderlayColor(kCardFill);
    m_btnRun.Create(_T("신용 승인 요청"),s,CRect(0,0,0,0),this,IDC_TRANS_BTN_RUN);
    m_btnRun.SetButtonStyle(ButtonStyle::Primary);
    m_btnRun.SetUnderlayColor(kCardFill);
}

void CTransDlg::ApplyFonts()
{
    m_segCtrl.SetFont(&m_fontLabel);
    for (int i=0; i<kNumFields; i++) {
        m_fieldLabels[i].SetFont(&m_fontLabel);
        if (m_fields[(size_t)i].pCtrl) {
            m_fields[(size_t)i].pCtrl->SetFont(&m_fontEdit);
        }
    }
    m_btnClose.SetFont(&m_fontEdit);
    m_btnRun.SetFont(&m_fontEdit);
    for (int i=0; i<kNumResults; i++) {
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
        } else {
            p->GetWindowText(m_tabValues[(int)m_eMode][i]);
        }
    }

    // 2. Switch mode
    m_eMode = mode;
    if (m_segCtrl.GetCurSel() != (int)mode) m_segCtrl.SetCurSelSilent((int)mode);
    m_btnRun.SetWindowText(GetModeButtonText());
    ShowFieldsForMode();

    // 3. Restore new tab values
    for (int i = 0; i < kNumFields; i++) {
        CWnd* p = m_fields[(size_t)i].pCtrl;
        if (!p) continue;
        if (m_fields[(size_t)i].ctrlType == 2) {
            int sel = _ttoi(m_tabValues[(int)mode][i]);
            p->SendMessage(CB_SETCURSEL, (WPARAM)(sel >= 0 ? sel : 0));
        } else {
            p->SetWindowText(m_tabValues[(int)mode][i]);
        }
    }

    LayoutControls();
    RedrawWindow(NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
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
    enum{F_SUPPLY=0,F_TAX,F_TIP,F_TAXFREE,F_INSTALL,F_QR,F_ORGDATE,F_ORGAPPNO,F_CASHTYPE,F_CASHNO};
    BOOL show[kNumFields]={};
    switch (m_eMode) {
    case MODE_CREDIT_APPROVAL:
        show[F_SUPPLY]=show[F_TAX]=show[F_TIP]=show[F_TAXFREE]=show[F_INSTALL]=show[F_QR]=TRUE; break;
    case MODE_CREDIT_CANCEL:
        show[F_SUPPLY]=show[F_ORGDATE]=show[F_ORGAPPNO]=show[F_INSTALL]=show[F_QR]=TRUE; break;
    case MODE_CASH_APPROVAL:
        show[F_SUPPLY]=show[F_TAX]=show[F_TIP]=show[F_TAXFREE]=show[F_CASHTYPE]=show[F_CASHNO]=TRUE; break;
    case MODE_CASH_CANCEL:
        show[F_SUPPLY]=show[F_ORGDATE]=show[F_ORGAPPNO]=TRUE; break;
    }
    for (int i=0; i<kNumFields; i++) {
        SafeShow(&m_fieldLabels[i], show[i]);
        SafeShow(m_fields[(size_t)i].pCtrl, show[i]);
    }
    bool bCancel=(m_eMode==MODE_CREDIT_CANCEL||m_eMode==MODE_CASH_CANCEL);
    CString supLbl=bCancel?_T("금액"):_T("공급가액");
    m_fieldLabels[F_SUPPLY].SetWindowText(supLbl);
    m_fields[F_SUPPLY].caption=supLbl;
}

void CTransDlg::ResetSampleResult()
{
    SetResultValue(0, _T("2026-04-15 14:30:05"));
    SetResultValue(1, _T("0000"));
    SetResultValue(2, _T("정상승인"),FALSE,TRUE);
    SetResultValue(3, _T("30018492"),TRUE);
    SetResultValue(4, _T("거래가 성공적으로 완료되었습니다."));
    SetResultValue(5, _T("KFTC_T001"));
    SetResultValue(6, _T("9410-****-****-1234"));
    SetResultValue(7, _T("신한카드"));
    SetResultValue(8, _T("04"));
    SetResultValue(9, _T("신한카드"));
    SetResultValue(10,_T("신한카드"));
    SetResultValue(11,_T("04"));
    SetResultValue(12,_T("개인 / 신용"));
    SetResultValue(13,_T("삼성페이"));
    SetResultValue(14,_T("20260415143005KF00182749"));
    m_strBadge=_T("정상 승인"); m_bBadgeOk=TRUE;
    UpdateResultControls();
}

void CTransDlg::SetResultValue(int idx,LPCTSTR v,BOOL bBlue,BOOL bRed)
{
    if (idx<0||idx>=(int)m_results.size()) return;
    m_results[(size_t)idx].value=v;
    m_results[(size_t)idx].bBlue=bBlue;
    m_results[(size_t)idx].bRed=bRed;
}

void CTransDlg::UpdateResultControls()
{
    for (int i=0; i<(int)m_results.size(); i++) {
        m_resultLabels[i].SetWindowText(m_results[(size_t)i].pszLabel);
        m_resultValues[i].SetWindowText(m_results[(size_t)i].value);
        if      (m_results[(size_t)i].bBlue) m_resultValues[i].SetFont(&m_fontResultBlue);
        else if (m_results[(size_t)i].bRed)  m_resultValues[i].SetFont(&m_fontResultRed);
        else                                  m_resultValues[i].SetFont(&m_fontResultValue);
    }
    Invalidate(TRUE);
}

BOOL CTransDlg::ValidateCurrentMode(CString& e)
{
    CString s; m_edtSupply.GetWindowText(s); s.Trim();
    if (s.IsEmpty()) { e=_T("금액을 입력하세요."); m_edtSupply.SetFocus(); return FALSE; }
    if (m_eMode==MODE_CREDIT_CANCEL||m_eMode==MODE_CASH_CANCEL) {
        m_edtOrgDate.GetWindowText(s); s.Trim();
        if (s.IsEmpty()) { e=_T("원거래 일자를 입력하세요."); m_edtOrgDate.SetFocus(); return FALSE; }
        m_edtOrgAppNo.GetWindowText(s); s.Trim();
        if (s.IsEmpty()) { e=_T("원거래 승인번호를 입력하세요."); m_edtOrgAppNo.SetFocus(); return FALSE; }
    }
    if (m_eMode==MODE_CASH_APPROVAL) {
        m_edtCashNo.GetWindowText(s); s.Trim();
        if (s.IsEmpty()) { e=_T("현금영수증 번호를 입력하세요."); m_edtCashNo.SetFocus(); return FALSE; }
    }
    return TRUE;
}

void CTransDlg::DrawRoundedCard(Gdiplus::Graphics& g,const CRect& rc,int radius,
                                  COLORREF fill,COLORREF border,int shadowAlpha)
{
    if (shadowAlpha>0) {
        for (int sh=1; sh<=3; sh++) {
            Gdiplus::GraphicsPath sp; CRect sr=rc; sr.OffsetRect(0,sh);
            AddRRP(sp,sr,(float)(radius+sh/2));
            Gdiplus::SolidBrush sb(Gdiplus::Color((BYTE)(shadowAlpha-sh+1),0,20,60));
            g.FillPath(&sb,&sp);
        }
    }
    Gdiplus::GraphicsPath fp; AddRRP(fp,rc,(float)radius);
    Gdiplus::SolidBrush fb(Gdiplus::Color(255,GetRValue(fill),GetGValue(fill),GetBValue(fill)));
    Gdiplus::Pen        bp(Gdiplus::Color(255,GetRValue(border),GetGValue(border),GetBValue(border)),1.f);
    g.FillPath(&fb,&fp); g.DrawPath(&bp,&fp);
}

void CTransDlg::LayoutControls()
{
    if (!::IsWindow(m_hWnd)) return;
    CRect rcForm, rcResult; GetContentRects(rcForm, rcResult);

    // [FIX 5] lH(라벨 높이), cH(입력칸 높이), gLC(라벨-입력칸 간격), fGX/fGY(상하좌우 간격) 대폭 확대
    const int lH = SX(16), cH = SX(40), gLC = SX(6), fGX = SX(16), fGY = SX(16);
    const int bH = SX(52), bW1 = SX(80), bW2 = SX(160);
    const int segH = SX(CSegmentCtrl::kBarH + 6); // 탭 바 높이 증가

    // segment control near top of form card
    int segX = rcForm.left + SX(20), segW = rcForm.Width() - SX(40), segY = rcForm.top + SX(14);
    if (::IsWindow(m_segCtrl.GetSafeHwnd()))
        m_segCtrl.MoveWindow(segX,segY,segW,segH);

    // input fields (below amount display area)
// input fields (below amount display area)
    int amtAreaH = SX(52); // [FIX 2] 파란색 배경 박스 높이를 52로 확 줄여서 슬림하게 만듭니다.
    int fieldsTop = segY + segH + SX(16) + amtAreaH + SX(16);
    int fl=rcForm.left+SX(14), fw=rcForm.Width()-SX(28);
    int colW = (fw - fGX) / 2;

    std::vector<int> vis;
    for (int i = 0; i < kNumFields; i++) {
        if (m_fields[(size_t)i].pCtrl) {
            // [FIX] ::IsWindowVisible 대신 컨트롤 자체의 속성(WS_VISIBLE)만 직접 검사
            // (OnInitDialog 실행 시점에는 부모 다이얼로그가 아직 화면에 뜨지 않아 FALSE가 나오는 현상 방지)
            DWORD dwStyle = ::GetWindowLong(m_fields[(size_t)i].pCtrl->GetSafeHwnd(), GWL_STYLE);
            if (dwStyle & WS_VISIBLE) {
                vis.push_back(i);
            }
        }
    }

    // mvField: combo(ctrlType==2) needs larger total height + CB_SETITEMHEIGHT for selection area
    auto mvField = [&](int fi, int x, int y, int w, int h) {
        FieldPair& fp = m_fields[(size_t)fi];
        if (fp.ctrlType == 2) {
            fp.pCtrl->MoveWindow(x, y, w, h + SX(120));
            fp.pCtrl->SendMessage(CB_SETITEMHEIGHT, (WPARAM)-1, h - SX(4));
        } else {
            fp.pCtrl->MoveWindow(x, y, w, h);
        }
    };
    int curY = fieldsTop;
    for (int c=0; c<(int)vis.size(); c+=2) {
        int idx1=vis[(size_t)c];
        int idx2=(c+1<(int)vis.size())?vis[(size_t)(c+1)]:-1;
        m_fieldLabels[idx1].MoveWindow(fl,curY,colW,lH);
        mvField(idx1, fl, curY+lH+gLC, colW, cH);
        if (idx2>=0) {
            m_fieldLabels[idx2].MoveWindow(fl+colW+fGX,curY,colW,lH);
            mvField(idx2, fl+colW+fGX, curY+lH+gLC, colW, cH);
        }
        curY+=lH+gLC+cH+fGY;
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
    const int rRowH=SX(kResRowH);
    int rTY=rcResult.top+SX(44);
    int rL=rcResult.left+SX(14), rR2=rcResult.right-SX(14);
    int rLW=SX(90), vW=rR2-rL-rLW;
    for (int i=0; i<kNumResults; i++) {
        int ry=rTY+rRowH*i;
        m_resultLabels[i].MoveWindow(rL, ry + SX(5), rLW, rRowH - SX(10));
        m_resultValues[i].MoveWindow(rL+rLW, ry + SX(5), vW, rRowH - SX(10));
    }
    Invalidate(TRUE);
}

BOOL CTransDlg::OnEraseBkgnd(CDC*) { return TRUE; }

void CTransDlg::OnPaint()
{
    CPaintDC dc(this); EnsureFonts();
    CRect cl; GetClientRect(&cl);
    CDC mem; mem.CreateCompatibleDC(&dc);
    CBitmap bmp; bmp.CreateCompatibleBitmap(&dc,cl.Width(),cl.Height());
    CBitmap* pOld=mem.SelectObject(&bmp);
    mem.FillSolidRect(cl,kDlgBg);

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
        g.FillPath(&cardBrush, &pForm);
        g.FillPath(&cardBrush, &pResult);

        // ----------------------------------------------------
        // [여기에 아래 두 줄을 추가해 보세요!]
        // 오른쪽(응답 정보) 카드에만 아주 연한 경계선(kCardBorder)을 그려줍니다.
        Gdiplus::Pen borderPen(Gdiplus::Color(255, GetRValue(kCardBorder), GetGValue(kCardBorder), GetBValue(kCardBorder)), 1.0f);
        g.DrawPath(&borderPen, &pResult);


        // result section title bar (blue pill)
        {
            Gdiplus::GraphicsPath bp;
            ModernUIGfx::AddRoundRect(bp,
                Gdiplus::RectF((Gdiplus::REAL)(rcResult.left+SX(14)),
                               (Gdiplus::REAL)(rcResult.top+SX(15)),
                               (Gdiplus::REAL)SX(4),(Gdiplus::REAL)SX(14)),SX(2));
            Gdiplus::SolidBrush bb(Gdiplus::Color(255,GetRValue(kBlueText),GetGValue(kBlueText),GetBValue(kBlueText)));
            g.FillPath(&bb,&bp);
        }
        // amount display bg
        {
            int segY2 = rcForm.top + SX(14), amtY = segY2 + SX(CSegmentCtrl::kBarH + 6) + SX(16);
            CRect rcAmt(rcForm.left + SX(20), amtY, rcForm.right - SX(20), amtY + SX(52));
            Gdiplus::GraphicsPath ap; AddRRP(ap, rcAmt, SX(8));

            // [수정] 테두리(선)를 절대 그리지 않으며, 색상을 ShopSetupDlg처럼 
            // 맑고 경계선이 안 보이는 플랫한 연한 파스텔 블루톤으로 변경합니다.
            Gdiplus::SolidBrush amtBg(Gdiplus::Color(255, 240, 246, 255));
            g.FillPath(&amtBg, &ap);
        }
        // result dividers
        {
            const int rRowH2=SX(kResRowH);
            int rTY2=rcResult.top+SX(44);
            int rL2=rcResult.left+SX(14), rR22=rcResult.right-SX(14);
            Gdiplus::Pen pen(Gdiplus::Color(255,GetRValue(kDivider),GetGValue(kDivider),GetBValue(kDivider)),1.f);
            for (int row=1; row<kNumResults; row++) {
                float ly=(float)(rTY2+rRowH2*row);
                g.DrawLine(&pen,(float)rL2,ly,(float)rR22,ly);
            }
        }
    } // GDI+ scope ends, flushed to mem

    // --- Phase 2: GDI text ---
    HDC hRaw=mem.GetSafeHdc();
    ::SetBkMode(hRaw,TRANSPARENT);

    // header
    {
        wchar_t wT[64]={},wS[256]={};
        ::MultiByteToWideChar(CP_ACP,0,_T("결제"),-1,wT,64);
        ::MultiByteToWideChar(CP_ACP,0,_T("신용 및 현금영수증 거래를 진행합니다"),-1,wS,256);
        ModernUIHeader::Draw(hRaw,
            (float)(rcMain.left+SX(14)),(float)(rcMain.top+SX(16)),(float)SX(44),
            ModernUIHeader::IconType::Transaction,wT,wS,
            (HFONT)m_fontTitle.GetSafeHandle(),(HFONT)m_fontSub.GetSafeHandle(),
            rcMain.left+SX(6),rcMain.top+SX(76),rcMain.right-SX(6));
    }
    // result section title
    {
        HFONT hO=(HFONT)::SelectObject(hRaw,m_fontSection.GetSafeHandle());
        ::SetTextColor(hRaw,kSectionText);
        ::SetBkMode(hRaw,TRANSPARENT);
        ::TextOut(hRaw,rcResult.left+SX(26),rcResult.top+SX(14),_T("응답 정보"),lstrlen(_T("응답 정보")));
        ::SelectObject(hRaw,hO);
    }
    // badge
    if (!m_strBadge.IsEmpty()) {
        HFONT hOB=(HFONT)::SelectObject(hRaw,m_fontBadge.GetSafeHandle());
        SIZE sz={}; ::GetTextExtentPoint32(hRaw,m_strBadge,m_strBadge.GetLength(),&sz);
        ::SelectObject(hRaw,hOB);
        int bw=sz.cx+SX(16), bh=SX(20);
        int bx=rcResult.right-SX(14)-bw, by2=rcResult.top+SX(12);
        {
            Gdiplus::Graphics g2(hRaw);
            g2.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            COLORREF bFill=m_bBadgeOk?RGB(230,245,235):RGB(255,235,235);
            Gdiplus::GraphicsPath bPath; AddRRP(bPath,CRect(bx,by2,bx+bw,by2+bh),SX(6));
            Gdiplus::SolidBrush bBr(Gdiplus::Color(255,GetRValue(bFill),GetGValue(bFill),GetBValue(bFill)));
            g2.FillPath(&bBr,&bPath);
        }
        COLORREF bText=m_bBadgeOk?RGB(30,130,60):kRedText;
        hOB=(HFONT)::SelectObject(hRaw,m_fontBadge.GetSafeHandle());
        ::SetTextColor(hRaw,bText); ::SetBkMode(hRaw,TRANSPARENT);
        RECT rcBT={bx,by2,bx+bw,by2+bh};
        ::DrawText(hRaw,m_strBadge,m_strBadge.GetLength(),&rcBT,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        ::SelectObject(hRaw,hOB);
    }
    // amount display text (좌/우 1줄 슬림 배치)
    {
        int segY2 = rcForm.top + SX(14), amtY = segY2 + SX(CSegmentCtrl::kBarH + 6) + SX(16);
        CRect rcAmt(rcForm.left + SX(20), amtY, rcForm.right - SX(20), amtY + SX(52)); // 52로 슬림하게 맞춤

        // 라벨: 진하고 큰 폰트(m_fontEdit) 적용, 좌측 세로 중앙 정렬
        HFONT hOL = (HFONT)::SelectObject(hRaw, m_fontLabel.GetSafeHandle());
        ::SetTextColor(hRaw, RGB(51, 61, 75)); // 까맣고 진한 텍스트
        ::SetBkMode(hRaw, TRANSPARENT);
        RECT rcLbl = { rcAmt.left + SX(16), rcAmt.top, rcAmt.right, rcAmt.bottom };
        ::DrawText(hRaw, _T("최종 결제 예정 금액"), -1, &rcLbl, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        ::SelectObject(hRaw, hOL);

        // 금액: 우측 세로 중앙 정렬
        bool bCancelMode=(m_eMode==MODE_CREDIT_CANCEL||m_eMode==MODE_CASH_CANCEL);
        auto parseN=[](CWnd& e)->long long {
            CString s; e.GetWindowText(s); s.Trim(); s.Remove(_T(','));
            return s.IsEmpty()?0LL:(long long)_ttoi64(s);
        };
        long long nTotal=parseN(m_edtSupply);
        if (!bCancelMode)
            nTotal+=parseN(m_edtTax)+parseN(m_edtTip)+parseN(m_edtTaxFree);
        CString sAmt; sAmt.Format(_T("%I64d"),nTotal); sAmt+=_T(" 원");

        HFONT hOA = (HFONT)::SelectObject(hRaw, m_fontAmount.GetSafeHandle());
        ::SetTextColor(hRaw, RGB(49, 130, 246)); // 토스 스타일 프라이머리 블루
        ::SetBkMode(hRaw, TRANSPARENT);
        RECT rcAV = { rcAmt.left, rcAmt.top, rcAmt.right - SX(16), rcAmt.bottom };
        ::DrawText(hRaw, sAmt, sAmt.GetLength(), &rcAV, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        ::SelectObject(hRaw, hOA);
    }

    dc.BitBlt(0,0,cl.Width(),cl.Height(),&mem,0,0,SRCCOPY);
    mem.SelectObject(pOld);
}

void CTransDlg::OnSize(UINT t,int cx,int cy)
{
    CDialog::OnSize(t,cx,cy);
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

void CTransDlg::OnTabSelChange(NMHDR*,LRESULT* pResult)
{
    int sel=m_segCtrl.GetCurSel();
    if (sel>=0&&sel<=3) SetMode((ETransMode)sel);
    *pResult=0;
}

BOOL CTransDlg::OnCommand(WPARAM wParam,LPARAM lParam)
{
    UINT nID=LOWORD(wParam);
    if (HIWORD(wParam)==BN_CLICKED) {
        if (nID==IDC_TRANS_BTN_CLOSE) { EndDialog(IDCANCEL); return TRUE; }
        if (nID==IDC_TRANS_BTN_RUN) {
            CString err;
            if (!ValidateCurrentMode(err)) { CModernMessageBox::Warning(err,this); return TRUE; }
            CString amt; m_edtSupply.GetWindowText(amt); amt.Trim();
            bool bCash=(m_eMode==MODE_CASH_APPROVAL||m_eMode==MODE_CASH_CANCEL);
            bool bCancel=(m_eMode==MODE_CREDIT_CANCEL||m_eMode==MODE_CASH_CANCEL);
            SetResultValue(0, _T("2026-04-15 14:30:05"));
            SetResultValue(1, _T("0000"));
            SetResultValue(2, bCancel?_T("정상취소"):_T("정상승인"),FALSE,TRUE);
            SetResultValue(3, bCancel?_T("-"):_T("30018492"),TRUE);
            SetResultValue(4, bCancel?_T("취소가 완료되었습니다."):_T("거래가 성공적으로 완료되었습니다."));
            SetResultValue(5, _T("KFTC_T001"));
            SetResultValue(6, bCash?_T("010-12**-****"):_T("9410-****-****-1234"));
            SetResultValue(7, bCash?_T("-"):_T("신한카드"));
            SetResultValue(8, _T("04"));
            SetResultValue(9, bCash?_T("-"):_T("신한카드"));
            SetResultValue(10,bCash?_T("-"):_T("신한카드"));
            SetResultValue(11,_T("04"));
            SetResultValue(12,bCash?_T("-"):_T("개인 / 신용"));
            SetResultValue(13,bCash?_T("-"):_T("삼성페이"));
            { CString n; n.Format(_T("[%s] %s"),(LPCTSTR)GetCurrentModeName(),(LPCTSTR)amt); SetResultValue(14,n); }
            m_strBadge=bCancel?_T("정상 취소"):_T("정상 승인"); m_bBadgeOk=TRUE;
            UpdateResultControls(); return TRUE;
        }
    }
    if (HIWORD(wParam)==EN_CHANGE) {
        UINT cID=LOWORD(wParam);
        if (cID==IDC_TRANS_EDIT_SUPPLY||cID==IDC_TRANS_EDIT_TAX||
            cID==IDC_TRANS_EDIT_TIP||cID==IDC_TRANS_EDIT_TAXFREE)
            Invalidate(FALSE);
    }
    return CDialog::OnCommand(wParam,lParam);
}
