// ModernUI.cpp - 완전 통합 버전
// CModernButton + CModernCheckBox + CPortToggleButton

#include "stdafx.h"
#include "ModernUI.h"
#include <string>
#include <vector>

// --- small safe helpers (avoid min/max macro conflicts) ---
static __inline int kftc_min_i(int a, int b) { return (a < b) ? a : b; }

// ── 부모 배경 헬퍼 ─────────────────────────────────────────────────
// 둥근 컨트롤(버튼·토글·콤보·에디트)을 그리기 전 사각 영역 전체를
// 부모 배경으로 먼저 채워 모서리가 자연스럽게 어울리게 한다.
static COLORREF kftc_parent_bg_color(HWND hWnd, HDC hdc)
{
    HWND hP = ::GetParent(hWnd);
    if (!hP) return ::GetSysColor(COLOR_WINDOW);
    HBRUSH hbr = (HBRUSH)::SendMessage(hP, WM_CTLCOLORSTATIC,
                                        (WPARAM)hdc, (LPARAM)hWnd);
    if (!hbr) return ::GetSysColor(COLOR_WINDOW);
    LOGBRUSH lb = {};
    if (::GetObject(hbr, sizeof(lb), &lb) && lb.lbStyle == BS_SOLID)
        return lb.lbColor;
    return ::GetSysColor(COLOR_WINDOW);
}
static void kftc_fill_parent_bg(HWND hWnd, HDC hdc, const RECT& rc)
{
    HWND hP = ::GetParent(hWnd);
    if (!hP) { ::FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1)); return; }
    HBRUSH hbr = (HBRUSH)::SendMessage(hP, WM_CTLCOLORSTATIC,
                                        (WPARAM)hdc, (LPARAM)hWnd);
    if (hbr) ::FillRect(hdc, &rc, hbr);
    else     ::FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
}
static __inline int kftc_max_i(int a, int b) { return (a > b) ? a : b; }

static std::wstring kftc_to_wide(const CString& s)
{
#ifdef UNICODE
	return std::wstring(s.GetString());
#else
	const char* src = s.GetString();
	int need = ::MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
	if (need <= 0) return std::wstring();
	std::wstring ws;
	ws.resize((size_t)need);
	::MultiByteToWideChar(CP_ACP, 0, src, -1, &ws[0], need);
	if (!ws.empty() && ws.back() == L'\0') ws.pop_back();
	return ws;
#endif
}


//////////////////////////////////////////////////////////////////////////
// Global theme storage (optional)
//////////////////////////////////////////////////////////////////////////
namespace ModernUITheme
{
	static KFTCInputTheme g_inputTheme; // default ctor -> macros
	const KFTCInputTheme& GetInputTheme() { return g_inputTheme; }
	void SetInputTheme(const KFTCInputTheme& theme) { g_inputTheme = theme; }
}

//////////////////////////////////////////////////////////////////////////
// GDI+ lifetime helper (single point)
//////////////////////////////////////////////////////////////////////////
namespace ModernUIGfx
{
	static ULONG_PTR s_gdiplusToken = 0;
	static bool s_started = false;

	void EnsureGdiplusStartup()
	{
		if (s_started) return;
		Gdiplus::GdiplusStartupInput si;
		if (Gdiplus::GdiplusStartup(&s_gdiplusToken, &si, NULL) == Gdiplus::Ok)
			s_started = true;
	}

	void ShutdownGdiplus()
	{
		if (!s_started) return;
		Gdiplus::GdiplusShutdown(s_gdiplusToken);
		s_gdiplusToken = 0;
		s_started = false;
	}
}

//////////////////////////////////////////////////////////////////////////
// DPI scaling helpers (per-monitor aware)
//////////////////////////////////////////////////////////////////////////
namespace ModernUIDpi
{
	typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);

	static UINT GetDpiForHwndImpl(HWND hwnd)
	{
		// Prefer per-window DPI on Win10+; fall back safely.
		HMODULE hUser32 = ::GetModuleHandle(_T("user32.dll"));
		if (hUser32)
		{
#ifdef UNICODE
			PFN_GetDpiForWindow pGetDpiForWindow = (PFN_GetDpiForWindow)::GetProcAddress(hUser32, "GetDpiForWindow");
#else
			PFN_GetDpiForWindow pGetDpiForWindow = (PFN_GetDpiForWindow)::GetProcAddress(hUser32, "GetDpiForWindow");
#endif
			if (pGetDpiForWindow && hwnd)
				return pGetDpiForWindow(hwnd);
		}

		// System DPI fallback
		HDC hdc = ::GetDC(hwnd);
		UINT dpi = 96;
		if (hdc)
		{
			dpi = (UINT)::GetDeviceCaps(hdc, LOGPIXELSX);
			::ReleaseDC(hwnd, hdc);
		}
		if (dpi == 0) dpi = 96;
		return dpi;
	}

	UINT GetDpiForHwnd(HWND hwnd)
	{
		return GetDpiForHwndImpl(hwnd);
	}

	int Scale(HWND hwnd, int px)
	{
		const UINT dpi = GetDpiForHwndImpl(hwnd);
		return ::MulDiv(px, (int)dpi, 96);
	}

	float ScaleF(HWND hwnd, float px)
	{
		const UINT dpi = GetDpiForHwndImpl(hwnd);
		return px * ((float)dpi / 96.0f);
	}
}


// ========================================
// CModernButton 구현
// ========================================

BEGIN_MESSAGE_MAP(CModernButton, CButton)
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
END_MESSAGE_MAP()

CModernButton::CModernButton()
{
	m_clrNormalBg = RGB(255, 255, 255);
	m_clrHoverBg = RGB(243, 248, 255);
	m_clrText = KFTC_TEXT_DARK;
	m_bHover = FALSE;
	m_bTracking = FALSE;
	m_bPressed = FALSE;
	m_bUseUnderlayBg = FALSE;
	m_clrUnderlayBg = RGB(255, 255, 255);
	m_clrBrushBg = (COLORREF)-1; // force create on first CtlColor
}

CModernButton::~CModernButton()
{
}

void CModernButton::SetColors(COLORREF normalBg, COLORREF hoverBg, COLORREF textColor)
{
	m_clrNormalBg = normalBg;
	m_clrHoverBg = hoverBg;
	m_clrText = textColor;
}

void CModernButton::SetUnderlayColor(COLORREF underlayBg)
{
	m_bUseUnderlayBg = TRUE;
	m_clrUnderlayBg = underlayBg;
	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_NOERASE);
}

void CModernButton::ClearUnderlayColor()
{
	m_bUseUnderlayBg = FALSE;
	Invalidate(FALSE);
}


void CModernButton::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bTracking)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE | TME_HOVER;
		tme.hwndTrack = m_hWnd;
		tme.dwHoverTime = 1;

		if (TrackMouseEvent(&tme))
		{
			m_bTracking = TRUE;
		}
	}

	CButton::OnMouseMove(nFlags, point);
}

void CModernButton::OnMouseLeave()
{
	m_bHover = FALSE;
	m_bTracking = FALSE;
	Invalidate();
}

LRESULT CModernButton::OnMouseHover(WPARAM wParam, LPARAM lParam)
{
	m_bHover = TRUE;
	Invalidate();
	return 0;
}

void CModernButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_bPressed = TRUE;
	Invalidate();
	CButton::OnLButtonDown(nFlags, point);
}

void CModernButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bPressed = FALSE;
	Invalidate();
	CButton::OnLButtonUp(nFlags, point);
}

void CModernButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rect = lpDrawItemStruct->rcItem;

	// 둥근 버튼 모서리: 전체 사각을 부모(or underlay) 배경으로 먼저 채움
	{
		COLORREF bgC = m_bUseUnderlayBg
			? m_clrUnderlayBg
			: kftc_parent_bg_color(m_hWnd, pDC->m_hDC);
		pDC->FillSolidRect(&rect, bgC);
	}

	ModernUIGfx::EnsureGdiplusStartup();
	Gdiplus::Graphics g(pDC->m_hDC);
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

	CString strText;
	GetWindowText(strText);

	// 버튼 유형 판별
	BOOL isOK = (strText.Find(_T("저장")) >= 0 || strText.Find(_T("확인")) >= 0 );
	BOOL isExit = (strText.Find(_T("종료")) >= 0);
	BOOL isMini = (strText.Find(_T("최소화")) >= 0);
	BOOL isDownload = (strText.Find(_T("다운로드")) >= 0);
	BOOL isPrimary = isOK;  // 파란 버튼 (확인/저장/다운로드)

	// 기본 배경
	const float rad = 8.0f;
	Gdiplus::RectF rf(
		(float)rect.left + 1.5f, (float)rect.top + 1.5f,
		(float)rect.Width() - 3.0f, (float)rect.Height() - 3.0f);

	// 그림자
	{
		Gdiplus::RectF sh(rf.X, rf.Y + 1.5f, rf.Width, rf.Height);
		Gdiplus::GraphicsPath sp; AddRoundRect(sp, sh, rad);
		Gdiplus::SolidBrush sb(Gdiplus::Color(12, 0, 0, 0));
		g.FillPath(&sb, &sp);
	}

	Gdiplus::GraphicsPath bp; AddRoundRect(bp, rf, rad);

	if (isPrimary)
	{
		// 파란 그라디언트
		Gdiplus::Color c1, c2;
		if (m_bPressed) { c1 = Gdiplus::Color(255, 0, 80, 190); c2 = Gdiplus::Color(255, 0, 60, 170); }
		else if (m_bHover) { c1 = Gdiplus::Color(255, 30, 120, 255); c2 = Gdiplus::Color(255, 0, 100, 235); }
		else { c1 = Gdiplus::Color(255, 20, 108, 240); c2 = Gdiplus::Color(255, 0, 88, 220); }
		Gdiplus::LinearGradientBrush gb(
			Gdiplus::PointF(rf.X, rf.Y),
			Gdiplus::PointF(rf.X, rf.Y + rf.Height), c1, c2);
		g.FillPath(&gb, &bp);
		// 테두리
		Gdiplus::Pen pen(Gdiplus::Color(255, 0, 78, 200), 1.0f);
		g.DrawPath(&pen, &bp);
	}
	else if (isExit)
	{
		// 빨간 버튼
		Gdiplus::Color c1, c2;
		if (m_bPressed) { c1 = Gdiplus::Color(255, 200, 40, 40); c2 = Gdiplus::Color(255, 180, 20, 20); }
		else if (m_bHover) { c1 = Gdiplus::Color(255, 240, 80, 80); c2 = Gdiplus::Color(255, 220, 50, 50); }
		else { c1 = Gdiplus::Color(255, 235, 65, 65); c2 = Gdiplus::Color(255, 210, 40, 40); }
		Gdiplus::LinearGradientBrush gb(
			Gdiplus::PointF(rf.X, rf.Y),
			Gdiplus::PointF(rf.X, rf.Y + rf.Height), c1, c2);
		g.FillPath(&gb, &bp);
		Gdiplus::Pen pen(Gdiplus::Color(255, 190, 30, 30), 1.0f);
		g.DrawPath(&pen, &bp);
	}
	else if (isDownload)
	{
		// 다운로드: 연한 파랑 배경 + 파란 텍스트 (토스 secondary 스타일)
		// 기본: 연한 파랑 bg + 파랑 테두리
		// 호버: 좀 더 진한 파랑 bg
		// 클릭: 더 진한
		Gdiplus::Color bg;
		if (m_bPressed)      bg = Gdiplus::Color(255, 210, 230, 255);
		else if (m_bHover)   bg = Gdiplus::Color(255, 225, 239, 255);
		else                 bg = Gdiplus::Color(255, 236, 246, 255);  // EEF6FF

		Gdiplus::SolidBrush br(bg);
		g.FillPath(&br, &bp);

		// 테두리: 배경보다 살짝 진한 파랑 (너무 강조 안되게)
		Gdiplus::Color bc = m_bPressed
			? Gdiplus::Color(255,   0,  86, 200)
			: Gdiplus::Color(255,  80, 150, 240);
		Gdiplus::Pen pen(bc, 1.f);
		pen.SetLineJoin(Gdiplus::LineJoinRound);
		g.DrawPath(&pen, &bp);
	}
	else
	{
		// 기본 (취소 등): 흰 배경 + 연회색 테두리
		Gdiplus::Color bg;
		if (m_bPressed)      bg = Gdiplus::Color(255, 235, 238, 245);
		else if (m_bHover)   bg = Gdiplus::Color(255, 244, 246, 252);
		else                  bg = Gdiplus::Color(255, 255, 255, 255);
		Gdiplus::SolidBrush br(bg);
		g.FillPath(&br, &bp);
		Gdiplus::Pen pen(Gdiplus::Color(255, 210, 216, 228), 1.2f);
		g.DrawPath(&pen, &bp);
	}

	// 텍스트
	Gdiplus::Color txtColor = (isPrimary || isExit || isMini)
		? Gdiplus::Color(255, 255, 255, 255)
		: (isDownload
			? (m_bPressed ? Gdiplus::Color(255, 0, 76, 180) : Gdiplus::Color(255, 0, 96, 210))
			: (m_bPressed ? Gdiplus::Color(255, 40, 50, 70) : Gdiplus::Color(255, 60, 72, 96)));

	Gdiplus::SolidBrush tb(txtColor);
	Gdiplus::FontFamily ff(L"Malgun Gothic");
	Gdiplus::Font font(&ff, (Gdiplus::REAL)ModernUIDpi::ScaleF(m_hWnd, 12.5f), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	Gdiplus::StringFormat sf;
	sf.SetAlignment(Gdiplus::StringAlignmentCenter);
	sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);

	std::wstring wt = kftc_to_wide(strText);
	g.DrawString(wt.c_str(), -1, &font, rf, &sf, &tb);
}

