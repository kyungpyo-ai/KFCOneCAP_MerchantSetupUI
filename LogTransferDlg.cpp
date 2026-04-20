// LogTransferDlg.cpp
#include "stdafx.h"
#include "LogTransferDlg.h"
#include "ModernMessageBox.h"
#include <Uxtheme.h>
#pragma comment(lib, "UxTheme.lib")


#define IDC_CAL_POPUP 5001 // 팝업 달력용 고유 ID 추가
#define IDC_MODERN_DATE_BTN 5002 // [추가] 순수한 새 버튼을 위한 완전히 독립된 ID!

IMPLEMENT_DYNAMIC(CLogTransferDlg, CDialog)

BEGIN_MESSAGE_MAP(CLogTransferDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_CTLCOLOR()
    ON_WM_NCHITTEST()
    ON_BN_CLICKED(IDC_LOG_BTN_SEND,       OnBtnSend)
    ON_BN_CLICKED(IDC_LOG_BTN_CANCEL_DLG, OnBtnCancel)
    ON_BN_CLICKED(IDC_LOG_BTN_CLOSE,      OnBtnClose)

    // [새로 추가해야 할 부분] 오너드로우 및 버튼 클릭, 달력 선택 이벤트 연결
    ON_WM_DRAWITEM()
    // [수정] 클릭 이벤트를 새로운 ID로 연결합니다!
    ON_BN_CLICKED(IDC_MODERN_DATE_BTN, OnBtnDatePicker)
    ON_NOTIFY(MCN_SELECT, IDC_CAL_POPUP, OnCalSelect)
END_MESSAGE_MAP()

CLogTransferDlg::CLogTransferDlg(CWnd* pParent)
    : CDialog(IDD_LOG_TRANSFER_DIALOG, pParent)
{}

CLogTransferDlg::~CLogTransferDlg() {}

void CLogTransferDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

LRESULT CLogTransferDlg::OnNcHitTest(CPoint point)
{
    LRESULT hit = CDialog::OnNcHitTest(point);
    if (hit == HTCLIENT) return HTCAPTION;
    return hit;
}

int CLogTransferDlg::SX(int px) const { return ModernUIDpi::Scale(m_hWnd, px); }

void CLogTransferDlg::SetClientSize(int cx, int cy)
{
    CRect rcWnd, rcCli;
    GetWindowRect(&rcWnd); GetClientRect(&rcCli);
    int ncW = rcWnd.Width()  - rcCli.Width();
    int ncH = rcWnd.Height() - rcCli.Height();
    SetWindowPos(NULL, 0, 0, cx + max(0,ncW), cy + max(0,ncH), SWP_NOMOVE | SWP_NOZORDER);
}

void CLogTransferDlg::EnsureFonts()
{
    if (m_fontTitle.GetSafeHandle()) return;
    ModernUIFont::EnsureFontsLoaded();

    LOGFONT lf = {};
    lf.lfCharSet        = HANGUL_CHARSET;
    lf.lfQuality        = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    ModernUIFont::ApplyUIFontFace(lf);

    lf.lfHeight = -SX(18); lf.lfWeight = FW_BOLD;     m_fontTitle.CreateFontIndirect(&lf);
    lf.lfHeight = -SX(13); lf.lfWeight = FW_NORMAL;   m_fontDesc.CreateFontIndirect(&lf);
    lf.lfHeight = -SX(13); lf.lfWeight = FW_SEMIBOLD; m_fontLabel.CreateFontIndirect(&lf);
}


BOOL CLogTransferDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_BORDER | WS_DLGFRAME, 0);
    ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE | WS_EX_STATICEDGE, 0);

    m_brBg.CreateSolidBrush(RGB(255, 255, 255));
    m_brDateBg.CreateSolidBrush(RGB(255, 255, 255));
    EnsureFonts();
    SetClientSize(SX(440), SX(282));

    // Close "X"
    m_btnClose.Create(_T("X"), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        CRect(0,0,0,0), this, IDC_LOG_BTN_CLOSE);
    m_btnClose.SetButtonStyle(ButtonStyle::Auto);
    m_btnClose.SetColors(RGB(255,255,255), RGB(240,242,245), RGB(100,114,132));
    m_btnClose.SetHoverTextColor(RGB(25,31,40));
    m_btnClose.SetUnderlayColor(RGB(255,255,255));
    m_btnClose.SetTextPx(SX(14));
    // 1. 유령 숨기기
    CWnd* pGhost = GetDlgItem(IDC_LOG_DATE_PICKER);
    if (pGhost) { pGhost->ShowWindow(SW_HIDE); pGhost->MoveWindow(0, 0, 0, 0); }

    // 2. 버튼 생성 및 텍스트 셋팅 (폰트 셋팅이 먼저 와야 함)
    m_strDate = CTime::GetCurrentTime().Format(_T("%Y-%m-%d"));
    // 1. 글자가 테두리에 너무 딱 붙지 않도록 앞에 띄어쓰기를 살짝 추가합니다.
    CString strDisplayDate = _T("   ") + m_strDate;

    // 2. [추가] BS_LEFT 속성을 넣어서 왼쪽 정렬로 만듭니다!
    m_btnDatePicker.Create((LPCTSTR)strDisplayDate, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_LEFT,
        CRect(0, 0, 0, 0), this, IDC_MODERN_DATE_BTN);

    m_btnDatePicker.SetFont(&m_fontDesc);
    m_btnDatePicker.SetWindowText((LPCTSTR)strDisplayDate);

    // 3. 색상 세팅
    m_btnDatePicker.SetButtonStyle(ButtonStyle::Auto);
    m_btnDatePicker.SetColors(RGB(242, 244, 246), RGB(232, 236, 240), RGB(17, 24, 39));
    m_btnDatePicker.SetHoverTextColor(RGB(27, 100, 242));
    m_btnDatePicker.SetUnderlayColor(RGB(255, 255, 255));



    // Cancel
    m_btnCancel.Create(_T("취소"), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        CRect(0,0,0,0), this, IDC_LOG_BTN_CANCEL_DLG);
    m_btnCancel.SetButtonStyle(ButtonStyle::Auto);
    m_btnCancel.SetColors(RGB(242,244,246), RGB(228,232,236), RGB(78,89,104));
    m_btnCancel.SetUnderlayColor(RGB(255,255,255));
    m_btnCancel.SetFont(&m_fontDesc);

    // Send
    m_btnSend.Create(_T("전송 요청"), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        CRect(0,0,0,0), this, IDC_LOG_BTN_SEND);
    m_btnSend.SetButtonStyle(ButtonStyle::Auto);
    m_btnSend.SetColors(RGB(27,100,242), RGB(20,90,220), RGB(255,255,255));
    m_btnSend.SetUnderlayColor(RGB(255,255,255));
    m_btnSend.SetFont(&m_fontDesc);

    LayoutControls();
    CenterWindow();
    return TRUE;
}

