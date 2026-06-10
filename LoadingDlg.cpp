// LoadingDlg.cpp
#include "stdafx.h"
#include "LoadingDlg.h"
#include <process.h>

#define TIMER_SPIN  1
#define SPIN_SWEEP  270.0f
#define SPIN_STEP   8

BEGIN_MESSAGE_MAP(CLoadingDlg, CWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_MESSAGE(WM_LOADING_DONE, OnLoadingDone)
END_MESSAGE_MAP()

CLoadingDlg::CLoadingDlg()
    : m_hwndNotify(NULL), m_hStopEvent(NULL), m_hThread(NULL), m_nAngle(0)
{}

CLoadingDlg::~CLoadingDlg()
{
    if (m_hStopEvent) { CloseHandle(m_hStopEvent); m_hStopEvent = NULL; }
    if (m_hThread)    { CloseHandle(m_hThread);    m_hThread    = NULL; }
}

void CLoadingDlg::PostNcDestroy()
{
    delete this;
}

int CLoadingDlg::SX(int px) const
{
    return ModernUIDpi::Scale(m_hWnd, px);
}

BOOL CLoadingDlg::Start(CWnd* pParent, HWND hwndNotify)
{
    m_hwndNotify = hwndNotify;

    m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!m_hStopEvent) return FALSE;

    LPCTSTR lpszClass = AfxRegisterWndClass(
        CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
        ::LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW + 1), NULL);

    const int W = 220, H = 130;
    CRect rcParent;
    if (pParent)
        pParent->GetWindowRect(&rcParent);
    else
        rcParent.SetRect(0, 0,
            ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN));

    int x = rcParent.left + (rcParent.Width()  - W) / 2;
    int y = rcParent.top  + (rcParent.Height() - H) / 2;

    if (!CreateEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                  lpszClass, NULL, WS_POPUP | WS_VISIBLE,
                  x, y, W, H,
                  pParent ? pParent->GetSafeHwnd() : NULL, NULL))
        return FALSE;

    UINT tid = 0;
    m_hThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, &tid);
    return (m_hThread != NULL);
}

void CLoadingDlg::Stop()
{
    if (m_hStopEvent) SetEvent(m_hStopEvent);
}

void CLoadingDlg::Cancel()
{
    m_hwndNotify = NULL; // 부모 통보 억제
    Stop();
    if (GetSafeHwnd()) DestroyWindow();
}

UINT WINAPI CLoadingDlg::WorkerThread(LPVOID pParam)
{
    CLoadingDlg* p = static_cast<CLoadingDlg*>(pParam);
    // 3초 대기 ? Stop()/Cancel() 신호 수신 시 즉시 탈출
    WaitForSingleObject(p->m_hStopEvent, 3000);
    if (p->GetSafeHwnd())
        p->PostMessage(WM_LOADING_DONE, 0, 0);
    return 0;
}

int CLoadingDlg::OnCreate(LPCREATESTRUCT lpCS)
{
    if (CWnd::OnCreate(lpCS) == -1) return -1;
    SetTimer(TIMER_SPIN, 30, NULL);
    return 0;
}

void CLoadingDlg::OnTimer(UINT_PTR nID)
{
    if (nID == TIMER_SPIN) {
        m_nAngle = (m_nAngle + SPIN_STEP) % 360;
        Invalidate(FALSE);
    }
    CWnd::OnTimer(nID);
}

void CLoadingDlg::OnDestroy()
{
    KillTimer(TIMER_SPIN);
    m_hwndNotify = NULL; // 이후 스레드 PostMessage 가 부모를 건드리지 않도록 차단
    if (m_hStopEvent) SetEvent(m_hStopEvent);
    if (m_hThread) {
        WaitForSingleObject(m_hThread, 2000);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
    CWnd::OnDestroy();
}

void CLoadingDlg::OnPaint()
{
    CPaintDC dc(this);
    CRect cl;
    GetClientRect(&cl);

    CDC mem;
    mem.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, cl.Width(), cl.Height());
    CBitmap* pOld = mem.SelectObject(&bmp);

    mem.FillSolidRect(cl, RGB(255, 255, 255));

    Gdiplus::Graphics g(mem.GetSafeHdc());
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    // 외곽 테두리
    Gdiplus::Pen penBorder(Gdiplus::Color(255, 210, 215, 225), 1.0f);
    g.DrawRectangle(&penBorder, 0, 0, cl.Width() - 1, cl.Height() - 1);

    // 스피너 중심 좌표
    float cx = cl.Width()  * 0.5f;
    float cy = cl.Height() * 0.5f - 14.0f;
    float r  = 22.0f;

    // 배경 트랙 (반투명)
    Gdiplus::Pen penTrack(Gdiplus::Color(40, 27, 100, 242), 4.0f);
    g.DrawEllipse(&penTrack, cx - r, cy - r, r * 2.0f, r * 2.0f);

    // 회전 호 (KFTC 블루)
    Gdiplus::Pen penArc(Gdiplus::Color(255, 27, 100, 242), 4.0f);
    penArc.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
    g.DrawArc(&penArc, cx - r, cy - r, r * 2.0f, r * 2.0f,
              (Gdiplus::REAL)m_nAngle, SPIN_SWEEP);

    // "처리 중..." 텍스트
    LOGFONT lf = {};
    ::GetObject((HFONT)::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    ModernUIFont::ApplyUIFontFace(lf);
    lf.lfHeight  = -13;
    lf.lfWeight  = FW_NORMAL;
    lf.lfQuality = CLEARTYPE_QUALITY;
    CFont font;
    font.CreateFontIndirect(&lf);
    CFont* pOldFont = mem.SelectObject(&font);
    mem.SetBkMode(TRANSPARENT);
    mem.SetTextColor(RGB(107, 114, 128));
    CRect rcTxt(0, (int)(cy + r) + 12, cl.Width(), cl.Height() - 6);
    mem.DrawText(_T("처리 중..."), &rcTxt,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    mem.SelectObject(pOldFont);

    dc.BitBlt(0, 0, cl.Width(), cl.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(pOld);
}

LRESULT CLoadingDlg::OnLoadingDone(WPARAM, LPARAM)
{
    KillTimer(TIMER_SPIN);
    ShowWindow(SW_HIDE);

    // PostNcDestroy 로 자동 소멸 전에 부모에 완료 통보
    HWND hwndNotify = m_hwndNotify;
    m_hwndNotify = NULL;
    if (hwndNotify && ::IsWindow(hwndNotify))
        ::PostMessage(hwndNotify, WM_LOADING_DONE, 0, 0);

    DestroyWindow(); // -> OnDestroy -> PostNcDestroy -> delete this
    return 0;
}