void CModernButton::AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, REAL radius)
{
	path.AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
	path.AddArc(rect.X + rect.Width - radius * 2, rect.Y, radius * 2, radius * 2, 270, 90);
	path.AddArc(rect.X + rect.Width - radius * 2, rect.Y + rect.Height - radius * 2, radius * 2, radius * 2, 0, 90);
	path.AddArc(rect.X, rect.Y + rect.Height - radius * 2, radius * 2, radius * 2, 90, 90);
	path.CloseFigure();
}

// ========================================
// CModernCheckBox 구현
// ========================================

BEGIN_MESSAGE_MAP(CModernCheckBox, CButton)
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
END_MESSAGE_MAP()

CModernCheckBox::CModernCheckBox()
{
	m_bChecked = FALSE;
	m_bHover = FALSE;
	m_bTracking = FALSE;
	m_bPressed = FALSE;
	m_clrCardBg = RGB(255, 255, 255);
	m_bUseUnderlayBg = FALSE;
	m_clrUnderlayBg = m_clrCardBg;

	m_nTextPx = 13; // 기존과 동일
	m_bNoWrapEllipsis = FALSE;

	m_bUseGdiText = FALSE;      // 기본: 기존(GDI+) 유지
	m_nGdiTextYOffset = 0;      // GDI 텍스트 기준선 보정
}


CModernCheckBox::~CModernCheckBox()
{
}

void CModernCheckBox::SetUnderlayColor(COLORREF underlayBg)
{
	m_bUseUnderlayBg = TRUE;
	m_clrUnderlayBg = underlayBg;
	Invalidate(FALSE);
}

void CModernCheckBox::ClearUnderlayColor()
{
	m_bUseUnderlayBg = FALSE;
	Invalidate(FALSE);
}


void CModernCheckBox::SetChecked(BOOL bChecked)
{
	m_bChecked = bChecked;
	Invalidate();
}

void CModernCheckBox::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bTracking)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE | TME_HOVER;
		tme.hwndTrack = m_hWnd;
		tme.dwHoverTime = 1;

		if (TrackMouseEvent(&tme))
		{
			m_bTracking = TRUE;
		}
	}

	CButton::OnMouseMove(nFlags, point);
}

void CModernCheckBox::OnMouseLeave()
{
	m_bHover = FALSE;
	m_bTracking = FALSE;
	Invalidate();
}

LRESULT CModernCheckBox::OnMouseHover(WPARAM wParam, LPARAM lParam)
{
	m_bHover = TRUE;
	Invalidate();
	return 0;
}

void CModernCheckBox::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_bPressed = TRUE;
	Invalidate();
	CButton::OnLButtonDown(nFlags, point);
}

void CModernCheckBox::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bPressed = FALSE;
	m_bChecked = !m_bChecked;
	Invalidate();

	// 부모에게 알림
	GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);

	CButton::OnLButtonUp(nFlags, point);
}

void CModernCheckBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	ModernUIGfx::EnsureGdiplusStartup();

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rect = lpDrawItemStruct->rcItem;

	Gdiplus::Graphics graphics(pDC->m_hDC);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	// 배경색: underlay 없으면 부모 배경 (모서리 투명 효과)
	COLORREF crUnder = m_bUseUnderlayBg
		? m_clrUnderlayBg
		: kftc_parent_bg_color(m_hWnd, pDC->m_hDC);
	Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255,
		GetRValue(crUnder), GetGValue(crUnder), GetBValue(crUnder)));
	graphics.FillRectangle(&bgBrush, Gdiplus::Rect(0, 0, rect.Width(), rect.Height()));

	// 체크박스 크기 및 위치
	REAL boxSize = ModernUIDpi::ScaleF(m_hWnd, 18.0f);
	REAL boxX = 0;
	REAL boxY = (rect.Height() - boxSize) / 2.0f;

	Gdiplus::RectF boxRect(boxX, boxY, boxSize, boxSize);

	// 체크박스 배경
	Gdiplus::GraphicsPath boxPath;
	AddRoundRect(boxPath, boxRect, ModernUIDpi::ScaleF(m_hWnd, 4.0f));

	if (m_bChecked)
	{
		// 체크된 상태
		Gdiplus::LinearGradientBrush checkBrush(
			Gdiplus::PointF(boxRect.X, boxRect.Y),
			Gdiplus::PointF(boxRect.X, boxRect.Y + boxRect.Height),
			Gdiplus::Color(255, 0, 100, 221),
			Gdiplus::Color(255, 15, 124, 255)
		);
		graphics.FillPath(&checkBrush, &boxPath);

		// 체크 마크
		Gdiplus::Pen checkPen(Gdiplus::Color(255, 255, 255, 255), ModernUIDpi::ScaleF(m_hWnd, 2.0f));
		checkPen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);

		graphics.DrawLine(&checkPen,
			Gdiplus::PointF(boxX + ModernUIDpi::ScaleF(m_hWnd, 5.0f), boxY + ModernUIDpi::ScaleF(m_hWnd, 9.0f)),
			Gdiplus::PointF(boxX + ModernUIDpi::ScaleF(m_hWnd, 8.0f), boxY + ModernUIDpi::ScaleF(m_hWnd, 12.0f)));
		graphics.DrawLine(&checkPen,
			Gdiplus::PointF(boxX + ModernUIDpi::ScaleF(m_hWnd, 8.0f), boxY + ModernUIDpi::ScaleF(m_hWnd, 12.0f)),
			Gdiplus::PointF(boxX + ModernUIDpi::ScaleF(m_hWnd, 13.0f), boxY + ModernUIDpi::ScaleF(m_hWnd, 6.0f)));
	}
	else
	{
		// 체크 안된 상태
		Gdiplus::SolidBrush uncheckBrush(Gdiplus::Color(255, 255, 255, 255));
		graphics.FillPath(&uncheckBrush, &boxPath);
	}

	// 테두리
	Gdiplus::Color borderColor;
	borderColor = m_bHover ? Gdiplus::Color(255, 15, 124, 255) : Gdiplus::Color(255, 168, 208, 255);

	Gdiplus::Pen borderPen(borderColor, m_bHover ? ModernUIDpi::ScaleF(m_hWnd, 1.6f) : ModernUIDpi::ScaleF(m_hWnd, 1.2f));
	graphics.DrawPath(&borderPen, &boxPath);

	// 텍스트
		// Text
	CString strText;
	GetWindowText(strText);

	if (!strText.IsEmpty())
	{
		// If enabled, draw text with GDI (DrawText) to match standard Windows controls.
		if (m_bUseGdiText)
		{
			CRect rcText = rect;
			rcText.left = (int)(boxSize + ModernUIDpi::ScaleF(m_hWnd, 8.0f));
			rcText.OffsetRect(0, m_nGdiTextYOffset);

			UINT dt = DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX;
			if (m_bNoWrapEllipsis)
				dt |= DT_END_ELLIPSIS;



			COLORREF clrText = ((lpDrawItemStruct->itemState & ODS_DISABLED) != 0)
				? ::GetSysColor(COLOR_GRAYTEXT)
				: RGB(90, 90, 90);
			int oldBk = pDC->SetBkMode(TRANSPARENT);
			COLORREF oldClr = pDC->SetTextColor(clrText);



			// Use the same font as standard controls (combo/edit): prefer this control's font,
			// fall back to the parent/dialog font, then DEFAULT_GUI_FONT.
			CFont* pOldFont = NULL;
			CFont* pFont = GetFont();
			if (pFont == NULL)
			{
				CWnd* pParent = GetParent();
				if (pParent) pFont = pParent->GetFont();
			}
			if (pFont == NULL)
				pFont = CFont::FromHandle((HFONT)::GetStockObject(DEFAULT_GUI_FONT));

			if (pFont != NULL)
				pOldFont = pDC->SelectObject(pFont);

			pDC->DrawText(strText, &rcText, dt);

			if (pOldFont != NULL) pDC->SelectObject(pOldFont);
			pDC->SetTextColor(oldClr);
			pDC->SetBkMode(oldBk);
		}
		else
		{
			// Original GDI+ text rendering
			Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 6, 52, 109));
			Gdiplus::FontFamily fontFamily(L"Malgun Gothic");
			Gdiplus::Font font(&fontFamily, (Gdiplus::REAL)ModernUIDpi::ScaleF(m_hWnd, (float)m_nTextPx), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

			Gdiplus::StringFormat format;
			format.SetAlignment(Gdiplus::StringAlignmentNear);
			format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

			if (m_bNoWrapEllipsis)
			{
				format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
				format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
			}

			Gdiplus::RectF textRect(boxX + boxSize + ModernUIDpi::ScaleF(m_hWnd, 8.0f), 0, rect.Width() - boxSize - ModernUIDpi::ScaleF(m_hWnd, 8.0f), (REAL)rect.Height());

			std::wstring wText = kftc_to_wide(strText);
			graphics.DrawString(wText.c_str(), -1, &font, textRect, &format, &textBrush);
		}
	}
}

void CModernCheckBox::AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, REAL radius)
{
	path.AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
	path.AddArc(rect.X + rect.Width - radius * 2, rect.Y, radius * 2, radius * 2, 270, 90);
	path.AddArc(rect.X + rect.Width - radius * 2, rect.Y + rect.Height - radius * 2, radius * 2, radius * 2, 0, 90);
	path.AddArc(rect.X, rect.Y + rect.Height - radius * 2, radius * 2, radius * 2, 90, 90);
	path.CloseFigure();
}

// ========================================
// CPortToggleButton 구현
// ========================================

BEGIN_MESSAGE_MAP(CPortToggleButton, CButton)
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONUP()
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
END_MESSAGE_MAP()

CPortToggleButton::CPortToggleButton()
{
	m_bToggled = FALSE;
	m_bHover = FALSE;
	m_bTracking = FALSE;
}

CPortToggleButton::~CPortToggleButton()
{
}

void CPortToggleButton::SetToggled(BOOL bToggled)
{
	m_bToggled = bToggled;
	Invalidate();
}

void CPortToggleButton::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bTracking)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE | TME_HOVER;
		tme.hwndTrack = m_hWnd;
		tme.dwHoverTime = 1;

		if (TrackMouseEvent(&tme))
		{
			m_bTracking = TRUE;
		}
	}

	CButton::OnMouseMove(nFlags, point);
}

void CPortToggleButton::OnMouseLeave()
{
	m_bHover = FALSE;
	m_bTracking = FALSE;
	Invalidate();
}

LRESULT CPortToggleButton::OnMouseHover(WPARAM wParam, LPARAM lParam)
{
	m_bHover = TRUE;
	Invalidate();
	return 0;
}

void CPortToggleButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bToggled = !m_bToggled;
	Invalidate();

	// 부모에게 알림
	GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);

	CButton::OnLButtonUp(nFlags, point);
}

// ========================================
// CModernToggleSwitch (settings-style switch)
// ========================================
BEGIN_MESSAGE_MAP(CModernToggleSwitch, CButton)
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONUP()
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
END_MESSAGE_MAP()

CModernToggleSwitch::CModernToggleSwitch()
{
	m_bToggled = FALSE;
	m_bHover = FALSE;
	m_bTracking = FALSE;
	m_nTextPx = 12;
	m_bNoWrapEllipsis = TRUE;
	m_bUseUnderlay = FALSE;
	m_clrUnderlay = RGB(255, 255, 255);
}

CModernToggleSwitch::~CModernToggleSwitch()
{
}

void CModernToggleSwitch::SetToggled(BOOL bToggled)
{
	m_bToggled = bToggled ? TRUE : FALSE;
	Invalidate(FALSE);
}

void CModernToggleSwitch::AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, REAL radius)
{
	const REAL d = radius * 2.0f;
	path.AddArc(rect.X, rect.Y, d, d, 180, 90);
	path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270, 90);
	path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0, 90);
	path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90, 90);
	path.CloseFigure();
}