void CLogTransferDlg::LayoutControls()
{
    CRect rc; GetClientRect(&rc);
    int cw   = rc.Width();
    int padL = SX(28);
    int padR = SX(28);

    int closeSize = SX(32);
    m_btnClose.MoveWindow(cw - SX(10) - closeSize, SX(10), closeSize, closeSize);

  
    // 버튼을 정 사이즈로 배치합니다.
    m_btnDatePicker.MoveWindow(padL, SX(148), cw - padL - padR, SX(42));

    int btnH    = SX(42);
    int btnY    = SX(220);
    int sendW   = SX(116);
    int cancelW = SX(92);
    int sendX   = cw - padR - sendW;
    int cancelX = sendX - SX(10) - cancelW;
    m_btnCancel.MoveWindow(cancelX, btnY, cancelW, btnH);
    m_btnSend.MoveWindow(sendX,   btnY, sendW,   btnH);
}

void CLogTransferDlg::OnPaint()
{
    CPaintDC dc(this);
    CRect cl; GetClientRect(&cl);

    CDC mem; mem.CreateCompatibleDC(&dc);
    CBitmap bmp; bmp.CreateCompatibleBitmap(&dc, cl.Width(), cl.Height());
    CBitmap* pOld = mem.SelectObject(&bmp);

    mem.FillSolidRect(cl, RGB(255, 255, 255));

    int cw   = cl.Width();
    int padL = SX(28);
    int padR = SX(28);

    EnsureFonts();
    mem.SetBkMode(TRANSPARENT);

    // Title
    CFont* pOldFont = mem.SelectObject(&m_fontTitle);
    mem.SetTextColor(RGB(17, 24, 39));
    CRect rcTitle(padL, SX(22), cw - SX(54), SX(22) + SX(28));
    mem.DrawText(_T("로그 전송"), &rcTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // --- GDI+ 초기화 (선명한 안티앨리어싱 적용) ---
    Gdiplus::Graphics g(mem.GetSafeHdc());
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    // 1. 상단 구분선 (부드러운 선)
    Gdiplus::Pen penLine(Gdiplus::Color(255, 229, 233, 240), 1.0f);
    g.DrawLine(&penLine, 0, SX(62), cw, SX(62));

    // Description 텍스트
    mem.SelectObject(&m_fontDesc);
    mem.SetTextColor(RGB(107, 114, 128));
    CRect rcDesc(padL, SX(72), cw - padR, SX(72) + SX(56));
    mem.DrawText(_T("단말기의 결제 이력 및 시스템 오류 로그를 추출하여\n안전하게 서버로 전송합니다"), &rcDesc, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);

    // Label 텍스트
    mem.SelectObject(&m_fontLabel);
    mem.SetTextColor(RGB(17, 24, 39));
    CRect rcLabel(padL, SX(132), cw - padR, SX(132) + SX(16));
    mem.DrawText(_T("추출할 로그 일자"), &rcLabel, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);



    // 3. 하단 구분선
    g.DrawLine(&penLine, 0, SX(208), cw, SX(208));

    // 4. 다이얼로그 전체 외곽선 (팝업창이 배경에 묻히지 않고 명확히 구분되도록)
    Gdiplus::Pen penDlgBorder(Gdiplus::Color(255, 210, 215, 225), 1.0f);
    g.DrawRectangle(&penDlgBorder, 0, 0, cl.Width() - 1, cl.Height() - 1);

    mem.SelectObject(pOldFont);
    dc.BitBlt(0, 0, cl.Width(), cl.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(pOld);
}

HBRUSH CLogTransferDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (pWnd && pWnd->GetDlgCtrlID() == IDC_LOG_DATE_PICKER) {
        pDC->SetBkColor(RGB(255, 255, 255));
        pDC->SetTextColor(RGB(17, 24, 39));
        return m_brDateBg;
    }
    pDC->SetBkColor(RGB(255, 255, 255));
    return m_brBg;
}

// --- 버튼 클릭 시 달력을 띄우는 함수 ---
void CLogTransferDlg::OnBtnDatePicker()
{
    ShowCalendar();
}

// --- 오너드로우: 버튼을 모던한 카카오/토스 입력창 스타일로 직접 그립니다 ---
void CLogTransferDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
    CDialog::OnDrawItem(nIDCtl, lpDIS);
}

