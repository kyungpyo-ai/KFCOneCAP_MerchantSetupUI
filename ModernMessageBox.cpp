// ModernMessageBox.cpp - Modern 알림창 구현
#include "stdafx.h"
#include "ModernMessageBox.h"

IMPLEMENT_DYNAMIC(CModernMessageBox, CDialog)

BEGIN_MESSAGE_MAP(CModernMessageBox, CDialog)
    ON_WM_PAINT()
    ON_WM_CTLCOLOR()
    ON_WM_DESTROY()
    ON_WM_NCHITTEST()
    ON_BN_CLICKED(IDYES, OnYes)
    ON_BN_CLICKED(IDNO, OnNo)
END_MESSAGE_MAP()

CModernMessageBox::CModernMessageBox(
    CWnd* pParent, const CString& strMessage, const CString& strCaption,
    ModernMsgBoxType type, ModernMsgBoxButtons buttons)
    : CDialog(IDD_MODERN_MESSAGEBOX, pParent)
    , m_strMessage(strMessage), m_strCaption(strCaption)
    , m_type(type), m_buttons(buttons), m_gdiplusToken(0)
    , m_pIconFont(nullptr), m_pIconFontFallback(nullptr)
    , m_pIconFmt(nullptr), m_pIconBgBrush(nullptr), m_pIconSymBrush(nullptr)
{}

CModernMessageBox::~CModernMessageBox() {}

void CModernMessageBox::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

// 창 제목 표시줄이 없어도 드래그로 창을 움직일 수 있게 해주는 기능 (선택)
LRESULT CModernMessageBox::OnNcHitTest(CPoint point)
{
    LRESULT hit = CDialog::OnNcHitTest(point);
    if (hit == HTCLIENT)
        return HTCAPTION;
    return hit;
}