void CModernToggleSwitch::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rcItem = lpDrawItemStruct->rcItem;

	const int w = rcItem.Width();
	const int h = rcItem.Height();

	// Double buffering (toggle click/focus flicker 제거)
	CDC memDC;
	memDC.CreateCompatibleDC(pDC);

	CBitmap memBmp;
	memBmp.CreateCompatibleBitmap(pDC, w, h);
	CBitmap* pOldBmp = memDC.SelectObject(&memBmp);

	// 배경: underlay 우선, 없으면 부모 배경 (모서리 투명 효과)
	COLORREF bg = m_bUseUnderlay
		? m_clrUnderlay
		: kftc_parent_bg_color(m_hWnd, memDC.GetSafeHdc());
	memDC.FillSolidRect(0, 0, w, h, bg);

	Gdiplus::Graphics graphics(memDC.GetSafeHdc());
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	// 로컬 좌표계 (0,0)
	CRect rect(0, 0, w, h);

	// 스위치 크기/위치: 텍스트 왼쪽, 스위치 오른쪽 (TOSS/Kakao 스타일)
	REAL switchW = ModernUIDpi::ScaleF(m_hWnd, 44.0f);
	REAL switchH = ModernUIDpi::ScaleF(m_hWnd, 24.0f);
	REAL marginR = ModernUIDpi::ScaleF(m_hWnd, 2.0f);
	REAL switchX = (REAL)rect.Width() - switchW - marginR;
	REAL switchY = (rect.Height() - switchH) / 2.0f;

	// keep stroke inside (avoid right-edge clipping)
	Gdiplus::RectF switchRect(switchX + ModernUIDpi::ScaleF(m_hWnd, 0.5f), switchY + ModernUIDpi::ScaleF(m_hWnd, 0.5f), switchW - ModernUIDpi::ScaleF(m_hWnd, 1.0f), switchH - ModernUIDpi::ScaleF(m_hWnd, 1.0f));

	// 트랙(배경)
	Gdiplus::GraphicsPath switchPath;
	AddRoundRect(switchPath, switchRect, switchH / 2);

	if (m_bToggled)
	{
		// ON: 기본 파랑 / 호버 시 더 밝고 선명한 파랑
		Gdiplus::Color c1, c2;
		if (m_bHover)
		{
			c1 = Gdiplus::Color(255, 20, 118, 245);  // 호버: 더 밝은 파랑
			c2 = Gdiplus::Color(255, 10, 140, 255);
		}
		else
		{
			c1 = Gdiplus::Color(255, 0, 100, 221);
			c2 = Gdiplus::Color(255, 15, 124, 255);
		}
		Gdiplus::LinearGradientBrush trackBrush(
			Gdiplus::PointF(switchRect.X, switchRect.Y),
			Gdiplus::PointF(switchRect.X, switchRect.Y + switchRect.Height),
			c1, c2);
		graphics.FillPath(&trackBrush, &switchPath);
	}
	else
	{
		// OFF: 기본 연회색 / 호버 시 약간 진한 회색
		Gdiplus::Color offColor = m_bHover
			? Gdiplus::Color(255, 208, 218, 232)   // 호버: 더 진한 회색
			: Gdiplus::Color(255, 230, 236, 245);   // 기본
		Gdiplus::SolidBrush trackBrush(offColor);
		graphics.FillPath(&trackBrush, &switchPath);
	}

	// ※ 토글 주변 테두리(미세 라인) 제거: 트랙 외곽선은 그리지 않음

	// 노브(동그라미): 호버 시 1.5px 확대
	REAL knobSize = ModernUIDpi::ScaleF(m_hWnd, m_bHover ? 19.5f : 18.0f);
	REAL knobPadding = (switchRect.Height - knobSize) / 2.0f;
	REAL knobX = m_bToggled
		? (switchRect.X + switchRect.Width - knobSize - knobPadding)
		: (switchRect.X + knobPadding);
	REAL knobY = switchRect.Y + knobPadding;

	Gdiplus::RectF knobRect(knobX, knobY, knobSize, knobSize);

	// 그림자 (호버 시 약간 더 진하게)
	BYTE shadowAlpha = m_bHover ? 40 : 28;
	Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(shadowAlpha, 0, 0, 0));
	Gdiplus::RectF shadowRect(knobX + ModernUIDpi::ScaleF(m_hWnd, 0.5f),
		knobY + ModernUIDpi::ScaleF(m_hWnd, 0.8f),
		knobSize, knobSize);
	graphics.FillEllipse(&shadowBrush, shadowRect);

	// 노브 본체 (흰색, 호버 시 살짝 아이보리)
	Gdiplus::Color knobColor = m_bHover
		? Gdiplus::Color(255, 252, 252, 255)   // 호버: 매우 연한 파랑빛 흰색
		: Gdiplus::Color(255, 255, 255, 255);
	Gdiplus::SolidBrush knobBrush(knobColor);
	graphics.FillEllipse(&knobBrush, knobRect);

	// 텍스트
	CString strText;
	GetWindowText(strText);

	if (!strText.IsEmpty())
	{
		// ── 라벨과 완전히 동일한 방식: GDI DrawText ──────────────────
		// GDI+ 대신 GDI를 사용해야 Windows 레이블 텍스트와
		// 폰트 힌팅·색상·기준선이 픽셀 단위로 일치합니다.
		HFONT hFont = (HFONT)::SendMessage(m_hWnd, WM_GETFONT, 0, 0);
		if (!hFont) hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

		HFONT hOld = (HFONT)::SelectObject(memDC.GetSafeHdc(), hFont);
		// 라벨 색 RGB(100, 112, 132) 그대로
		::SetTextColor(memDC.GetSafeHdc(), RGB(100, 112, 132));
		::SetBkMode(memDC.GetSafeHdc(), TRANSPARENT);

		CRect rcText(0, 0, (int)switchX - 6, rect.Height());
		::DrawText(memDC.GetSafeHdc(), strText, -1, &rcText,
			DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

		::SelectObject(memDC.GetSafeHdc(), hOld);
	}

	// Present
	pDC->BitBlt(rcItem.left, rcItem.top, w, h, &memDC, 0, 0, SRCCOPY);

	memDC.SelectObject(pOldBmp);
}

void CModernToggleSwitch::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bTracking)
	{
		TRACKMOUSEEVENT tme;
		::ZeroMemory(&tme, sizeof(tme));
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE | TME_HOVER;
		tme.hwndTrack = m_hWnd;
		tme.dwHoverTime = 1;
		m_bTracking = ::TrackMouseEvent(&tme);
	}

	if (!m_bHover)
	{
		m_bHover = TRUE;
		Invalidate(FALSE);
	}

	CButton::OnMouseMove(nFlags, point);
}

void CModernToggleSwitch::OnMouseLeave()
{
	m_bTracking = FALSE;
	if (m_bHover)
	{
		m_bHover = FALSE;
		Invalidate(FALSE);
	}
	CButton::OnMouseLeave();
}

LRESULT CModernToggleSwitch::OnMouseHover(WPARAM wParam, LPARAM lParam)
{
	// nothing special, keep hover state
	return 0;
}

void CModernToggleSwitch::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bToggled = !m_bToggled;
	Invalidate(FALSE);

	// 부모에게 BN_CLICKED 알림
	GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);

	CButton::OnLButtonUp(nFlags, point);
}


void CPortToggleButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rect = lpDrawItemStruct->rcItem;

	// 토글 트랙 밖 모서리 영역: 부모 배경으로 채움
	pDC->FillSolidRect(&rect, kftc_parent_bg_color(m_hWnd, pDC->m_hDC));

	Gdiplus::Graphics graphics(pDC->m_hDC);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	// 토글 스위치 크기
	REAL switchW = ModernUIDpi::ScaleF(m_hWnd, 44.0f);
	REAL switchH = ModernUIDpi::ScaleF(m_hWnd, 24.0f);
	REAL switchX = 0;
	REAL switchY = (rect.Height() - switchH) / 2.0f;

	Gdiplus::RectF switchRect(switchX, switchY, switchW, switchH);

	// 배경
	Gdiplus::GraphicsPath switchPath;
	AddRoundRect(switchPath, switchRect, switchH / 2);

	if (m_bToggled)
	{
		// ON 상태 (파란색)
		Gdiplus::LinearGradientBrush bgBrush(
			Gdiplus::PointF(switchRect.X, switchRect.Y),
			Gdiplus::PointF(switchRect.X, switchRect.Y + switchRect.Height),
			Gdiplus::Color(255, 0, 100, 221),
			Gdiplus::Color(255, 15, 124, 255)
		);
		graphics.FillPath(&bgBrush, &switchPath);
	}
	else
	{
		// OFF 상태 (회색)
		Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, 184, 208, 238));
		graphics.FillPath(&bgBrush, &switchPath);
	}

	// 테두리
	Gdiplus::Color borderColor;
	if (m_bHover)
		borderColor = m_bToggled ? Gdiplus::Color(255, 0, 163, 224) : Gdiplus::Color(255, 160, 160, 160);
	else
		borderColor = m_bToggled ? Gdiplus::Color(255, 0, 118, 190) : Gdiplus::Color(100, 0, 0, 0);

	Gdiplus::Pen borderPen(borderColor, 1.5f);
	graphics.DrawPath(&borderPen, &switchPath);

	// 동그란 버튼
	REAL knobSize = ModernUIDpi::ScaleF(m_hWnd, 18.0f);
	REAL knobPadding = ModernUIDpi::ScaleF(m_hWnd, 3.0f);
	REAL knobX = m_bToggled ? (switchRect.X + switchRect.Width - knobSize - knobPadding) : (switchRect.X + knobPadding);
	REAL knobY = switchRect.Y + (switchRect.Height - knobSize) / 2;

	Gdiplus::RectF knobRect(knobX, knobY, knobSize, knobSize);

	// 그림자
	Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(30, 0, 0, 0));
	Gdiplus::RectF shadowRect(knobX + ModernUIDpi::ScaleF(m_hWnd, 0.5f), knobY + ModernUIDpi::ScaleF(m_hWnd, 0.5f), knobSize, knobSize);
	graphics.FillEllipse(&shadowBrush, shadowRect);

	// 버튼
	Gdiplus::SolidBrush knobBrush(Gdiplus::Color(255, 255, 255, 255));
	graphics.FillEllipse(&knobBrush, knobRect);

	// 텍스트
	CString strText;
	GetWindowText(strText);

	if (!strText.IsEmpty())
	{
		Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 6, 52, 109));
		Gdiplus::FontFamily fontFamily(L"Malgun Gothic");
		Gdiplus::Font font(&fontFamily, (Gdiplus::REAL)ModernUIDpi::ScaleF(m_hWnd, (float)m_nTextPx), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

		Gdiplus::StringFormat format;
		format.SetAlignment(Gdiplus::StringAlignmentNear);
		format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

		if (m_bNoWrapEllipsis)
		{
			format.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
			format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
		}

		Gdiplus::RectF textRect(switchX + switchW + ModernUIDpi::ScaleF(m_hWnd, 10.0f), 0, rect.Width() - switchW - ModernUIDpi::ScaleF(m_hWnd, 10.0f), (REAL)rect.Height());

		std::wstring wText = kftc_to_wide(strText);
		graphics.DrawString(wText.c_str(), -1, &font, textRect, &format, &textBrush);
	}
}

void CPortToggleButton::AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, REAL radius)
{
	path.AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
	path.AddArc(rect.X + rect.Width - radius * 2, rect.Y, radius * 2, radius * 2, 270, 90);
	path.AddArc(rect.X + rect.Width - radius * 2, rect.Y + rect.Height - radius * 2, radius * 2, radius * 2, 0, 90);
	path.AddArc(rect.X, rect.Y + rect.Height - radius * 2, radius * 2, radius * 2, 90, 90);
	path.CloseFigure();
}


// ========================================
// CSkinnedComboBox implementation
// ========================================

BEGIN_MESSAGE_MAP(CSkinnedComboBox, CComboBox)
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_MESSAGE(WM_PRINTCLIENT, OnPrintClientMsg)
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnCbnDropdown)
	ON_CONTROL_REFLECT(CBN_CLOSEUP, OnCbnCloseup)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnCbnSelchange)
	ON_CONTROL_REFLECT(CBN_SELENDOK, OnCbnSelendok)
END_MESSAGE_MAP()

CSkinnedComboBox::CSkinnedComboBox()
{
	m_bUseGlobalTheme = TRUE;
	m_bHasLocalTheme = FALSE;
	m_bHover = FALSE;
	m_bTracking = FALSE;
	m_bFocus = FALSE;
	m_bDropped = FALSE;
	m_nSelOnDrop = -1;
	m_bSelCommitted = FALSE;
	m_bHoverTimer = FALSE;

	m_bInPaint = FALSE;
	m_bUseUnderlayBg = FALSE;
	m_clrUnderlayBg = RGB(255, 255, 255);
	m_clrBrushBg = (COLORREF)-1; // force create on first CtlColor
	m_hList = NULL;
	m_oldListProc = NULL;

	m_bUseGlobalTheme = TRUE;
	m_localTheme = ModernUITheme::GetInputTheme();

	m_nTextPx = 11;
	m_nDropW = 26;
	m_nRadius = ModernUITheme::GetInputTheme().radius;

	// cached font for GDI+ text (avoid per-paint CreateFontIndirect)
	m_hTextFontCache = NULL;
	::ZeroMemory(&m_lfTextCache, sizeof(m_lfTextCache));
	m_bHasTextFontCache = FALSE;
}

CSkinnedComboBox::~CSkinnedComboBox()
{
	UnhookListBox();

	if (m_hTextFontCache)
	{
		::DeleteObject(m_hTextFontCache);
		m_hTextFontCache = NULL;
	}
	m_bHasTextFontCache = FALSE;
}

BOOL CSkinnedComboBox::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE; // everything is owner-drawn
}

void CSkinnedComboBox::PreSubclassWindow()
{
	// remove classic edges/borders; we draw our own
	ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);
	ModifyStyle(WS_BORDER, 0, SWP_FRAMECHANGED);

	LONG_PTR st = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
	st &= ~WS_BORDER;
	::SetWindowLongPtr(m_hWnd, GWL_STYLE, st);

	LONG_PTR ex = ::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
	ex &= ~WS_EX_CLIENTEDGE;
	::SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, ex);

	::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

	// enforce owner-draw (works best with CBS_DROPDOWNLIST)
	ModifyStyle(0, CBS_OWNERDRAWFIXED | CBS_HASSTRINGS, SWP_FRAMECHANGED);

	// item height: MoveWindow로 지정된 높이에서 border 여백(2px) 빼서 설정
	CRect rc;
	GetClientRect(&rc);
	int selH = max(16, rc.Height() - 2);   // 거의 전체 높이 사용 (border 1px 여백)
	int listH = max(selH, m_nTextPx + 10);

	SendMessage(CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)selH);
	SendMessage(CB_SETITEMHEIGHT, (WPARAM)0, (LPARAM)listH);

	CComboBox::PreSubclassWindow();
}

void CSkinnedComboBox::TrackMouseLeave()
{
	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(tme);
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = m_hWnd;
	tme.dwHoverTime = 0;

	if (::_TrackMouseEvent(&tme))
		m_bTracking = TRUE;
	else
		m_bTracking = FALSE;
}