// --- 팝업 달력 생성 및 표시 ---
void CLogTransferDlg::ShowCalendar()
{
    // 1. 공통 컨트롤 초기화
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);

    // 2. 이미 달력이 켜져 있으면 무시
    if (m_calCtrl.GetSafeHwnd() && m_calCtrl.IsWindowVisible()) {
        return;
    }

    // 3. 달력이 안 만들어져 있다면 최초 1회 생성
    if (!m_calCtrl.GetSafeHwnd()) {
        // [핵심 수정] WS_POPUP 속성일 때는 HMENU 자리에 반드시 NULL을 넣어야 창이 정상 생성됩니다!
        HWND hwndCal = ::CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, MONTHCAL_CLASS, NULL,
            WS_POPUP | WS_BORDER, 0, 0, 200, 200, m_hWnd, NULL, AfxGetInstanceHandle(), NULL);

        if (hwndCal) {
            // 창이 성공적으로 만들어진 후에, OS 수준에서 강제로 ID(IDC_CAL_POPUP)를 부여합니다.
            // 이렇게 해야 메시지 맵의 ON_NOTIFY(MCN_SELECT)가 정상적으로 이벤트를 받을 수 있습니다.
            ::SetWindowLongPtr(hwndCal, GWLP_ID, IDC_CAL_POPUP);
            m_calCtrl.Attach(hwndCal);
        }
        else {
            return; // 윈도우가 창 생성을 거부하면 안전하게 탈출
        }
    }

    // 4. 달력이 생성된 직후에 모던 UI KFTC 색상을 칠해줍니다.
    m_calCtrl.SetColor(MCSC_TITLEBK, RGB(27, 100, 242));
    m_calCtrl.SetColor(MCSC_TITLETEXT, RGB(255, 255, 255));
    m_calCtrl.SetColor(MCSC_MONTHBK, RGB(255, 255, 255));
    m_calCtrl.SetColor(MCSC_TEXT, RGB(17, 24, 39));
    m_calCtrl.SetColor(MCSC_TRAILINGTEXT, RGB(180, 190, 200));
    m_calCtrl.SetColor(MCSC_BACKGROUND, RGB(245, 247, 250));

    // 5. 달력 위치를 '버튼 바로 아래'에 정확히 맞춥니다.
    CRect rcBtn;
    m_btnDatePicker.GetWindowRect(&rcBtn);
    CRect rcMin;
    m_calCtrl.GetMinReqRect(&rcMin);
    m_calCtrl.SetWindowPos(&CWnd::wndTopMost, rcBtn.left, rcBtn.bottom + SX(4), rcMin.Width(), rcMin.Height(), SWP_SHOWWINDOW);

    // 6. 현재 문자열로 저장된 날짜를 분석해서 달력에 셋팅합니다.
    int y, m, d;
    if (_stscanf_s(m_strDate, _T("%d-%d-%d"), &y, &m, &d) == 3) {
        CTime t(y, m, d, 0, 0, 0);
        m_calCtrl.SetCurSel(t);
    }

    // 7. 포커스를 주어 키보드 방향키 조작도 가능하게 만듭니다.
    m_calCtrl.SetFocus();
}