// Pretendard 기반 DPI 스케일 폰트 생성
void CModernMessageBox::EnsureFonts()
{
    if (m_fontTitle.GetSafeHandle()) return;
    ModernUIFont::EnsureFontsLoaded();

    LOGFONT lf = {};
    lf.lfCharSet = HANGUL_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    ModernUIFont::ApplyUIFontFace(lf);

    // 제목은 크게(15), 본문은 작게(13) 분리
    lf.lfHeight = -SX(15); lf.lfWeight = FW_BOLD;    m_fontTitle.CreateFontIndirect(&lf);
    lf.lfHeight = -SX(13); lf.lfWeight = FW_NORMAL;  m_fontBody.CreateFontIndirect(&lf);

    // DrawIcon cached GDI+ objects
    struct IconColor { BYTE r1,g1,b1,rt,gt,bt; };
    static const IconColor kTC[] = {
        {235,244,255, 27,100,242}, {255,248,230,245,166, 35},
        {255,235,238,240, 68, 82}, {230,248,238, 49,172,100},
        {235,244,255, 27,100,242},
    };
    const int ti = max(0, min(4, (int)m_type));
    const float iFontH = SX(40) * 0.55f;
    if (!m_pIconFont)         m_pIconFont        = new Gdiplus::Font(L"Malgun Gothic", iFontH, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    if (!m_pIconFontFallback) m_pIconFontFallback = new Gdiplus::Font(Gdiplus::FontFamily::GenericSansSerif(), iFontH, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    if (!m_pIconFmt) { m_pIconFmt = new Gdiplus::StringFormat(); m_pIconFmt->SetAlignment(Gdiplus::StringAlignmentCenter); m_pIconFmt->SetLineAlignment(Gdiplus::StringAlignmentCenter); }
    if (!m_pIconBgBrush)  m_pIconBgBrush  = new Gdiplus::SolidBrush(Gdiplus::Color(255,kTC[ti].r1,kTC[ti].g1,kTC[ti].b1));
    if (!m_pIconSymBrush) m_pIconSymBrush = new Gdiplus::SolidBrush(Gdiplus::Color(255,kTC[ti].rt,kTC[ti].gt,kTC[ti].bt));
}

void CModernMessageBox::MeasureText(CFont& font, const CString& text, int maxW, int& outH)
{
    CDC* pDC = GetDC(); if (!pDC) { outH = 20; return; }
    CFont* pOld = pDC->SelectObject(&font);
    CRect rc(0, 0, maxW, 2000);
    pDC->DrawText(text, &rc, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
    outH = rc.Height();
    pDC->SelectObject(pOld);
    ReleaseDC(pDC);
}

BOOL CModernMessageBox::OnInitDialog()
{
    CDialog::OnInitDialog();

    ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_BORDER | WS_DLGFRAME, 0);
    ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE | WS_EX_STATICEDGE, 0);

    GdiplusStartupInput gsi;
    GdiplusStartup(&m_gdiplusToken, &gsi, NULL);
    m_brushBg.CreateSolidBrush(RGB(255, 255, 255));
    SetWindowText(m_strCaption);

    // IDC_STATIC_MESSAGE 숨김 - OnPaint에서 직접 렌더링
    CWnd* pMsg = GetDlgItem(IDC_STATIC_MESSAGE);
    if (pMsg) pMsg->ShowWindow(SW_HIDE);

    EnsureFonts();

    int clientH = CalculateHeight();
    SetClientSize(SX(360), clientH);

    CRect rcClient; GetClientRect(&rcClient);
    int btnY = rcClient.bottom - SX(24) - SX(44); // 하단 여백 24, 버튼 높이 44
    SetupButtons(btnY);

    CenterWindow();

    return TRUE;
}

int CModernMessageBox::CalculateHeight()
{
    CString title, body;
    int nPos = m_strMessage.Find(_T('\n'));
    if (nPos >= 0) { title = m_strMessage.Left(nPos); body = m_strMessage.Mid(nPos + 1); }
    else { title = m_strMessage; }

    int textW = SX(274); // 폭 360 - 여백
    int titleH = 0, bodyH = 0;

    MeasureText(m_fontTitle, title, textW, titleH);
    if (!body.IsEmpty()) MeasureText(m_fontBody, body, textW, bodyH);

    int textH = titleH + (body.IsEmpty() ? 0 : SX(8) + bodyH);
    int contentH = max(SX(40), textH); // 아이콘 높이(40)와 비교

    // 상단(24) + 컨텐츠 + 간격(24) + 버튼(44) + 하단(24)
    return SX(24) + contentH + SX(24) + SX(44) + SX(24);
}

void CModernMessageBox::SetClientSize(int cx, int cy)
{
    CRect rcWnd, rcCli;
    GetWindowRect(&rcWnd); GetClientRect(&rcCli);
    int ncW = rcWnd.Width() - rcCli.Width();
    int ncH = rcWnd.Height() - rcCli.Height();
    SetWindowPos(NULL, 0, 0, cx + max(0, ncW), cy + max(0, ncH), SWP_NOMOVE | SWP_NOZORDER);
}

void CModernMessageBox::SetupButtons(int btnY)
{
    CWnd* pOK = GetDlgItem(IDOK);
    CWnd* pCancel = GetDlgItem(IDCANCEL);
    CRect rcClient; GetClientRect(&rcClient);

    int margin = SX(24); // 좌우 여백 확대
    int gap = SX(12);    // 버튼 간격
    int btnH = SX(44);   // 큼직한 높이

    int fullW = rcClient.Width() - (margin * 2);
    int halfW = (fullW - gap) / 2;

    auto ApplyBtn = [&](CModernButton& btn, CWnd* p, BOOL bPrimary, LPCTSTR text, int x, int w) {
        if (!p) return;
        btn.SubclassDlgItem(p->GetDlgCtrlID(), this);
        if (bPrimary)
            btn.SetColors(RGB(27, 100, 242), RGB(20, 90, 220), RGB(255, 255, 255)); // 토스 블루 프라이머리
        else
            btn.SetColors(RGB(242, 244, 246), RGB(230, 232, 236), RGB(78, 89, 104)); // 토스 그레이 세컨더리

        p->SetWindowText(text);
        p->MoveWindow(x, btnY, w, btnH);
        p->ShowWindow(SW_SHOW);
    };

    switch (m_buttons)
    {
    case ModernMsgBoxButtons::OK:
        ApplyBtn(m_btnOK, pOK, TRUE, _T("확인"), margin, fullW);
        if (pCancel) pCancel->ShowWindow(SW_HIDE);
        break;
    case ModernMsgBoxButtons::OKCancel:
        ApplyBtn(m_btnCancel, pCancel, FALSE, _T("취소"), margin, halfW);
        ApplyBtn(m_btnOK, pOK, TRUE, _T("확인"), margin + halfW + gap, halfW);
        break;
    case ModernMsgBoxButtons::YesNo:
        if (pOK)     pOK->SetDlgCtrlID(IDYES);
        if (pCancel) pCancel->SetDlgCtrlID(IDNO);
        if (m_type == ModernMsgBoxType::Question) {
            ApplyBtn(m_btnYes, pOK, TRUE, _T("예"), margin, halfW);
            ApplyBtn(m_btnNo, pCancel, FALSE, _T("아니오"), margin + halfW + gap, halfW);
        } else {
            ApplyBtn(m_btnNo, pCancel, FALSE, _T("아니오"), margin, halfW);
            ApplyBtn(m_btnYes, pOK, TRUE, _T("예"), margin + halfW + gap, halfW);
        }
        break;
    default: break;
    }
}

void CModernMessageBox::OnPaint()
{
    CPaintDC dc(this);
    CRect cl; GetClientRect(&cl);

    CDC mem; mem.CreateCompatibleDC(&dc);
    CBitmap bmp; bmp.CreateCompatibleBitmap(&dc, cl.Width(), cl.Height());
    CBitmap* pOld = mem.SelectObject(&bmp);

    // 배경 흰색
    mem.FillSolidRect(cl, RGB(255, 255, 255));

    Gdiplus::Graphics g(mem.GetSafeHdc());
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    int iconSz = SX(40); // 아이콘 약간 확대
    int iconX = SX(24);
    int iconY = SX(24);

    DrawIcon(g, Gdiplus::RectF((float)iconX, (float)iconY, (float)iconSz, (float)iconSz), m_type);

    EnsureFonts();
    int textL = iconX + iconSz + SX(16);
    int textR = cl.right - SX(24);

    CString title, body;
    int nPos = m_strMessage.Find(_T('\n'));
    if (nPos >= 0) { title = m_strMessage.Left(nPos); body = m_strMessage.Mid(nPos + 1); }
    else { title = m_strMessage; }

    mem.SetBkMode(TRANSPARENT);

    // 1. 제목 높이 먼저 계산
    CFont* pOldFont = mem.SelectObject(&m_fontTitle);
    mem.SetTextColor(RGB(25, 31, 40));
    CRect rcTitleCalc(textL, 0, textR, 2000);
    mem.DrawText(title, &rcTitleCalc, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
    int titleH = rcTitleCalc.Height();

    //  2. 텍스트 세로 중앙 정렬 로직 (본문이 없으면 아이콘 중앙에 맞춤!)
    int drawY = SX(24); // 기본 Y 좌표
    if (body.IsEmpty())
    {
        int iconCenterY = iconY + (iconSz / 2);
        drawY = iconCenterY - (titleH / 2);
    }

    // 3. 계산된 위치에 제목 그리기
    CRect rcTitleDraw(textL, drawY, textR, drawY + titleH);
    mem.DrawText(title, &rcTitleDraw, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);

    // 4. 본문이 있으면 이어서 그리기
    if (!body.IsEmpty()) {
        mem.SelectObject(&m_fontBody);
        mem.SetTextColor(RGB(78, 89, 104));
        int bodyT = rcTitleDraw.bottom + SX(8);
        CRect rcBody(textL, bodyT, textR, bodyT + 2000);
        mem.DrawText(body, &rcBody, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
    }
    mem.SelectObject(pOldFont);

    //  5. 배경과 창을 분리하는 깔끔한 1px 테두리 (필수!)
    CPen borderPen(PS_SOLID, 1, RGB(220, 224, 228)); // 연한 회색 외곽선
    CPen* pOldPen = mem.SelectObject(&borderPen);
    CBrush* pOldBrush = (CBrush*)mem.SelectStockObject(NULL_BRUSH);
    mem.Rectangle(0, 0, cl.right, cl.bottom);
    mem.SelectObject(pOldPen);
    mem.SelectObject(pOldBrush);

    // 화면에 출력
    dc.BitBlt(0, 0, cl.Width(), cl.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(pOld);
}

// DrawIcon: 파스텔톤 플랫 원 + 맑은 고딕 심볼 (CStringW 완벽 회피 적용)
void CModernMessageBox::DrawIcon(Gdiplus::Graphics& g, Gdiplus::RectF rect, ModernMsgBoxType type)
{
    if (rect.Width <= 0 || rect.Height <= 0) return;

    // ?? 회원님의 피드백 반영: 왼쪽 아래 쏠림을 해결하기 위해 
    // offsetX는 플러스(오른쪽), offsetY는 마이너스(위쪽)로 정밀 조정
    struct IconTheme { BYTE r1, g1, b1, rt, gt, bt; WCHAR sym; float offsetX; float offsetY; };
    static const IconTheme themes[] = {
        { 235, 244, 255,   27, 100, 242, L'i',      0.5f, -0.5f },
        { 255, 248, 230,  245, 166,  35, L'!',      1.5f, -1.5f }, // ?? 오른쪽 위로 1.5px 이동
        { 255, 235, 238,  240,  68,  82, L'\u00D7', 1.0f, -1.0f }, // ?? x표도 오른쪽 위로 1px 이동
        { 230, 248, 238,   49, 172, 100, L'\u2713', 0.5f, -0.5f },
        { 235, 244, 255,   27, 100, 242, L'?',      0.5f, -0.5f },
    };
    const IconTheme& th = themes[(int)type];

    // 1. 배경 그리기
    if (m_pIconBgBrush) g.FillEllipse(m_pIconBgBrush, rect);

    // 2. 심볼 준비
    CStringW strSym;
    strSym.AppendChar(th.sym);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit); // ?? 힌팅 옵션 변경으로 가독성 향상

    Gdiplus::RectF textRect = rect;
    textRect.X += th.offsetX;
    textRect.Y += th.offsetY;
    Gdiplus::Font* pF = (m_pIconFont && m_pIconFont->GetLastStatus() == Gdiplus::Ok) ? m_pIconFont : m_pIconFontFallback;
    if (pF && m_pIconFmt && m_pIconSymBrush)
        g.DrawString(strSym.GetString(), strSym.GetLength(), pF, textRect, m_pIconFmt, m_pIconSymBrush);
}
HBRUSH CModernMessageBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_STATIC || nCtlColor == CTLCOLOR_DLG)
        return (HBRUSH)m_brushBg.GetSafeHandle();
    return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CModernMessageBox::OnDestroy()
{
    if (m_gdiplusToken) { GdiplusShutdown(m_gdiplusToken); m_gdiplusToken = 0; }
    if (m_brushBg.GetSafeHandle())   m_brushBg.DeleteObject();
    if (m_fontTitle.GetSafeHandle()) m_fontTitle.DeleteObject();
    if (m_fontBody.GetSafeHandle())  m_fontBody.DeleteObject();
    CDialog::OnDestroy();
}

void CModernMessageBox::OnOK() { CDialog::OnOK(); }
void CModernMessageBox::OnCancel() { CDialog::OnCancel(); }
void CModernMessageBox::OnYes() { EndDialog(IDYES); }
void CModernMessageBox::OnNo() { EndDialog(IDNO); }

// ============================================
// 정적 함수들 (호출부)
// ============================================

INT_PTR CModernMessageBox::Show(
    const CString& strMessage,
    const CString& strCaption,
    ModernMsgBoxType type,
    ModernMsgBoxButtons buttons,
    CWnd* pParent
)
{
    CModernMessageBox dlg(pParent, strMessage, strCaption, type, buttons);
    return dlg.DoModal();
}

INT_PTR CModernMessageBox::Info(const CString& strMessage, CWnd* pParent)
{
    return Show(strMessage, _T("KFTCGiroCAP"), ModernMsgBoxType::Info, ModernMsgBoxButtons::OK, pParent);
}

INT_PTR CModernMessageBox::Warning(const CString& strMessage, CWnd* pParent)
{
    return Show(strMessage, _T("KFTCGiroCAP"), ModernMsgBoxType::Warning, ModernMsgBoxButtons::OK, pParent);
}

INT_PTR CModernMessageBox::Error(const CString& strMessage, CWnd* pParent)
{
    return Show(strMessage, _T("KFTCGiroCAP"), ModernMsgBoxType::Error, ModernMsgBoxButtons::OK, pParent);
}

INT_PTR CModernMessageBox::Success(const CString& strMessage, CWnd* pParent)
{
    return Show(strMessage, _T("KFTCGiroCAP"), ModernMsgBoxType::Success, ModernMsgBoxButtons::OK, pParent);
}

INT_PTR CModernMessageBox::Question(const CString& strMessage, CWnd* pParent)
{
    return Show(strMessage, _T("KFTCGiroCAP"), ModernMsgBoxType::Question, ModernMsgBoxButtons::YesNo, pParent);
}