void CSkinnedComboBox::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bHover)
	{
		m_bHover = TRUE;
		Invalidate(FALSE);
		if (!m_bHoverTimer)
		{
			SetTimer(0x4A11, 50, NULL);
			m_bHoverTimer = TRUE;
		}
	}
	TrackMouseLeave();
	CComboBox::OnMouseMove(nFlags, point);
}

void CSkinnedComboBox::OnMouseLeave()
{
	m_bHover = FALSE;
	m_bTracking = FALSE;
	if (m_bHoverTimer)
	{
		KillTimer(0x4A11);
		m_bHoverTimer = FALSE;
	}
	Invalidate(FALSE);
	CComboBox::OnMouseLeave();
}

void CSkinnedComboBox::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0x4A11)
	{
		// Safety net: WM_MOUSELEAVE can be missed depending on capture/child popup behavior.
		// If the cursor is no longer over the combo (and dropdown is not open), clear hover.
		if (!GetDroppedState())
		{
			POINT ptScreen = {};
			::GetCursorPos(&ptScreen);
			CPoint pt(ptScreen);
			ScreenToClient(&pt);

			CRect rc;
			GetClientRect(&rc);

			if (!rc.PtInRect(pt))
			{
				m_bHover = FALSE;
				m_bTracking = FALSE;
				KillTimer(0x4A11);
				m_bHoverTimer = FALSE;
				Invalidate(FALSE);
				return;
			}
		}
		return; // handled
	}
	CComboBox::OnTimer(nIDEvent);
}

void CSkinnedComboBox::OnSetFocus(CWnd* pOldWnd)
{
	m_bFocus = TRUE;
	CComboBox::OnSetFocus(pOldWnd);
	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_NOERASE);
}

void CSkinnedComboBox::OnKillFocus(CWnd* pNewWnd)
{
	m_bFocus = FALSE;
	m_bHover = FALSE;
	m_bTracking = FALSE;
	if (m_bHoverTimer)
	{
		KillTimer(0x4A11);
		m_bHoverTimer = FALSE;
	}
	CComboBox::OnKillFocus(pNewWnd);

	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_NOERASE);
}

void CSkinnedComboBox::OnLButtonDown(UINT nFlags, CPoint point)
{
	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_NOERASE);
	CComboBox::OnLButtonDown(nFlags, point);
	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_NOERASE);
}

CString CSkinnedComboBox::GetDisplayText() const
{
	CString text;
	int sel = ((CSkinnedComboBox*)this)->GetCurSel();
	if (((CSkinnedComboBox*)this)->m_bDropped && !((CSkinnedComboBox*)this)->m_bSelCommitted && ((CSkinnedComboBox*)this)->m_nSelOnDrop >= 0)
		sel = ((CSkinnedComboBox*)this)->m_nSelOnDrop;
	if (sel >= 0)
		((CSkinnedComboBox*)this)->GetLBText(sel, text);
	else
		((CSkinnedComboBox*)this)->GetWindowText(text);
	return text;
}

const KFTCInputTheme& CSkinnedComboBox::GetActiveInputTheme() const
{
	if (!m_bUseGlobalTheme && m_bHasLocalTheme)
		return m_localTheme;
	if (m_bHasLocalTheme && !m_bUseGlobalTheme)
		return m_localTheme;
	return ModernUITheme::GetInputTheme();
}

void CSkinnedComboBox::AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, REAL radius)
{
	if (radius <= 0.1f)
	{
		path.AddRectangle(rect);
		path.CloseFigure();
		return;
	}

	REAL maxR = (rect.Width < rect.Height ? rect.Width : rect.Height) / 2.0f;
	if (radius > maxR) radius = maxR;
	path.AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
	path.AddArc(rect.X + rect.Width - radius * 2, rect.Y, radius * 2, radius * 2, 270, 90);
	path.AddArc(rect.X + rect.Width - radius * 2, rect.Y + rect.Height - radius * 2, radius * 2, radius * 2, 0, 90);
	path.AddArc(rect.X, rect.Y + rect.Height - radius * 2, radius * 2, radius * 2, 90, 90);
	path.CloseFigure();
}

void CSkinnedComboBox::PaintComboToDC(CDC& dc)
{
	CRect rc;
	GetClientRect(&rc);

	CDC memDC;
	memDC.CreateCompatibleDC(&dc);
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
	CBitmap* pOldBmp = memDC.SelectObject(&bmp);

	Gdiplus::Graphics g(memDC.m_hDC);
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

	// colors (modern style)
	const KFTCInputTheme& th = GetActiveInputTheme();
	const COLORREF crBorderN = th.borderN;
	const COLORREF crBorderH = th.borderH;
	const COLORREF crBorderF = th.borderF;

	// 입력 영역 내부 배경:
// - 기본은 흰색
// - UnderlayColor가 지정된 경우(카드/섹션 배경이 실시간 변경되는 케이스)에는
//   라운드 내부 채움도 UnderlayColor로 맞춰 "흰색 박스"처럼 보이는 현상을 제거한다.
Gdiplus::Color bg(255, 255, 255, 255);

    // 카드 배경과 컨트롤 배경을 일치시키기 위해 underlay가 설정된 경우 내부 fill도 underlay 색을 사용한다.
    if (m_bUseUnderlayBg)
        bg = Gdiplus::Color(255, GetRValue(m_clrUnderlayBg), GetGValue(m_clrUnderlayBg), GetBValue(m_clrUnderlayBg));
if (m_bUseUnderlayBg)
{
    bg = Gdiplus::Color(255, GetRValue(m_clrUnderlayBg), GetGValue(m_clrUnderlayBg), GetBValue(m_clrUnderlayBg));
}
	Gdiplus::Color borderN(255, GetRValue(crBorderN), GetGValue(crBorderN), GetBValue(crBorderN));
	Gdiplus::Color borderH(255, GetRValue(crBorderH), GetGValue(crBorderH), GetBValue(crBorderH));
	Gdiplus::Color borderF(255, GetRValue(crBorderF), GetGValue(crBorderF), GetBValue(crBorderF));

	Gdiplus::Color dropN(255, 255, 255, 255);
	Gdiplus::Color dropH(255, 255, 255, 255);
	Gdiplus::Color dropF(255, 255, 255, 255);

	Gdiplus::Color dividerC(255, 232, 235, 240);
	Gdiplus::Color arrowC(255, 100, 100, 100);

	if (m_bUseUnderlayBg)
	{
		HBRUSH hbr = ::CreateSolidBrush(m_clrUnderlayBg);
		::FillRect(memDC.m_hDC, &rc, hbr);
		::DeleteObject(hbr);
	}
	else
	{
		HBRUSH hbr = NULL;
		HWND hParent = ::GetParent(m_hWnd);
		if (hParent)
			hbr = (HBRUSH)::SendMessage(hParent, WM_CTLCOLORSTATIC, (WPARAM)memDC.m_hDC, (LPARAM)m_hWnd);

		if (hbr)
			::FillRect(memDC.m_hDC, &rc, hbr);
		else
			::FillRect(memDC.m_hDC, &rc, (HBRUSH)(COLOR_WINDOW + 1));
	}

	const int dropW = kftc_min_i(m_nDropW, rc.Width());
	CRect rcDrop(rc.right - dropW, rc.top, rc.right, rc.bottom);

	const int thickI = (m_bFocus || m_bDropped) ? (int)th.thickF : (int)th.thickN;
	const Gdiplus::REAL thick = (Gdiplus::REAL)thickI;

	int radI = m_nRadius;
	int maxRad = (kftc_min_i(rc.Width(), rc.Height()) - 2) / 2;
	if (maxRad < 1) maxRad = 1;
	if (radI > maxRad) radI = maxRad;
	if (radI < 1) radI = 1;

	Gdiplus::RectF rfOuter(0.5f, 0.5f, (Gdiplus::REAL)rc.Width() - 1.0f, (Gdiplus::REAL)rc.Height() - 1.0f);
	Gdiplus::RectF rfInner(rfOuter.X + thick, rfOuter.Y + thick,
		rfOuter.Width - (thick * 2.0f), rfOuter.Height - (thick * 2.0f));

	if (rfInner.Width < 2.0f) rfInner.Width = 2.0f;
	if (rfInner.Height < 2.0f) rfInner.Height = 2.0f;

	Gdiplus::Color bc = borderN;
	if (m_bFocus || m_bDropped) bc = borderF;
	else if (m_bHover) bc = borderH;

	Gdiplus::GraphicsPath pathOuter;
	Gdiplus::GraphicsPath pathInner;

	AddRoundRect(pathOuter, rfOuter, (Gdiplus::REAL)radI);

	Gdiplus::REAL innerRad = (Gdiplus::REAL)radI - thick;
	if (innerRad < 1.0f) innerRad = 1.0f;
	AddRoundRect(pathInner, rfInner, innerRad);

	{
		Gdiplus::SolidBrush brBorder(bc);
		g.FillPath(&brBorder, &pathOuter);
	}
	{
		Gdiplus::SolidBrush brBg(bg);
		g.FillPath(&brBg, &pathInner);
	}

	{
		Gdiplus::Pen edgePen(bc, 1.0f);
		edgePen.SetLineJoin(Gdiplus::LineJoinRound);
		edgePen.SetStartCap(Gdiplus::LineCapFlat);
		edgePen.SetEndCap(Gdiplus::LineCapFlat);
		g.DrawPath(&edgePen, &pathOuter);
	}

	{
		g.SetClip(Gdiplus::Rect(rcDrop.left, rcDrop.top, rcDrop.Width(), rcDrop.Height()));
		Gdiplus::Color dropColor = dropN;
		if (m_bFocus || m_bDropped) dropColor = dropF;
		else if (m_bHover) dropColor = dropH;

		Gdiplus::SolidBrush dropBrush(dropColor);
		g.FillPath(&dropBrush, &pathInner);

		Gdiplus::Pen divPen(dividerC, 1.0f);
		const Gdiplus::REAL x = (Gdiplus::REAL)rcDrop.left + 0.5f;
		g.DrawLine(&divPen, Gdiplus::PointF(x, 2.0f), Gdiplus::PointF(x, (Gdiplus::REAL)rc.Height() - 3.0f));

		g.ResetClip();
	}

	{
		Gdiplus::REAL cx = (Gdiplus::REAL)rcDrop.CenterPoint().x;
		Gdiplus::REAL cy = (Gdiplus::REAL)rcDrop.CenterPoint().y;
		Gdiplus::REAL sz = 4.0f; // V자 날개 크기

		Gdiplus::PointF pts[3] = {
			Gdiplus::PointF(cx - sz, cy - sz * 0.4f),
			Gdiplus::PointF(cx,      cy + sz * 0.6f),
			Gdiplus::PointF(cx + sz, cy - sz * 0.4f)
		};

		if (m_bDropped) {
			pts[0] = Gdiplus::PointF(cx - sz, cy + sz * 0.6f);
			pts[1] = Gdiplus::PointF(cx, cy - sz * 0.4f);
			pts[2] = Gdiplus::PointF(cx + sz, cy + sz * 0.6f);
		}

		Gdiplus::Color arrColor = m_bHover ? Gdiplus::Color(255, 100, 100, 100) : Gdiplus::Color(255, 160, 160, 160);
		if (m_bDropped || m_bFocus) arrColor = Gdiplus::Color(255, GetRValue(crBorderF), GetGValue(crBorderF), GetBValue(crBorderF));

		Gdiplus::Pen arrPen(arrColor, 2.0f); // 선 두께 2.0
		arrPen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
		arrPen.SetLineJoin(Gdiplus::LineJoinRound);

		g.DrawLines(&arrPen, pts, 3);
	}

	{
		CString text = GetDisplayText();

		const int padL = ModernUIDpi::Scale(m_hWnd, 10);
		const int padR = ModernUIDpi::Scale(m_hWnd, 6);

		CRect rcText;
		rcText.left = (int)(rfInner.X) + padL;
		rcText.top = (int)(rfInner.Y);
		rcText.right = (int)(rfInner.X + rfInner.Width) - dropW - padR;
		rcText.bottom = (int)(rfInner.Y + rfInner.Height);

		CFont* pFont = GetFont();
		LOGFONT lf;
		::ZeroMemory(&lf, sizeof(lf));
		if (pFont && pFont->GetSafeHandle() && pFont->GetLogFont(&lf))
		{
			lf.lfQuality = CLEARTYPE_QUALITY;
			lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, m_nTextPx);
		}
		else
		{
			::ZeroMemory(&lf, sizeof(lf));
			lf.lfHeight = -ModernUIDpi::Scale(m_hWnd, m_nTextPx);
			_tcscpy_s(lf.lfFaceName, _T("Malgun Gothic"));
			lf.lfQuality = CLEARTYPE_QUALITY;
		}

		if (!m_bHasTextFontCache || m_hTextFontCache == NULL || 0 != ::memcmp(&m_lfTextCache, &lf, sizeof(LOGFONT)))
		{
			if (m_hTextFontCache)
			{
				::DeleteObject(m_hTextFontCache);
				m_hTextFontCache = NULL;
			}
			m_hTextFontCache = ::CreateFontIndirect(&lf);
			m_lfTextCache = lf;
			m_bHasTextFontCache = (m_hTextFontCache != NULL);
		}

		HFONT hFont = m_hTextFontCache ? m_hTextFontCache : (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
		Gdiplus::Font font(memDC.m_hDC, hFont);

		if (!text.IsEmpty())
		{
			const COLORREF crTxt = RGB(30, 45, 70);
			Gdiplus::SolidBrush brText(Gdiplus::Color(255, GetRValue(crTxt), GetGValue(crTxt), GetBValue(crTxt)));

			Gdiplus::StringFormat fmt;
			fmt.SetAlignment(Gdiplus::StringAlignmentNear);
			fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
			fmt.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
			fmt.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);

			Gdiplus::RectF rfText((Gdiplus::REAL)rcText.left, (Gdiplus::REAL)rcText.top,
				(Gdiplus::REAL)rcText.Width(), (Gdiplus::REAL)rcText.Height());

#ifdef _UNICODE
			const WCHAR* w = text.GetString();
			g.DrawString(w, -1, &font, rfText, &fmt, &brText);
#else
			int len = text.GetLength();
			if (len > 0)
			{
				std::vector<WCHAR> wbuf((size_t)len + 1);
				::MultiByteToWideChar(CP_ACP, 0, text, -1, &wbuf[0], len + 1);
				g.DrawString(&wbuf[0], -1, &font, rfText, &fmt, &brText);
			}
#endif
		}
	}
	dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);

	memDC.SelectObject(pOldBmp);
}