void CLogTransferDlg::HideCalendar()
{
    if (m_calCtrl.GetSafeHwnd()) m_calCtrl.ShowWindow(SW_HIDE);
}

void CLogTransferDlg::OnCalSelect(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMSELCHANGE* pSC = (NMSELCHANGE*)pNMHDR;

    // 1. 선택한 날짜로 변수 업데이트
    m_strDate.Format(_T("%04d-%02d-%02d"), pSC->stSelStart.wYear, pSC->stSelStart.wMonth, pSC->stSelStart.wDay);

    // 2. 창 즉시 닫기
    HideCalendar();

    // 3. 버튼 텍스트에 여백을 포함하여 업데이트 후 화면 강제 갱신
    CString strDisplay = _T("   ") + m_strDate;
    m_btnDatePicker.SetWindowText(strDisplay);
    m_btnDatePicker.Invalidate();
    m_btnDatePicker.UpdateWindow(); // 즉각적인 시각적 피드백 제공

    *pResult = 0;
}
// --- Hover 감지 및 달력 팝업 제어 ---
BOOL CLogTransferDlg::PreTranslateMessage(MSG* pMsg)
{

    // 달력이 떠 있을 때의 동작 제어
    if (m_calCtrl.GetSafeHwnd() && m_calCtrl.IsWindowVisible()) {

        // 1. 마우스로 달력 밖의 다른 영역을 클릭했을 때 달력 닫기
        if (pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_RBUTTONDOWN || pMsg->message == WM_NCLBUTTONDOWN) {
            CRect rcCal;
            m_calCtrl.GetWindowRect(&rcCal);

            if (!rcCal.PtInRect(pMsg->pt)) {
                HideCalendar();
                m_btnDatePicker.Invalidate(FALSE); // 달력이 닫힐 때 버튼도 원래 상태로 돌아가도록 업데이트
                return TRUE; // 다른 버튼이 눌리는 등의 추가 동작 방지
            }
        }

        // 2. 키보드 ESC 키를 눌렀을 때 달력 닫기
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) {
            HideCalendar();
            m_btnDatePicker.Invalidate(FALSE);
            return TRUE;
        }
    }
    return CDialog::PreTranslateMessage(pMsg);
}

// --- [수정] 전송 버튼 클릭 시 m_strDate를 사용하도록 변경 ---
void CLogTransferDlg::OnBtnSend()
{
    if (m_strDate.IsEmpty()) {
        CModernMessageBox::Warning(_T("날짜를 선택해주세요"), this);
        return;
    }
    CString strMsg;
    strMsg.Format(_T("%s 로그 전송"), (LPCTSTR)m_strDate);
    CModernMessageBox::Info(strMsg, this);
    EndDialog(IDOK);
}


void CLogTransferDlg::OnBtnCancel() { EndDialog(IDCANCEL); }
void CLogTransferDlg::OnBtnClose()  { EndDialog(IDCANCEL); }
void CLogTransferDlg::OnOK()        {}
void CLogTransferDlg::OnCancel()    { EndDialog(IDCANCEL); }