void CSkinnedComboBox::OnPaint()
{
	CPaintDC dc(this);
	if (m_bInPaint)
		return;

	m_bInPaint = TRUE;
	PaintComboToDC(dc);
	m_bInPaint = FALSE;
}

LRESULT CSkinnedComboBox::OnPrintClientMsg(WPARAM wParam, LPARAM lParam)
{
	HDC hdc = (HDC)wParam;
	if (!hdc) return 0;

	if (m_bInPaint)
		return 0;

	m_bInPaint = TRUE;

	CDC dc;
	dc.Attach(hdc);
	PaintComboToDC(dc);
	dc.Detach();

	m_bInPaint = FALSE;
	return 0;
}

void CSkinnedComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	if (!lpMIS) return;
	lpMIS->itemHeight = max(26, m_nTextPx + 14);
}

LRESULT CALLBACK CSkinnedComboBox::ListBoxProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	WNDPROC oldProc = (WNDPROC)::GetProp(hWnd, _T("SKCBX_OLDPROC"));

	if (msg == WM_NCDESTROY)
	{
		if (oldProc)
			::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oldProc);
		::RemoveProp(hWnd, _T("SKCBX_OLDPROC"));
		::RemoveProp(hWnd, _T("SKCBX_PTR"));
		return oldProc ? ::CallWindowProc(oldProc, hWnd, msg, wp, lp) : 0;
	}

	// 1. 기본 리스트박스 동작을 먼저 수행하여 내부 아이템과 스크롤바가 빠짐없이 그려지도록 함
	LRESULT lRes = oldProc ? ::CallWindowProc(oldProc, hWnd, msg, wp, lp) : 0;

	// 2. 그리기 이벤트가 끝난 직후, 최상단에 우리가 원하는 테두리를 덮어씌움
	if (msg == WM_PAINT || msg == WM_NCPAINT)
	{
		HDC hdc = ::GetWindowDC(hWnd); // 클라이언트 영역을 넘어 창 전체 제어
		if (hdc)
		{
			RECT rc;
			::GetWindowRect(hWnd, &rc);
			::OffsetRect(&rc, -rc.left, -rc.top); // (0,0) 기준으로 변환

			// 리스트 사이 구분선과 동일한 연회색 지정 (모던 UI)
			COLORREF borderClr = RGB(228, 232, 240);
			HPEN hPen = ::CreatePen(PS_SOLID, 1, borderClr);
			HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);

			// 좌, 우, 하단에 1픽셀 선 그리기 (상단은 콤보박스 본체와 매끄럽게 연결되도록 생략)
			::MoveToEx(hdc, 0, 0, NULL);
			::LineTo(hdc, 0, rc.bottom - 1);
			::LineTo(hdc, rc.right - 1, rc.bottom - 1);
			::LineTo(hdc, rc.right - 1, -1);

			::SelectObject(hdc, hOldPen);
			::DeleteObject(hPen);
			::ReleaseDC(hWnd, hdc);
		}
	}

	return lRes;
}

void CSkinnedComboBox::HookListBox(HWND hList)
{
	if (!hList || !::IsWindow(hList)) return;
	if (m_hList == hList) return;

	UnhookListBox();

	m_hList = hList;

	// 1. 시스템 기본 입체 테두리 및 외곽선 스타일 완전 제거 (Win32 API 방식)
	LONG_PTR style = ::GetWindowLongPtr(m_hList, GWL_STYLE);
	style &= ~WS_BORDER;
	::SetWindowLongPtr(m_hList, GWL_STYLE, style);

	LONG_PTR exStyle = ::GetWindowLongPtr(m_hList, GWL_EXSTYLE);
	exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	::SetWindowLongPtr(m_hList, GWL_EXSTYLE, exStyle);

	// 스타일 변경 사항을 시스템에 즉시 반영
	::SetWindowPos(m_hList, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

	// 2. 카카오/토스 스타일처럼 리스트가 공중에 뜬 느낌을 주는 그림자 효과 추가
#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW 0x00020000
#endif
	::SetClassLongPtr(m_hList, GCL_STYLE, ::GetClassLongPtr(m_hList, GCL_STYLE) | CS_DROPSHADOW);

	m_oldListProc = (WNDPROC)::GetWindowLongPtr(m_hList, GWLP_WNDPROC);
	::SetProp(m_hList, _T("SKCBX_PTR"), (HANDLE)this);
	::SetProp(m_hList, _T("SKCBX_OLDPROC"), (HANDLE)m_oldListProc);
	::SetWindowLongPtr(m_hList, GWLP_WNDPROC, (LONG_PTR)&CSkinnedComboBox::ListBoxProc);
}

void CSkinnedComboBox::UnhookListBox()
{
	if (m_hList && ::IsWindow(m_hList))
	{
		WNDPROC oldProc = (WNDPROC)::GetProp(m_hList, _T("SKCBX_OLDPROC"));
		if (oldProc)
			::SetWindowLongPtr(m_hList, GWLP_WNDPROC, (LONG_PTR)oldProc);

		::RemoveProp(m_hList, _T("SKCBX_OLDPROC"));
		::RemoveProp(m_hList, _T("SKCBX_PTR"));
	}
	m_hList = NULL;
	m_oldListProc = NULL;
}

void CSkinnedComboBox::OnCbnDropdown()
{
	m_bDropped = TRUE;

	m_nSelOnDrop = GetCurSel();
	m_bSelCommitted = FALSE;

	COMBOBOXINFO cbi;
	::ZeroMemory(&cbi, sizeof(cbi));
	cbi.cbSize = sizeof(cbi);
	if (::GetComboBoxInfo(m_hWnd, &cbi))
	{
		CFont* pFont = GetFont();
		if (cbi.hwndList && pFont && pFont->GetSafeHandle())
			::SendMessage(cbi.hwndList, WM_SETFONT, (WPARAM)pFont->GetSafeHandle(), TRUE);

		if (cbi.hwndList)
		{
			int itemH = (int)::SendMessage(m_hWnd, CB_GETITEMHEIGHT, (WPARAM)-1, 0);
			if (itemH <= 0)
				itemH = max(26, m_nTextPx + 14);
			::SendMessage(cbi.hwndList, LB_SETITEMHEIGHT, 0, (LPARAM)itemH);
		}

		HookListBox(cbi.hwndList);
	}

	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_NOERASE);
}

void CSkinnedComboBox::OnCbnCloseup()
{
	m_bDropped = FALSE;
	UnhookListBox();

	if (!m_bSelCommitted)
	{
		if (m_nSelOnDrop >= 0)
			SetCurSel(m_nSelOnDrop);
	}

	POINT pt;
	::GetCursorPos(&pt);
	CRect rcWnd;
	GetWindowRect(&rcWnd);

	if (!rcWnd.PtInRect(pt))
	{
		m_bFocus = FALSE;
		m_bHover = FALSE;
		m_bTracking = FALSE;
	}

	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_NOERASE);
}

void CSkinnedComboBox::OnCbnSelchange()
{
	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
}

void CSkinnedComboBox::OnCbnSelendok()
{
	m_bSelCommitted = TRUE;
	m_nSelOnDrop = GetCurSel();
	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
}

void CSkinnedComboBox::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	if (!lpDIS) return;

	CDC dc;
	dc.Attach(lpDIS->hDC);

	const bool bDisabled = (lpDIS->itemState & ODS_DISABLED) != 0;
	const bool bSelected = (lpDIS->itemState & ODS_SELECTED) != 0;
	const bool bEditArea = (lpDIS->itemState & ODS_COMBOBOXEDIT) != 0;

	if ((int)lpDIS->itemID < 0 || bEditArea)
	{
		CClientDC clientDC(this);
		PaintComboToDC(clientDC);
		return;
	}

	CRect rc(lpDIS->rcItem);

	const COLORREF clrBg = RGB(255, 255, 255);
	const COLORREF clrText = RGB(50, 50, 50);
	const COLORREF clrDisText = RGB(160, 160, 160);

	// 모던 UI: 부드러운 배경 하이라이트
	const COLORREF clrSelFill = RGB(242, 246, 252);
	const COLORREF clrSelText = RGB(15, 124, 255);
	const COLORREF clrSep = RGB(235, 238, 242);

	dc.FillSolidRect(&rc, clrBg);

	if (bSelected)
	{
		Gdiplus::Graphics g(dc.m_hDC);
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
		g.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);

		CRect rSel = rc;
		rSel.DeflateRect(4, 2, 4, 2);
		if (rSel.Width() < 2) rSel.right = rSel.left + 2;
		if (rSel.Height() < 2) rSel.bottom = rSel.top + 2;

		Gdiplus::RectF rf((Gdiplus::REAL)rSel.left, (Gdiplus::REAL)rSel.top,
			(Gdiplus::REAL)rSel.Width(), (Gdiplus::REAL)rSel.Height());

		Gdiplus::GraphicsPath path;
		AddRoundRect(path, rf, 6.0f);

		Gdiplus::SolidBrush brFill(Gdiplus::Color(255, GetRValue(clrSelFill), GetGValue(clrSelFill), GetBValue(clrSelFill)));
		g.FillPath(&brFill, &path);
	}

	CString s;
	GetLBText((int)lpDIS->itemID, s);

	CRect rcText = rc;
	// 텍스트 좌우 여백을 넓혀 모던한 개방감 부여 (14 -> 18)
	rcText.DeflateRect(18, 0, 18, 0);
	dc.SetBkMode(TRANSPARENT);
	if (bDisabled)
		dc.SetTextColor(clrDisText);
	else
		dc.SetTextColor(bSelected ? clrSelText : clrText);

	int nCount = GetCount();
	if ((int)lpDIS->itemID >= 0 && (int)lpDIS->itemID < nCount - 1)
	{
		CRect rLine = rc;
		rLine.top = rLine.bottom - 1;
		rLine.left += 10;
		rLine.right -= 10;
		if (rLine.right > rLine.left)
			dc.FillSolidRect(&rLine, clrSep);
	}

	CFont* pOldFont = dc.SelectObject(GetFont());
	dc.DrawText(s, &rcText, DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

	dc.SelectObject(pOldFont);
	dc.Detach();
}

// ============================================================
// CSkinnedEdit (Edit - ComboBox와 동일한 라운드/테두리 스타일)
// ============================================================

BEGIN_MESSAGE_MAP(CSkinnedEdit, CEdit)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_CHAR()
	ON_MESSAGE(WM_SETFONT, OnSetFontMsg)
	ON_MESSAGE(WM_SETTEXT, OnSetTextMsg)
	ON_MESSAGE(WM_PRINTCLIENT, OnPrintClientMsg)
    ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()

CSkinnedEdit::CSkinnedEdit()
{
	m_bUseGlobalTheme = TRUE;
	m_bHasLocalTheme = FALSE;
	m_bHover = FALSE;
	m_bTracking = FALSE;
	m_bFocus = FALSE;
	m_bInPaint = FALSE;
	m_bUseUnderlayBg = FALSE;
	m_clrUnderlayBg = RGB(255, 255, 255);
	m_clrBrushBg = (COLORREF)-1; // force create on first CtlColor

	m_bUseGlobalTheme = TRUE;
	m_localTheme = ModernUITheme::GetInputTheme();
	m_nRadius = ModernUITheme::GetInputTheme().radius;
	m_nTextPx = 11;
}

CSkinnedEdit::~CSkinnedEdit()
{
}

const KFTCInputTheme& CSkinnedEdit::GetActiveInputTheme() const
{
	if (!m_bUseGlobalTheme && m_bHasLocalTheme)
		return m_localTheme;
	if (m_bHasLocalTheme && !m_bUseGlobalTheme)
		return m_localTheme;
	return ModernUITheme::GetInputTheme();
}

void CSkinnedEdit::PreSubclassWindow()
{
	CEdit::PreSubclassWindow();
	EnsureMultilineForVCenter();
	ApplyThemeAndMargins();
}

int CSkinnedEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;

	EnsureMultilineForVCenter();
	ApplyThemeAndMargins();
	return 0;
}

void CSkinnedEdit::EnsureMultilineForVCenter()
{
	if (!::IsWindow(m_hWnd)) return;

	DWORD style = (DWORD)GetStyle();
	if ((style & ES_MULTILINE) == 0)
	{
		ModifyStyle(0, ES_MULTILINE | ES_AUTOVSCROLL, SWP_FRAMECHANGED);
		ModifyStyle(WS_VSCROLL | WS_HSCROLL, 0, SWP_FRAMECHANGED);
	}
}

LRESULT CSkinnedEdit::OnSetFontMsg(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	LRESULT r = Default();
	EnsureMultilineForVCenter();
	ApplyThemeAndMargins();
	return r;
}

LRESULT CSkinnedEdit::OnSetTextMsg(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	LRESULT r = Default();
	EnsureMultilineForVCenter();
	ApplyThemeAndMargins();
	return r;
}

void CSkinnedEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_RETURN || nChar == 0x0A)
		return;

	// Flicker fix: suppress intermediate WM_PAINTs during typing.
	// Without this, the native Edit triggers its own WM_PAINT mid-keystroke,
	// briefly showing a plain white background before our OnPaint runs.
	SetRedraw(FALSE);
	CEdit::OnChar(nChar, nRepCnt, nFlags);
	SetRedraw(TRUE);
	Invalidate(FALSE);
}

void CSkinnedEdit::ApplyThemeAndMargins()
{
	if (!::IsWindow(m_hWnd)) return;

	SetWindowTheme(m_hWnd, L"", L"");
	ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0, SWP_FRAMECHANGED);
	ModifyStyle(WS_BORDER, 0, SWP_FRAMECHANGED);

	const KFTCInputTheme& th = GetActiveInputTheme();

	::SendMessage(m_hWnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(th.marginLR, th.marginLR));

	CRect rc;
	GetClientRect(&rc);
	if (rc.Width() <= 0 || rc.Height() <= 0) return;

	int textH = 0;
	{
		CClientDC dc(this);
		HFONT hFont = (HFONT)::SendMessage(m_hWnd, WM_GETFONT, 0, 0);
		HFONT hOld = (HFONT)::SelectObject(dc.GetSafeHdc(), hFont ? hFont : (HFONT)::GetStockObject(DEFAULT_GUI_FONT));
		TEXTMETRIC tm = { 0 };
		if (dc.GetTextMetrics(&tm))
		{
			textH = (int)(tm.tmAscent + tm.tmDescent);
			if (textH <= 0) textH = (int)tm.tmHeight;
		}
		::SelectObject(dc.GetSafeHdc(), hOld);
	}

	int h = rc.Height();
	float maxThick = (th.thickF > th.thickN) ? th.thickF : th.thickN;
	int inset = (int)(maxThick + 2.0f);
	if (inset < 2) inset = 2;
	if (inset > 10) inset = 10;

	int contentH = h - inset * 2;
	if (contentH < 1) contentH = 1;

	int padInside = 0;
	if (textH > 0)
	{
		int free = contentH - textH;
		if (free < 0) free = 0;
		padInside = free / 2;
	}

	int topPad = inset + padInside;
	if (topPad < inset) topPad = inset;
	if (topPad > h - inset - 1) topPad = h - inset - 1;

	CRect trc = rc;
	trc.left += th.marginLR;
	trc.right -= th.marginLR;
	trc.top = topPad;
	trc.bottom = h - inset;

	::SendMessage(m_hWnd, EM_SETRECTNP, 0, (LPARAM)&trc);
}

void CSkinnedEdit::TrackMouseLeave()
{
	if (m_bTracking || !::IsWindow(m_hWnd)) return;

	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(TRACKMOUSEEVENT);
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = m_hWnd;
	tme.dwHoverTime = 0;
	if (::TrackMouseEvent(&tme))
		m_bTracking = TRUE;
}

BOOL CSkinnedEdit::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

LRESULT CSkinnedEdit::OnPrintClientMsg(WPARAM wParam, LPARAM lParam)
{
	// Flicker fix: only let DefWindowProc handle WM_PRINTCLIENT when WE
	// sent it from inside our own OnPaint (m_bInPaint == TRUE).
	//
	// If an external caller (parent WM_PAINT, scroll, theme engine) sends
	// WM_PRINTCLIENT, DefWindowProc would paint a plain white rectangle
	// directly to whatever DC was passed -- which can be the real screen DC --
	// causing a one-frame white flash visible to the user.
	//
	// When m_bInPaint == FALSE we redirect to Invalidate() instead so that
	// our double-buffered OnPaint handles the update cleanly.
	if (!m_bInPaint)
	{
		Invalidate(FALSE);
		return 0;
	}
	return DefWindowProc(WM_PRINTCLIENT, wParam, lParam);
}



HBRUSH CSkinnedEdit::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
    // Make the native text/background paint match our underlay so that
    // WM_PRINTCLIENT (used to render text/caret/selection) does not repaint
    // the client area with plain white.
    const COLORREF bg = m_bUseUnderlayBg ? m_clrUnderlayBg : RGB(255, 255, 255);

    if (pDC)
    {
        pDC->SetBkColor(bg);
        // text color is managed by Windows theme; keep default window text
        pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
        pDC->SetBkMode(OPAQUE);
    }

    if (!m_brUnderlay.GetSafeHandle() || m_clrBrushBg != bg)
    {
        if (m_brUnderlay.GetSafeHandle())
            m_brUnderlay.DeleteObject();
        m_brUnderlay.CreateSolidBrush(bg);
        m_clrBrushBg = bg;
    }

    return (HBRUSH)m_brUnderlay.GetSafeHandle();
}
void CSkinnedEdit::AddRoundRect(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& rect, Gdiplus::REAL radius)
{
	path.Reset();

	if (radius <= 0.1f)
	{
		path.AddRectangle(rect);
		path.CloseFigure();
		return;
	}

	const Gdiplus::REAL d = radius * 2.0f;
	path.AddArc(rect.X, rect.Y, d, d, 180.0f, 90.0f);
	path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270.0f, 90.0f);
	path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0.0f, 90.0f);
	path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90.0f, 90.0f);
	path.CloseFigure();
}

void CSkinnedEdit::OnPaint()
{
	if (m_bInPaint)
	{
		CPaintDC dc(this);
		return;
	}
	m_bInPaint = TRUE;

	CPaintDC dc(this);
	CRect rc;
	GetClientRect(&rc);
	if (rc.Width() <= 0 || rc.Height() <= 0)
	{
		m_bInPaint = FALSE;
		return;
	}

	CDC memDC;
	memDC.CreateCompatibleDC(&dc);
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
	CBitmap* pOldBmp = memDC.SelectObject(&bmp);

	// 라운드 테두리 모서리: underlay 없으면 부모 배경
	if (m_bUseUnderlayBg)
		memDC.FillSolidRect(&rc, m_clrUnderlayBg);
	else
		kftc_fill_parent_bg(m_hWnd, memDC.GetSafeHdc(), rc);

	Gdiplus::Graphics g(memDC.m_hDC);
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

	const KFTCInputTheme& th = GetActiveInputTheme();
	const COLORREF crBorderN = th.borderN;
	const COLORREF crBorderH = th.borderH;
	const COLORREF crBorderF = th.borderF;

	// 입력 영역 내부 배경: 항상 흰색 (라운드 rect 안쪽)
	Gdiplus::Color bg(255, 255, 255, 255);

    // 카드 배경과 컨트롤 배경을 일치시키기 위해 underlay가 설정된 경우 내부 fill도 underlay 색을 사용한다.
    if (m_bUseUnderlayBg)
        bg = Gdiplus::Color(255, GetRValue(m_clrUnderlayBg), GetGValue(m_clrUnderlayBg), GetBValue(m_clrUnderlayBg));
	Gdiplus::Color borderN(255, GetRValue(crBorderN), GetGValue(crBorderN), GetBValue(crBorderN));
	Gdiplus::Color borderH(255, GetRValue(crBorderH), GetGValue(crBorderH), GetBValue(crBorderH));
	Gdiplus::Color borderF(255, GetRValue(crBorderF), GetGValue(crBorderF), GetBValue(crBorderF));

	Gdiplus::Color bc = borderN;
	if (m_bFocus)      bc = borderF;
	else if (m_bHover) bc = borderH;

	const int thickI = m_bFocus ? (int)th.thickF : (int)th.thickN;
	const Gdiplus::REAL thick = (Gdiplus::REAL)thickI;

	int radI = m_nRadius;
	int maxRad = (kftc_min_i(rc.Width(), rc.Height()) - 2) / 2;
	if (maxRad < 1) maxRad = 1;
	if (radI > maxRad) radI = maxRad;
	if (radI < 1) radI = 1;

	Gdiplus::RectF rfOuter(0.5f, 0.5f, (Gdiplus::REAL)rc.Width() - 1.0f, (Gdiplus::REAL)rc.Height() - 1.0f);
	Gdiplus::RectF rfInner(rfOuter.X + thick, rfOuter.Y + thick,
		rfOuter.Width - (thick * 2.0f), rfOuter.Height - (thick * 2.0f));

	if (rfInner.Width < 2.0f) rfInner.Width = 2.0f;
	if (rfInner.Height < 2.0f) rfInner.Height = 2.0f;

	Gdiplus::GraphicsPath pathOuter;
	Gdiplus::GraphicsPath pathInner;

	AddRoundRect(pathOuter, rfOuter, (Gdiplus::REAL)radI);

	Gdiplus::REAL innerRad = (Gdiplus::REAL)radI - thick;
	if (innerRad < 1.0f) innerRad = 1.0f;
	AddRoundRect(pathInner, rfInner, innerRad);

	Gdiplus::SolidBrush brBorder(bc);
	g.FillPath(&brBorder, &pathOuter);

	Gdiplus::SolidBrush brBg(bg);
	g.FillPath(&brBg, &pathInner);

	{
		Gdiplus::Pen edgePen(bc, 1.0f);
		edgePen.SetLineJoin(Gdiplus::LineJoinRound);
		edgePen.SetStartCap(Gdiplus::LineCapFlat);
		edgePen.SetEndCap(Gdiplus::LineCapFlat);
		g.DrawPath(&edgePen, &pathOuter);
	}

	int save = memDC.SaveDC();
	{
		int inset = thickI + 1;
		int l = inset, t = inset;
		int r = rc.Width() - inset;
		int b = rc.Height() - inset;
		if (r <= l) r = l + 1;
		if (b <= t) b = t + 1;

		int rr = radI - thickI;
		if (rr < 1) rr = 1;
		int d = rr * 2;

		HRGN hrgn = ::CreateRoundRectRgn(l, t, r + 1, b + 1, d, d);
		::SelectClipRgn(memDC.m_hDC, hrgn);
		::DeleteObject(hrgn);

		::SendMessage(m_hWnd, WM_PRINTCLIENT, (WPARAM)memDC.m_hDC, (LPARAM)(PRF_CLIENT));
	}
	memDC.RestoreDC(save);

	dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);

	memDC.SelectObject(pOldBmp);
	m_bInPaint = FALSE;
}

void CSkinnedEdit::OnSetFocus(CWnd* pOldWnd)
{
	m_bFocus = TRUE;
	CEdit::OnSetFocus(pOldWnd);
	Invalidate(FALSE);
}

void CSkinnedEdit::OnKillFocus(CWnd* pNewWnd)
{
	m_bFocus = FALSE;
	m_bTracking = FALSE;
	CEdit::OnKillFocus(pNewWnd);
	Invalidate(FALSE);
}

void CSkinnedEdit::OnSize(UINT nType, int cx, int cy)
{
	CEdit::OnSize(nType, cx, cy);
	ApplyThemeAndMargins();
}

void CSkinnedEdit::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bHover)
	{
		m_bHover = TRUE;
		Invalidate(FALSE);
	}
	TrackMouseLeave();

	CEdit::OnMouseMove(nFlags, point);
}

void CSkinnedEdit::OnMouseLeave()
{
	m_bHover = FALSE;
	m_bTracking = FALSE;
	Invalidate(FALSE);
}

void CSkinnedComboBox::SetUnderlayColor(COLORREF underlayBg)
{
	m_bUseUnderlayBg = TRUE;
	m_clrUnderlayBg = underlayBg;
	Invalidate(FALSE);
}

void CSkinnedComboBox::ClearUnderlayColor()
{
	m_bUseUnderlayBg = FALSE;
	Invalidate(FALSE);
}

void CSkinnedEdit::SetUnderlayColor(COLORREF underlayBg)
{
	m_bUseUnderlayBg = TRUE;
	m_clrUnderlayBg = underlayBg;
	Invalidate(FALSE);
}

void CSkinnedEdit::ClearUnderlayColor()
{
	m_bUseUnderlayBg = FALSE;
	Invalidate(FALSE);
}

// ============================================================
// CModernTabCtrl 구현
// ============================================================
static void MakeRoundRect(Gdiplus::GraphicsPath& path,
	const Gdiplus::RectF& r, float radius)
{
	path.Reset();
	if (radius <= 0.0f) { path.AddRectangle(r); return; }
	const float d = radius * 2.0f;
	path.AddArc(r.X, r.Y, d, d, 180.0f, 90.0f);
	path.AddArc(r.X + r.Width - d, r.Y, d, d, 270.0f, 90.0f);
	path.AddArc(r.X + r.Width - d, r.Y + r.Height - d, d, d, 0.0f, 90.0f);
	path.AddArc(r.X, r.Y + r.Height - d, d, d, 90.0f, 90.0f);
	path.CloseFigure();
}

BEGIN_MESSAGE_MAP(CModernTabCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
END_MESSAGE_MAP()

// -----------------------------------------------------------
// 생성 / 소멸
// -----------------------------------------------------------
CModernTabCtrl::CModernTabCtrl()
	: m_nSel(0), m_nHover(-1), m_bTrack(false)
{
}

CModernTabCtrl::~CModernTabCtrl()
{
	if (m_font.GetSafeHandle())     m_font.DeleteObject();
	if (m_fontBold.GetSafeHandle()) m_fontBold.DeleteObject();
	if (m_brushBg.GetSafeHandle())  m_brushBg.DeleteObject();
}

BOOL CModernTabCtrl::Create(CWnd* pParent, UINT nID, const CRect& rect)
{
	m_brushBg.CreateSolidBrush(RGB(255, 255, 255));

	m_font.CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Malgun Gothic"));
	m_fontBold.CreateFont(13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, 0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("Malgun Gothic"));

	LPCTSTR cls = AfxRegisterWndClass(
		CS_HREDRAW | CS_VREDRAW,
		::LoadCursor(NULL, IDC_ARROW),
		(HBRUSH)GetStockObject(WHITE_BRUSH));

	return CWnd::Create(cls, _T(""),
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
		rect, pParent, nID);
}

void CModernTabCtrl::AddTab(LPCTSTR text, int iconType)
{
	ModernTabItem item;
	item.text = text;
	item.iconType = iconType;
	m_items.push_back(item);
}

void CModernTabCtrl::SetCurSel(int n)
{
	if (n < 0 || n >= (int)m_items.size()) return;
	m_nSel = n;
	Invalidate();
	UpdateWindow();

	NMHDR nm;
	::ZeroMemory(&nm, sizeof(nm));
	nm.hwndFrom = GetSafeHwnd();
	nm.idFrom = (UINT_PTR)GetDlgCtrlID();
	nm.code = TCN_SELCHANGE;
	CWnd* pParent = GetParent();
	if (pParent && pParent->GetSafeHwnd())
		pParent->SendMessage(WM_NOTIFY, (WPARAM)GetDlgCtrlID(), (LPARAM)&nm);
}

RectF CModernTabCtrl::GetTabRect(int idx) const
{
	CRect wndRc;
	GetClientRect(&wndRc);

	const int cnt = (int)m_items.size();
	if (cnt == 0) return Gdiplus::RectF(0.0f, 0.0f, 0.0f, 0.0f);

	float eachW = (float)wndRc.Width() / (float)cnt;
	float x = (float)idx * eachW;
	float y = 0.0f;
	float h = (float)wndRc.Height() - 2.0f;

	return Gdiplus::RectF(x, y, eachW, h);
}

void CModernTabCtrl::OnPaint()
{
	CPaintDC dc(this);
	CRect wndRc;
	GetClientRect(&wndRc);

	CDC memDC;
	memDC.CreateCompatibleDC(&dc);
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(&dc, wndRc.Width(), wndRc.Height());
	CBitmap* pOld = memDC.SelectObject(&bmp);

	memDC.FillSolidRect(wndRc, RGB(255, 255, 255));

	ModernUIGfx::EnsureGdiplusStartup();

	Gdiplus::Graphics g(memDC.GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

	const float kBarX = 8.0f;
	const float kBarW = (float)wndRc.Width() - kBarX * 2.0f;
	const float kBarY = (float)(wndRc.Height() - kBarH) * 0.5f;

	Gdiplus::Pen botLine(Gdiplus::Color(255, 220, 224, 234), 1.0f);
	g.DrawLine(&botLine,
		Gdiplus::PointF(0.0f, (float)wndRc.Height() - 1.0f),
		Gdiplus::PointF((float)wndRc.Width(), (float)wndRc.Height() - 1.0f));

	for (int i = 0; i < (int)m_items.size(); i++)
		DrawTab(g, i, GetTabRect(i));

	dc.BitBlt(0, 0, wndRc.Width(), wndRc.Height(), &memDC, 0, 0, SRCCOPY);
	memDC.SelectObject(pOld);
}

BOOL CModernTabCtrl::OnEraseBkgnd(CDC* /*pDC*/) { return TRUE; }

void CModernTabCtrl::DrawTab(Graphics& g, int idx, const RectF& rc)
{
	const bool bActive = (idx == m_nSel);
	const bool bHover = (idx == m_nHover && !bActive);

	if (bHover)
	{
		Gdiplus::SolidBrush hBrush(Gdiplus::Color(255, 246, 248, 252));
		g.FillRectangle(&hBrush, rc);
	}

	if (bActive)
	{
		const float iW = rc.Width * 0.78f;
		const float iX = rc.X + (rc.Width - iW) * 0.5f;
		const float iY = rc.Y + rc.Height - 3.5f;
		Gdiplus::GraphicsPath iPath;
		Gdiplus::RectF iRc(iX, iY, iW, 3.5f);
		MakeRoundRect(iPath, iRc, 1.8f);
		Gdiplus::SolidBrush iBrush(Gdiplus::Color(255, 0, 96, 210));
		g.FillPath(&iBrush, &iPath);
	}
	else if (bHover)
	{
		const float iW = rc.Width * 0.45f;
		const float iX = rc.X + (rc.Width - iW) * 0.5f;
		const float iY = rc.Y + rc.Height - 2.5f;
		Gdiplus::GraphicsPath iPath;
		Gdiplus::RectF iRc(iX, iY, iW, 2.0f);
		MakeRoundRect(iPath, iRc, 1.0f);
		Gdiplus::SolidBrush iBrush(Gdiplus::Color(100, 0, 96, 210));
		g.FillPath(&iBrush, &iPath);
	}

	const float kIconSz = 16.0f;
	const float kIconGap = 6.0f;

	Gdiplus::FontFamily ff(L"Malgun Gothic");

	Gdiplus::Font fontN(&ff, 12.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
	Gdiplus::Font fontB(&ff, 12.5f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	Gdiplus::Font* pFont = bActive ? &fontB : &fontN;

	Gdiplus::RectF measRc(0.0f, 0.0f, 0.0f, 0.0f);
	g.MeasureString(m_items[idx].text.GetString(), -1,
		pFont, Gdiplus::PointF(0.0f, 0.0f), &measRc);

	float totalW = kIconSz + kIconGap + measRc.Width;
	float startX = rc.X + (rc.Width - totalW) * 0.5f;
	float midY = rc.Y + rc.Height * 0.5f;

	DrawIcon(g, m_items[idx].iconType,
		startX + kIconSz * 0.5f, midY, kIconSz, bActive);

	Gdiplus::Color txtColor;
	if (bActive)      txtColor = Gdiplus::Color(255, 0, 76, 200);
	else if (bHover)  txtColor = Gdiplus::Color(255, 60, 72, 96);
	else              txtColor = Gdiplus::Color(255, 128, 138, 160);

	Gdiplus::SolidBrush txtBrush(txtColor);
	Gdiplus::RectF txtRc(startX + kIconSz + kIconGap,
		midY - measRc.Height * 0.5f,
		measRc.Width + 2.0f, measRc.Height);
	Gdiplus::StringFormat sf;
	sf.SetAlignment(Gdiplus::StringAlignmentNear);
	sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
	g.DrawString(m_items[idx].text.GetString(), -1,
		pFont, txtRc, &sf, &txtBrush);
}

void CModernTabCtrl::DrawIcon(Graphics& g, int iconType,
	REAL cx, REAL cy,
	REAL sz, bool active)
{
	Gdiplus::Color iconColor = active
		? Gdiplus::Color(255, 0, 96, 210)
		: Gdiplus::Color(255, 140, 152, 175);

	Gdiplus::Pen pen(iconColor, 1.4f);
	pen.SetStartCap(Gdiplus::LineCapRound);
	pen.SetEndCap(Gdiplus::LineCapRound);
	pen.SetLineJoin(Gdiplus::LineJoinRound);

	const Gdiplus::REAL hw = sz * 0.42f;

	switch (iconType)
	{
	case 0:
	{
		Gdiplus::RectF r(cx - hw, cy - hw * 0.8f, hw * 2.0f, hw * 1.5f);
		g.DrawRectangle(&pen, r);
		g.DrawLine(&pen, cx - hw * 0.4f, cy + hw * 0.7f,
			cx + hw * 0.4f, cy + hw * 0.7f);
		g.DrawLine(&pen, cx, cy + hw * 0.7f, cx, cy + hw);
		break;
	}
	case 1:
	{
		Gdiplus::RectF r(cx - hw * 0.6f, cy - hw, hw * 1.2f, hw * 2.0f);
		g.DrawRectangle(&pen, r);
		Gdiplus::SolidBrush dot(iconColor);
		g.FillEllipse(&dot, cx - 1.5f, cy + hw * 0.55f, 3.0f, 3.0f);
		break;
	}
	case 2:
	{
		Gdiplus::REAL rr = sz * 0.32f;
		g.DrawEllipse(&pen, cx - rr, cy - rr, rr * 2.0f, rr * 2.0f);
		g.DrawLine(&pen, cx - hw, cy, cx + hw, cy);
		g.DrawLine(&pen, cx, cy - hw, cx, cy + hw);
		break;
	}
	case 3:
	{
		Gdiplus::PointF roof[3];
		roof[0] = Gdiplus::PointF(cx, cy - hw);
		roof[1] = Gdiplus::PointF(cx - hw, cy);
		roof[2] = Gdiplus::PointF(cx + hw, cy);
		g.DrawPolygon(&pen, roof, 3);
		Gdiplus::RectF door(cx - hw * 0.45f, cy, hw * 0.9f, hw);
		g.DrawRectangle(&pen, door);
		break;
	}
	default: break;
	}
}

void CModernTabCtrl::OnLButtonDown(UINT nFlags, CPoint pt)
{
	for (int i = 0; i < (int)m_items.size(); i++)
	{
		Gdiplus::RectF r = GetTabRect(i);
		Gdiplus::RectF hit(r.X - 4.0f, r.Y - 2.0f,
			r.Width + 8.0f, r.Height + 4.0f);
		if (hit.Contains((Gdiplus::REAL)pt.x, (Gdiplus::REAL)pt.y))
		{
			if (i != m_nSel) SetCurSel(i);
			return;
		}
	}
	CWnd::OnLButtonDown(nFlags, pt);
}

void CModernTabCtrl::EnsureTrack()
{
	if (m_bTrack) return;
	TRACKMOUSEEVENT tme;
	::ZeroMemory(&tme, sizeof(tme));
	tme.cbSize = sizeof(tme);
	tme.dwFlags = TME_HOVER | TME_LEAVE;
	tme.hwndTrack = GetSafeHwnd();
	tme.dwHoverTime = 60;
	::TrackMouseEvent(&tme);
	m_bTrack = true;
}

void CModernTabCtrl::OnMouseMove(UINT nFlags, CPoint pt)
{
	EnsureTrack();
	int prev = m_nHover;
	m_nHover = -1;
	for (int i = 0; i < (int)m_items.size(); i++)
	{
		Gdiplus::RectF r = GetTabRect(i);
		Gdiplus::RectF hit(r.X - 4.0f, r.Y - 2.0f,
			r.Width + 8.0f, r.Height + 4.0f);
		if (hit.Contains((Gdiplus::REAL)pt.x, (Gdiplus::REAL)pt.y))
		{
			m_nHover = i;
			break;
		}
	}
	if (m_nHover != prev) { Invalidate(); UpdateWindow(); }
	CWnd::OnMouseMove(nFlags, pt);
}

void CModernTabCtrl::OnMouseLeave()
{
	m_bTrack = false;
	if (m_nHover != -1) { m_nHover = -1; Invalidate(); UpdateWindow(); }
}

LRESULT CModernTabCtrl::OnMouseHover(WPARAM /*wp*/, LPARAM /*lp*/)
{
	return 0;
}

// ============================================================================
// CInfoText (Read-only value display)
// ============================================================================
BEGIN_MESSAGE_MAP(CInfoText, CStatic)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL CInfoText::OnEraseBkgnd(CDC* /*pDC*/)
{
    // 깜빡임 방지: OnPaint에서 배경까지 직접 그림
    return TRUE;
}

void CInfoText::OnPaint()
{
    CPaintDC dc(this);
    CRect rc; GetClientRect(&rc);

    // background
    dc.FillSolidRect(rc, m_crUnderlay);

    // text
    CString s; GetWindowText(s);
    const bool bPlaceholder = (s.IsEmpty() && !m_strPlaceholder.IsEmpty());
    if (bPlaceholder) s = m_strPlaceholder;

    // font: 기본은 부모 설정을 따르되 Bold 옵션만 제공
    CFont* pOldFont = nullptr;
    CFont  boldFont;
    if (m_bBold)
    {
        CFont* pFont = GetFont();
        LOGFONT lf{};
        if (pFont && pFont->GetLogFont(&lf))
        {
            lf.lfWeight = FW_SEMIBOLD; // 너무 두껍지 않게
            boldFont.CreateFontIndirect(&lf);
            pOldFont = dc.SelectObject(&boldFont);
        }
    }
    if (!pOldFont)
        pOldFont = dc.SelectObject(GetFont());

    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(bPlaceholder ? m_crPlaceholder : m_crText);

    CRect rt = rc;
    rt.DeflateRect(m_nPadX, m_nPadY);

    dc.DrawText(s, rt, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    dc.SelectObject(pOldFont);
}
// ============================================================================
// CInfoIconButton - circular "i" info icon button
// ============================================================================

BEGIN_MESSAGE_MAP(CInfoIconButton, CButton)
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

CInfoIconButton::CInfoIconButton()
    : m_bHover(FALSE), m_bTracking(FALSE)
    , m_bUseUnderlay(FALSE), m_clrUnderlay(RGB(249,250,252))
{
}

void CInfoIconButton::OnMouseMove(UINT nFlags, CPoint point)
{
    if (!m_bTracking)
    {
        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
        tme.dwFlags   = TME_LEAVE | TME_HOVER;
        tme.hwndTrack = m_hWnd;
        tme.dwHoverTime = 1;
        TrackMouseEvent(&tme);
        m_bTracking = TRUE;
    }
    if (!m_bHover) { m_bHover = TRUE; Invalidate(FALSE); }
    CButton::OnMouseMove(nFlags, point);
}

void CInfoIconButton::OnMouseLeave()
{
    m_bTracking = FALSE;
    if (m_bHover) { m_bHover = FALSE; Invalidate(FALSE); }
}

void CInfoIconButton::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
    ModernUIGfx::EnsureGdiplusStartup();

    CDC* pDC = CDC::FromHandle(lpDIS->hDC);
    CRect rect(lpDIS->rcItem);

    COLORREF crBg = m_bUseUnderlay ? m_clrUnderlay
                                   : kftc_parent_bg_color(m_hWnd, lpDIS->hDC);
    pDC->FillSolidRect(&rect, crBg);

    Gdiplus::Graphics g(lpDIS->hDC);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    float sz  = (float)min(rect.Width(), rect.Height()) - 2.0f;
    float ox  = (rect.Width()  - sz) * 0.5f;
    float oy  = (rect.Height() - sz) * 0.5f;
    Gdiplus::RectF rf(ox, oy, sz, sz);

    COLORREF cirFill = m_bHover ? BLUE_100 : RGB(220, 232, 248);
    Gdiplus::SolidBrush brFill(Gdiplus::Color(255,
        GetRValue(cirFill), GetGValue(cirFill), GetBValue(cirFill)));
    g.FillEllipse(&brFill, rf);

    Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(BLUE_300),
                                        GetGValue(BLUE_300),
                                        GetBValue(BLUE_300)), 1.0f);
    g.DrawEllipse(&pen, rf);

    Gdiplus::FontFamily ff(L"Malgun Gothic");
    Gdiplus::Font font(&ff, sz * 0.52f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush brText(Gdiplus::Color(255,
        GetRValue(KFTC_PRIMARY), GetGValue(KFTC_PRIMARY), GetBValue(KFTC_PRIMARY)));
    Gdiplus::StringFormat sf;
    sf.SetAlignment(Gdiplus::StringAlignmentCenter);
    sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    g.DrawString(L"?", -1, &font, rf, &sf, &brText);
}

// ============================================================================
// CModernPopover - floating info popover
// ============================================================================

BEGIN_MESSAGE_MAP(CModernPopover, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

HHOOK           CModernPopover::s_hMouseHook  = NULL;
CModernPopover* CModernPopover::s_pPopoverInst = NULL;

CModernPopover::CModernPopover()
    : m_nArrowX(0), m_bVisible(FALSE)
{
}

/*static*/ void CModernPopover::RegisterPopoverClass()
{
    static bool s_registered = false;
    if (s_registered) return;
    s_registered = true;

    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style         = CS_DROPSHADOW | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = ::DefWindowProc;
    wc.hInstance     = AfxGetInstanceHandle();
    wc.hbrBackground = NULL;
    wc.lpszClassName = _T("KFTCModernPopover");
    ::RegisterClassEx(&wc);
}

void CModernPopover::AddRoundRect(Gdiplus::GraphicsPath& path,
                                   const Gdiplus::RectF& r, REAL radius)
{
    const float d = radius * 2.0f;
    Gdiplus::RectF a(r.X, r.Y, d, d);
    path.AddArc(a, 180, 90); a.X = r.X + r.Width - d;
    path.AddArc(a, 270, 90); a.Y = r.Y + r.Height - d;
    path.AddArc(a,   0, 90); a.X = r.X;
    path.AddArc(a,  90, 90); path.CloseFigure();
}

void CModernPopover::ShowAt(const CRect& anchorScrRc, LPCTSTR title,
                             LPCTSTR body, CWnd* pParent)
{
    m_strTitle = title;
    m_strBody  = body;

    RegisterPopoverClass();

    if (!GetSafeHwnd())
    {
        CreateEx(WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED,
                 _T("KFTCModernPopover"), _T(""),
                 WS_POPUP,
                 0, 0, kPopW, kPopH + kArrowH,
                 pParent ? pParent->GetSafeHwnd() : NULL, NULL);
    }

    HWND hRef = pParent ? pParent->GetSafeHwnd() : m_hWnd;
    int popW = ModernUIDpi::Scale(hRef, kPopW);
    int popH = ModernUIDpi::Scale(hRef, kPopH + kArrowH);

    int px = anchorScrRc.CenterPoint().x - popW / 2;
    int py = anchorScrRc.bottom + 2;

    int screenH = ::GetSystemMetrics(SM_CYSCREEN);
    if (py + popH > screenH - 10)
        py = anchorScrRc.top - popH - 2;

    m_nArrowX = anchorScrRc.CenterPoint().x - px;
    if (m_nArrowX < 14) m_nArrowX = 14;
    if (m_nArrowX > popW - 14) m_nArrowX = popW - 14;

    SetWindowPos(&wndTopMost, px, py, popW, popH,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    RefreshLayered();
    m_bVisible = TRUE;

    // Low-level mouse hook: close popup on any click outside
    if (!s_hMouseHook)
    {
        s_pPopoverInst = this;
        s_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
    }
}

void CModernPopover::Hide()
{
    if (GetSafeHwnd()) ShowWindow(SW_HIDE);
    m_bVisible = FALSE;
    if (s_hMouseHook)
    {
        UnhookWindowsHookEx(s_hMouseHook);
        s_hMouseHook  = NULL;
        s_pPopoverInst = NULL;
    }
}

BOOL CModernPopover::OnEraseBkgnd(CDC* pDC)
{
    return TRUE;
}

void CModernPopover::OnLButtonDown(UINT nFlags, CPoint point)
{
    Hide();
}

void CModernPopover::OnPaint()
{
    CPaintDC dc(this); // validate paint region
    UNREFERENCED_PARAMETER(dc);
    // Content rendered via UpdateLayeredWindow in RefreshLayered()
}

// ----------------------------------------------------------------------------
// RefreshLayered -- draws popup to a 32-bpp DIB and calls UpdateLayeredWindow.
// Per-pixel alpha gives smooth anti-aliased rounded corners with no clipping.
// ----------------------------------------------------------------------------
void CModernPopover::RefreshLayered()
{
    if (!GetSafeHwnd()) return;

    CRect rcWin;
    GetWindowRect(&rcWin);
    const int W = rcWin.Width();
    const int H = rcWin.Height();
    if (W <= 0 || H <= 0) return;

    HDC hdcScreen = ::GetDC(NULL);
    HDC hdcMem    = ::CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi   = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = W;
    bmi.bmiHeader.biHeight      = -H;   // top-down
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    BYTE*   pvBits = NULL;
    HBITMAP hBmp   = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS,
                                      (void**)&pvBits, NULL, 0);
    if (!hBmp) { ::DeleteDC(hdcMem); ::ReleaseDC(NULL, hdcScreen); return; }

    HBITMAP hOldBmp = (HBITMAP)::SelectObject(hdcMem, hBmp);
    ::ZeroMemory(pvBits, W * H * 4);   // fully transparent initially

    ModernUIGfx::EnsureGdiplusStartup();

    {
        // Gdiplus::Bitmap wraps the DIB (BGRA in memory == PixelFormat32bppARGB)
        Gdiplus::Bitmap bmpGdi(W, H, W * 4, PixelFormat32bppARGB, pvBits);
        Gdiplus::Graphics g(&bmpGdi);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

        const int   arrowH  = ModernUIDpi::Scale(m_hWnd, kArrowH);
        const int   arrowHW = ModernUIDpi::Scale(m_hWnd, 8);
        const float radius  = ModernUIDpi::ScaleF(m_hWnd, 10.0f);

        Gdiplus::RectF body(1.0f, (float)arrowH,
                            (float)W - 2.0f,
                            (float)H - arrowH - 1.0f);

        // -- White body (rounded rect, no border) --
        Gdiplus::GraphicsPath bodyPath;
        AddRoundRect(bodyPath, body, radius);
        Gdiplus::SolidBrush brBody(Gdiplus::Color(255, 255, 255, 255));
        g.FillPath(&brBody, &bodyPath);

        // -- Arrow: filled blue (BLUE_500), no outline --
        Gdiplus::PointF arrowPts[3] = {
            Gdiplus::PointF((float)m_nArrowX,              1.0f),
            Gdiplus::PointF((float)(m_nArrowX - arrowHW),  (float)arrowH),
            Gdiplus::PointF((float)(m_nArrowX + arrowHW),  (float)arrowH),
        };
        Gdiplus::SolidBrush brArrow(Gdiplus::Color(255,
            GetRValue(BLUE_500), GetGValue(BLUE_500), GetBValue(BLUE_500)));
        g.FillPolygon(&brArrow, arrowPts, 3);

        // -- Accent bar (BLUE_500 -> BLUE_400 gradient) --
        const float accentH = ModernUIDpi::ScaleF(m_hWnd, 36.0f);
        Gdiplus::RectF accentRc(body.X, body.Y, body.Width, accentH);
        Gdiplus::GraphicsPath accentPath;
        float d = radius * 2.0f;
        accentPath.AddArc(Gdiplus::RectF(accentRc.X, accentRc.Y, d, d), 180, 90);
        accentPath.AddArc(Gdiplus::RectF(accentRc.X + accentRc.Width - d,
                                         accentRc.Y, d, d), 270, 90);
        accentPath.AddLine(accentRc.X + accentRc.Width, accentRc.Y + accentH,
                           accentRc.X,                  accentRc.Y + accentH);
        accentPath.CloseFigure();
        Gdiplus::LinearGradientBrush accentBrush(
            Gdiplus::PointF(0, accentRc.Y),
            Gdiplus::PointF(0, accentRc.Y + accentH),
            Gdiplus::Color(255, GetRValue(BLUE_500), GetGValue(BLUE_500), GetBValue(BLUE_500)),
            Gdiplus::Color(255, GetRValue(BLUE_400), GetGValue(BLUE_400), GetBValue(BLUE_400)));
        g.FillPath(&accentBrush, &accentPath);

        // -- Title (white bold 12px over accent) --
        Gdiplus::FontFamily ff(L"Malgun Gothic");
        Gdiplus::Font fTitle(&ff, ModernUIDpi::ScaleF(m_hWnd, 12.0f),
                             Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brWhiteText(Gdiplus::Color(255, 255, 255, 255));
        Gdiplus::StringFormat sfTitle;
        sfTitle.SetAlignment(Gdiplus::StringAlignmentNear);
        sfTitle.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        sfTitle.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
        sfTitle.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
        float padX = ModernUIDpi::ScaleF(m_hWnd, 14.0f);
        Gdiplus::RectF titleRc(body.X + padX, body.Y,
                               body.Width - padX * 2.0f, accentH);
        std::wstring wTitle = kftc_to_wide(m_strTitle);
        g.DrawString(wTitle.c_str(), -1, &fTitle, titleRc, &sfTitle, &brWhiteText);

        // -- Body text (dark gray 11px) --
        Gdiplus::Font fBody(&ff, ModernUIDpi::ScaleF(m_hWnd, 11.0f),
                            Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush brBodyText(Gdiplus::Color(255, 55, 65, 81));
        Gdiplus::StringFormat sfBody;
        sfBody.SetAlignment(Gdiplus::StringAlignmentNear);
        sfBody.SetLineAlignment(Gdiplus::StringAlignmentNear);
        float bodyTextTop = body.Y + accentH + ModernUIDpi::ScaleF(m_hWnd, 10.0f);
        float bodyTextH   = body.Y + body.Height - bodyTextTop
                            - ModernUIDpi::ScaleF(m_hWnd, 10.0f);
        Gdiplus::RectF bodyRc(body.X + padX, bodyTextTop,
                              body.Width - padX * 2.0f, bodyTextH);
        std::wstring wBody = kftc_to_wide(m_strBody);
        g.DrawString(wBody.c_str(), -1, &fBody, bodyRc, &sfBody, &brBodyText);
    }

    // Premultiply alpha (required for ULW_ALPHA / AC_SRC_ALPHA)
    for (int i = 0; i < W * H; ++i)
    {
        BYTE a = pvBits[i*4 + 3];
        if (a > 0 && a < 255)
        {
            pvBits[i*4+0] = static_cast<BYTE>(pvBits[i*4+0] * a / 255);
            pvBits[i*4+1] = static_cast<BYTE>(pvBits[i*4+1] * a / 255);
            pvBits[i*4+2] = static_cast<BYTE>(pvBits[i*4+2] * a / 255);
        }
    }

    POINT          ptDst = { rcWin.left, rcWin.top };
    SIZE           szWnd = { W, H };
    POINT          ptSrc = { 0, 0 };
    BLENDFUNCTION  bf    = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    ::UpdateLayeredWindow(m_hWnd, hdcScreen,
                          &ptDst, &szWnd,
                          hdcMem, &ptSrc, 0, &bf, ULW_ALPHA);

    ::SelectObject(hdcMem, hOldBmp);
    ::DeleteObject(hBmp);
    ::DeleteDC(hdcMem);
    ::ReleaseDC(NULL, hdcScreen);
}

// ----------------------------------------------------------------------------
// MouseHookProc -- WH_MOUSE_LL: close popup on click outside its rect
// ----------------------------------------------------------------------------
/*static*/ LRESULT CALLBACK CModernPopover::MouseHookProc(int nCode,
                                                           WPARAM wParam,
                                                           LPARAM lParam)
{
    HHOOK hSave = s_hMouseHook;   // capture before Hide() may clear it
    if (nCode == HC_ACTION &&
        (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN ||
         wParam == WM_NCLBUTTONDOWN || wParam == WM_NCRBUTTONDOWN) &&
        s_pPopoverInst != NULL && s_pPopoverInst->IsVisible())
    {
        MSLLHOOKSTRUCT* p = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        CRect rc;
        s_pPopoverInst->GetWindowRect(&rc);
        if (!rc.PtInRect(p->pt))
            s_pPopoverInst->Hide();
    }
    return ::CallNextHookEx(hSave, nCode, wParam, lParam);
}