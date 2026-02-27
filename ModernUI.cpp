// ModernUI.cpp -   
// CModernButton + CModernCheckBox + CPortToggleButton

#include "stdafx.h"
#include "ModernUI.h"
#include <string>
#include <vector>
#include <cmath>

// --- small safe helpers (avoid min/max macro conflicts) ---
static __inline int kftc_min_i(int a, int b) { return (a < b) ? a : b; }

//  ¥è   
//  ()     
// ¥è       .
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
// CModernButton 
// ========================================

BEGIN_MESSAGE_MAP(CModernButton, CButton)
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_ERASEBKGND()
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


BOOL CModernButton::OnEraseBkgnd(CDC* pDC)
{
	// OwnerDraw         
	return TRUE;
}

void CModernButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rect = lpDrawItemStruct->rcItem;

	// 
	const bool pressed = (m_bPressed != FALSE) || ((lpDrawItemStruct->itemState & ODS_SELECTED) != 0);
	const bool hover   = (m_bHover != FALSE);

	// :  DC      (hover/pressed  )
	CDC memDC;
	memDC.CreateCompatibleDC(pDC);

	CBitmap memBmp;
	memBmp.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
	CBitmap* pOldBmp = memDC.SelectObject(&memBmp);

	//  (0,0) 
	CRect rc(0, 0, rect.Width(), rect.Height());

	//   :   ¥è(or underlay)   
	{
		COLORREF bgC = m_bUseUnderlayBg
			? m_clrUnderlayBg
			: kftc_parent_bg_color(m_hWnd, pDC->m_hDC);
		memDC.FillSolidRect(&rc, bgC);
	}

	ModernUIGfx::EnsureGdiplusStartup();
	Gdiplus::Graphics g(memDC.m_hDC);
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);

	CString strText;
	GetWindowText(strText);

	//   
		// ¹öÆ° Å¸ÀÔ: ÅØ½ºÆ® ±â¹Ý(¿äÃ» À¯Áö)
	CString t = strText; t.Trim();
	BOOL isPrimary  = (t.Find(_T("È®ÀÎ")) >= 0);
	BOOL isExit     = (t.Find(_T("Ãë¼Ò")) >= 0) || (t.Find(_T("´Ý±â")) >= 0) || (t.Find(_T("»èÁ¦")) >= 0);
	BOOL isDownload = (t.Find(_T("´Ù¿î")) >= 0);
	BOOL isMini     = (t.Find(_T("ÃÖ¼Ò")) >= 0) || (t.Find(_T("Ãà¼Ò")) >= 0);
//   (//¥å)

	//  :    1px  
	const int pressOffset = pressed ? 1 : 0;

	const float rad = 8.0f;
	Gdiplus::RectF rf(
		(float)rc.left + 1.5f,
		(float)rc.top  + 1.5f + (float)pressOffset,
		(float)rc.Width()  - 3.0f,
		(float)rc.Height() - 3.0f);

	//  (Pressed    "" )
	{
		const int shadowA = pressed ? 6 : 12;
		Gdiplus::RectF sh(rf.X, rf.Y + 1.5f, rf.Width, rf.Height);
		Gdiplus::GraphicsPath sp; AddRoundRect(sp, sh, rad);
		Gdiplus::SolidBrush sb(Gdiplus::Color(shadowA, 0, 0, 0));
		g.FillPath(&sb, &sp);
	}

	Gdiplus::GraphicsPath bp; AddRoundRect(bp, rf, rad);

	// ==============================
	// /¥è: Hover/Pressed " "  
	// (Hover ¥è ¥â , Pressed   )
	// ==============================
		// ==============================
	// ¹è°æ/Å×µÎ¸®: Primary / Secondary / Tint
	// ==============================
	if (isPrimary)
	{
		COLORREF c = pressed ? BLUE_700 : (hover ? BLUE_600 : BLUE_500);
		Gdiplus::SolidBrush br(Gdiplus::Color(255, GetRValue(c), GetGValue(c), GetBValue(c)));
		g.FillPath(&br, &bp);
	}
	else if (isDownload)
	{
		COLORREF bg = pressed ? BLUE_200 : (hover ? BLUE_100 : BLUE_50);
		Gdiplus::SolidBrush br(Gdiplus::Color(255, GetRValue(bg), GetGValue(bg), GetBValue(bg)));
		g.FillPath(&br, &bp);
		Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(BLUE_200), GetGValue(BLUE_200), GetBValue(BLUE_200)), 1.0f);
		pen.SetLineJoin(Gdiplus::LineJoinRound);
		g.DrawPath(&pen, &bp);
	}
	else
	{
		// Secondary (Ãë¼Ò/´Ý±â/ÀÏ¹Ý)
		COLORREF bg = pressed ? GRAY_100 : (hover ? GRAY_50 : RGB(255, 255, 255));
		Gdiplus::SolidBrush br(Gdiplus::Color(255, GetRValue(bg), GetGValue(bg), GetBValue(bg)));
		g.FillPath(&br, &bp);
		COLORREF border = hover ? GRAY_300 : GRAY_200;
		Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(border), GetGValue(border), GetBValue(border)), 1.2f);
		pen.SetLineJoin(Gdiplus::LineJoinRound);
		g.DrawPath(&pen, &bp);
	}

Gdiplus::Color txtColor = (isPrimary)
		? Gdiplus::Color(255, 255, 255, 255)
		: (isDownload
			? Gdiplus::Color(255, GetRValue(BLUE_500), GetGValue(BLUE_500), GetBValue(BLUE_500))
			: Gdiplus::Color(255, GetRValue(GRAY_800), GetGValue(GRAY_800), GetBValue(GRAY_800)));

	Gdiplus::SolidBrush tb(txtColor);
	Gdiplus::FontFamily ff(L"Malgun Gothic");
	Gdiplus::Font font(&ff, (Gdiplus::REAL)ModernUIDpi::ScaleF(m_hWnd, 12.5f), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	Gdiplus::StringFormat sf;
	sf.SetAlignment(Gdiplus::StringAlignmentCenter);
	sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);

	std::wstring wt = kftc_to_wide(strText);
	g.DrawString(wt.c_str(), -1, &font, rf, &sf, &tb);

	// 
	pDC->BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);

	memDC.SelectObject(pOldBmp);
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
// CModernCheckBox 
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

	m_nTextPx = 13; //  
	m_bNoWrapEllipsis = FALSE;

	m_bUseGdiText = FALSE;      // : (GDI+) 
	m_nGdiTextYOffset = 0;      // GDI   
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

	// ¥è 
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

	// : underlay  ¥è  (  )
	COLORREF crUnder = m_bUseUnderlayBg
		? m_clrUnderlayBg
		: kftc_parent_bg_color(m_hWnd, pDC->m_hDC);
	Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255,
		GetRValue(crUnder), GetGValue(crUnder), GetBValue(crUnder)));
	graphics.FillRectangle(&bgBrush, Gdiplus::Rect(0, 0, rect.Width(), rect.Height()));

	//    
	REAL boxSize = (float)ModernUIDpi::Scale(m_hWnd, 18);
	REAL boxX = 0;
	REAL boxY = (rect.Height() - boxSize) / 2.0f;

	Gdiplus::RectF boxRect(boxX, boxY, boxSize, boxSize);

	//  
	Gdiplus::GraphicsPath boxPath;
	AddRoundRect(boxPath, boxRect, ModernUIDpi::ScaleF(m_hWnd, 4.0f));

	if (m_bChecked)
	{
		//  
		Gdiplus::LinearGradientBrush checkBrush(
			Gdiplus::PointF(boxRect.X, boxRect.Y),
			Gdiplus::PointF(boxRect.X, boxRect.Y + boxRect.Height),
			Gdiplus::Color(255, 0, 100, 221),
			Gdiplus::Color(255, 15, 124, 255)
		);
		graphics.FillPath(&checkBrush, &boxPath);

		//  
		Gdiplus::Pen checkPen(Gdiplus::Color(255, 255, 255, 255), (float)ModernUIDpi::Scale(m_hWnd, 2));
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
		//   
		Gdiplus::SolidBrush uncheckBrush(Gdiplus::Color(255, 255, 255, 255));
		graphics.FillPath(&uncheckBrush, &boxPath);
	}

	// ¥è
	Gdiplus::Color borderColor;
	borderColor = m_bHover ? Gdiplus::Color(255, 15, 124, 255) : Gdiplus::Color(255, 168, 208, 255);

	Gdiplus::Pen borderPen(borderColor, m_bHover ? ModernUIDpi::ScaleF(m_hWnd, 1.6f) : ModernUIDpi::ScaleF(m_hWnd, 1.2f));
	graphics.DrawPath(&borderPen, &boxPath);

	// 
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
// CPortToggleButton 
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

	// ¥è 
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

	// Double buffering (toggle click/focus flicker )
	CDC memDC;
	memDC.CreateCompatibleDC(pDC);

	CBitmap memBmp;
	memBmp.CreateCompatibleBitmap(pDC, w, h);
	CBitmap* pOldBmp = memDC.SelectObject(&memBmp);

	// : underlay ÄØ,  ¥è  (  )
	COLORREF bg = m_bUseUnderlay
		? m_clrUnderlay
		: kftc_parent_bg_color(m_hWnd, memDC.GetSafeHdc());
	memDC.FillSolidRect(0, 0, w, h, bg);

	Gdiplus::Graphics graphics(memDC.GetSafeHdc());
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	//   (0,0)
	CRect rect(0, 0, w, h);

	//  /:  ,   (TOSS/Kakao )
	REAL switchW = ModernUIDpi::ScaleF(m_hWnd, 44.0f);
	REAL switchH = ModernUIDpi::ScaleF(m_hWnd, 24.0f);
	REAL marginR = (float)ModernUIDpi::Scale(m_hWnd, 2);
	REAL switchX = (REAL)rect.Width() - switchW - marginR;
	REAL switchY = (rect.Height() - switchH) / 2.0f;

	// keep stroke inside (avoid right-edge clipping)
	Gdiplus::RectF switchRect(switchX + ModernUIDpi::ScaleF(m_hWnd, 0.5f), switchY + ModernUIDpi::ScaleF(m_hWnd, 0.5f), switchW - ModernUIDpi::ScaleF(m_hWnd, 1.0f), switchH - ModernUIDpi::ScaleF(m_hWnd, 1.0f));

	// ()
	Gdiplus::GraphicsPath switchPath;
	AddRoundRect(switchPath, switchRect, switchH / 2);

	if (m_bToggled)
	{
		// ON:   /      
		Gdiplus::Color c1, c2;
		if (m_bHover)
		{
			c1 = Gdiplus::Color(255, 20, 118, 245);  // :   
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
		// OFF:   /     
		Gdiplus::Color offColor = m_bHover
			? Gdiplus::Color(255, 208, 218, 232)   // :   
			: Gdiplus::Color(255, 230, 236, 245);   // 
		Gdiplus::SolidBrush trackBrush(offColor);
		graphics.FillPath(&trackBrush, &switchPath);
	}

	//    ¥è( ) :    

	// ():   1.5px 
	REAL knobSize = ModernUIDpi::ScaleF(m_hWnd, m_bHover ? 19.5f : 18.0f);
	REAL knobPadding = (switchRect.Height - knobSize) / 2.0f;
	REAL knobX = m_bToggled
		? (switchRect.X + switchRect.Width - knobSize - knobPadding)
		: (switchRect.X + knobPadding);
	REAL knobY = switchRect.Y + knobPadding;

	Gdiplus::RectF knobRect(knobX, knobY, knobSize, knobSize);

	//  (    )
	BYTE shadowAlpha = m_bHover ? 40 : 28;
	Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(shadowAlpha, 0, 0, 0));
	Gdiplus::RectF shadowRect(knobX + ModernUIDpi::ScaleF(m_hWnd, 0.5f),
		knobY + ModernUIDpi::ScaleF(m_hWnd, 0.8f),
		knobSize, knobSize);
	graphics.FillEllipse(&shadowBrush, shadowRect);

	//   (,    )
	Gdiplus::Color knobColor = m_bHover
		? Gdiplus::Color(255, 252, 252, 255)   // :    
		: Gdiplus::Color(255, 255, 255, 255);
	Gdiplus::SolidBrush knobBrush(knobColor);
	graphics.FillEllipse(&knobBrush, knobRect);

	// 
	CString strText;
	GetWindowText(strText);

	if (!strText.IsEmpty())
	{
		//     : GDI DrawText 
		// GDI+  GDI  Windows  
		//     .
		HFONT hFont = (HFONT)::SendMessage(m_hWnd, WM_GETFONT, 0, 0);
		if (!hFont) hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

		HFONT hOld = (HFONT)::SelectObject(memDC.GetSafeHdc(), hFont);
		//   RGB(100, 112, 132) 
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

	// ¥è BN_CLICKED 
	GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);

	CButton::OnLButtonUp(nFlags, point);
}


void CPortToggleButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rect = lpDrawItemStruct->rcItem;

	//     : ¥è  
	pDC->FillSolidRect(&rect, kftc_parent_bg_color(m_hWnd, pDC->m_hDC));

	Gdiplus::Graphics graphics(pDC->m_hDC);
	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	//   
	REAL switchW = ModernUIDpi::ScaleF(m_hWnd, 44.0f);
	REAL switchH = ModernUIDpi::ScaleF(m_hWnd, 24.0f);
	REAL switchX = 0;
	REAL switchY = (rect.Height() - switchH) / 2.0f;

	Gdiplus::RectF switchRect(switchX, switchY, switchW, switchH);

	// 
	Gdiplus::GraphicsPath switchPath;
	AddRoundRect(switchPath, switchRect, switchH / 2);

	if (m_bToggled)
	{
		// ON  ()
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
		// OFF  ()
		Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, 184, 208, 238));
		graphics.FillPath(&bgBrush, &switchPath);
	}

	// ¥è
	Gdiplus::Color borderColor;
	if (m_bHover)
		borderColor = m_bToggled ? Gdiplus::Color(255, 0, 163, 224) : Gdiplus::Color(255, 160, 160, 160);
	else
		borderColor = m_bToggled ? Gdiplus::Color(255, 0, 118, 190) : Gdiplus::Color(100, 0, 0, 0);

	Gdiplus::Pen borderPen(borderColor, 1.5f);
	graphics.DrawPath(&borderPen, &switchPath);

	//  
	REAL knobSize = (float)ModernUIDpi::Scale(m_hWnd, 18);
	REAL knobPadding = ModernUIDpi::ScaleF(m_hWnd, 3.0f);
	REAL knobX = m_bToggled ? (switchRect.X + switchRect.Width - knobSize - knobPadding) : (switchRect.X + knobPadding);
	REAL knobY = switchRect.Y + (switchRect.Height - knobSize) / 2;

	Gdiplus::RectF knobRect(knobX, knobY, knobSize, knobSize);

	// 
	Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(30, 0, 0, 0));
	Gdiplus::RectF shadowRect(knobX + ModernUIDpi::ScaleF(m_hWnd, 0.5f), knobY + ModernUIDpi::ScaleF(m_hWnd, 0.5f), knobSize, knobSize);
	graphics.FillEllipse(&shadowBrush, shadowRect);

	// 
	Gdiplus::SolidBrush knobBrush(Gdiplus::Color(255, 255, 255, 255));
	graphics.FillEllipse(&knobBrush, knobRect);

	// 
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

	// item height: MoveWindow   border (2px)  
	CRect rc;
	GetClientRect(&rc);
	int selH = max(16, rc.Height() - 2);   //     (border 1px )
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
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

	// colors (modern style)
	const KFTCInputTheme& th = GetActiveInputTheme();
	const COLORREF crBorderN = th.borderN;
	const COLORREF crBorderH = th.borderH;
	const COLORREF crBorderF = th.borderF;

	//    :
// -  
// - UnderlayColor  (/  ©£  )
//      UnderlayColor  " "   .
	Gdiplus::Color bg(255, 255, 255, 255);
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
		Gdiplus::REAL sz = 4.0f; // V  

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

		Gdiplus::Pen arrPen(arrColor, 2.0f); //  ¥â 2.0
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

	// 1.           
	LRESULT lRes = oldProc ? ::CallWindowProc(oldProc, hWnd, msg, wp, lp) : 0;

	// 2.    ,  ¯C  ¥è 
	if (msg == WM_PAINT || msg == WM_NCPAINT)
	{
		HDC hdc = ::GetWindowDC(hWnd); //      
		if (hdc)
		{
			RECT rc;
			::GetWindowRect(hWnd, &rc);
			::OffsetRect(&rc, -rc.left, -rc.top); // (0,0)  

			//   ¬Þ    ( UI)
			COLORREF borderClr = RGB(228, 232, 240);
			HPEN hPen = ::CreatePen(PS_SOLID, 1, borderClr);
			HGDIOBJ hOldPen = ::SelectObject(hdc, hPen);

			// , ,  1   (     )
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

	// 1.    ¥è      (Win32 API )
	LONG_PTR style = ::GetWindowLongPtr(m_hList, GWL_STYLE);
	style &= ~WS_BORDER;
	::SetWindowLongPtr(m_hList, GWL_STYLE, style);

	LONG_PTR exStyle = ::GetWindowLongPtr(m_hList, GWL_EXSTYLE);
	exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	::SetWindowLongPtr(m_hList, GWL_EXSTYLE, exStyle);

	//      
	::SetWindowPos(m_hList, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

	// 2. /îí         
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

	//  UI: ¥å  
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
	//  ¢¯     ¥ï (14 -> 18)
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
// CSkinnedEdit (Edit - ComboBox  /¥è )
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
	const COLORREF bg = RGB(255, 255, 255);

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

	//  ¥è : underlay  ¥è 
	if (m_bUseUnderlayBg)
		memDC.FillSolidRect(&rc, m_clrUnderlayBg);
	else
		kftc_fill_parent_bg(m_hWnd, memDC.GetSafeHdc(), rc);

	Gdiplus::Graphics g(memDC.m_hDC);
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

	const KFTCInputTheme& th = GetActiveInputTheme();
	const COLORREF crBorderN = th.borderN;
	const COLORREF crBorderH = th.borderH;
	const COLORREF crBorderF = th.borderF;

	//    :   ( rect )
	Gdiplus::Color bg(255, 255, 255, 255);
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
// CModernTabCtrl 
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
//  / 
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
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);

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

	Gdiplus::Pen pen(iconColor, 1.6f);
	pen.SetStartCap(Gdiplus::LineCapRound);
	pen.SetEndCap(Gdiplus::LineCapRound);
	pen.SetLineJoin(Gdiplus::LineJoinRound);

	const Gdiplus::REAL hw = sz * 0.44f;

	switch (iconType)
	{
	case 0: // Credit card with filled magnetic stripe - payment settings
	{
		REAL cw = hw * 2.0f, ch = hw * 1.25f;
		REAL x0 = cx - hw, y0 = cy - ch * 0.5f;
		// Filled magnetic stripe band first (so card outline covers the edges cleanly)
		REAL s1 = y0 + ch * 0.25f;
		REAL sH = ch * 0.27f;
		Gdiplus::SolidBrush stripeBrush(iconColor);
		g.FillRectangle(&stripeBrush, x0, s1, cw, sH);
		// Card outline drawn on top (rounded corners frame the stripe)
		Gdiplus::GraphicsPath cardPath;
		MakeRoundRect(cardPath, Gdiplus::RectF(x0, y0, cw, ch), sz * 0.14f);
		g.DrawPath(&pen, &cardPath);
		// Small signature line (bottom right - like card signature panel)
		REAL sigX = x0 + cw * 0.52f;
		REAL sigY = y0 + ch * 0.72f;
		g.DrawLine(&pen, sigX, sigY, x0 + cw * 0.88f, sigY);
		break;
	}
	case 1: // IC chip with pins - device info
	{
		REAL cs = hw * 0.60f;  // half-size of chip body
		REAL pl = hw * 0.38f;  // pin length
		REAL po = cs * 0.45f;  // pin offset from center
		// Chip body (square with rounded corners)
		Gdiplus::GraphicsPath chipPath;
		MakeRoundRect(chipPath, Gdiplus::RectF(cx - cs, cy - cs, cs * 2.0f, cs * 2.0f), 1.8f);
		g.DrawPath(&pen, &chipPath);
		// Top pins
		g.DrawLine(&pen, cx - po, cy - cs, cx - po, cy - cs - pl);
		g.DrawLine(&pen, cx + po, cy - cs, cx + po, cy - cs - pl);
		// Bottom pins
		g.DrawLine(&pen, cx - po, cy + cs, cx - po, cy + cs + pl);
		g.DrawLine(&pen, cx + po, cy + cs, cx + po, cy + cs + pl);
		// Left pins
		g.DrawLine(&pen, cx - cs, cy - po, cx - cs - pl, cy - po);
		g.DrawLine(&pen, cx - cs, cy + po, cx - cs - pl, cy + po);
		// Right pins
		g.DrawLine(&pen, cx + cs, cy - po, cx + cs + pl, cy - po);
		g.DrawLine(&pen, cx + cs, cy + po, cx + cs + pl, cy + po);
		break;
	}
	case 2: // Equalizer sliders - system settings
	{
		REAL lL = cx - hw, lR = cx + hw;
		REAL y1 = cy - hw * 0.65f;
		REAL y2 = cy;
		REAL y3 = cy + hw * 0.65f;
		REAL kr  = 2.1f; // knob radius
		// Knob x positions (staggered for visual variety)
		REAL k1x = cx - hw * 0.18f;
		REAL k2x = cx + hw * 0.32f;
		REAL k3x = cx - hw * 0.32f;
		// Top line + knob
		g.DrawLine(&pen, lL, y1, k1x - kr, y1);
		g.DrawLine(&pen, k1x + kr, y1, lR, y1);
		g.DrawEllipse(&pen, k1x - kr, y1 - kr, kr * 2.0f, kr * 2.0f);
		// Mid line + knob
		g.DrawLine(&pen, lL, y2, k2x - kr, y2);
		g.DrawLine(&pen, k2x + kr, y2, lR, y2);
		g.DrawEllipse(&pen, k2x - kr, y2 - kr, kr * 2.0f, kr * 2.0f);
		// Bottom line + knob
		g.DrawLine(&pen, lL, y3, k3x - kr, y3);
		g.DrawLine(&pen, k3x + kr, y3, lR, y3);
		g.DrawEllipse(&pen, k3x - kr, y3 - kr, kr * 2.0f, kr * 2.0f);
		break;
	}
	case 3: // Download arrow - merchant download
	{
		REAL shaftTop = cy - hw * 0.85f;
		REAL shaftBot = cy;
		REAL awW     = hw * 0.65f;
		REAL awH     = hw * 0.55f;
		REAL baseLine = cy + hw * 0.85f;
		// Vertical shaft
		g.DrawLine(&pen, cx, shaftTop, cx, shaftBot);
		// Chevron arrowhead (V pointing down)
		g.DrawLine(&pen, cx - awW, shaftBot, cx, shaftBot + awH);
		g.DrawLine(&pen, cx, shaftBot + awH, cx + awW, shaftBot);
		// Base tray line
		g.DrawLine(&pen, cx - hw, baseLine, cx + hw, baseLine);
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
	//  : OnPaint   
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

	// font:  ¥è   Bold  
	CFont* pOldFont = nullptr;
	CFont  boldFont;
	if (m_bBold)
	{
		CFont* pFont = GetFont();
		LOGFONT lf{};
		if (pFont && pFont->GetLogFont(&lf))
		{
			lf.lfWeight = FW_SEMIBOLD; //  ¥â 
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
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CInfoIconButton::CInfoIconButton()
	: m_bHover(FALSE), m_bTracking(FALSE)
	, m_bUseUnderlay(FALSE), m_clrUnderlay(RGB(249, 250, 252))
{
}

void CInfoIconButton::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bTracking)
	{
		TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
		tme.dwFlags = TME_LEAVE | TME_HOVER;
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


BOOL CInfoIconButton::OnEraseBkgnd(CDC* pDC)
{
	// Prevent background erase to reduce flicker (we fully paint in DrawItem)
	UNREFERENCED_PARAMETER(pDC);
	return TRUE;
}

void CInfoIconButton::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	ModernUIGfx::EnsureGdiplusStartup();

	CRect rect(lpDIS->rcItem);

	// Double-buffered drawing to prevent hover/click flicker.
	CDC dcMem;
	dcMem.CreateCompatibleDC(CDC::FromHandle(lpDIS->hDC));

	CBitmap bmp;
	bmp.CreateCompatibleBitmap(CDC::FromHandle(lpDIS->hDC), rect.Width(), rect.Height());
	CBitmap* pOldBmp = dcMem.SelectObject(&bmp);

	// Background
	COLORREF crBg = m_bUseUnderlay ? m_clrUnderlay
		: kftc_parent_bg_color(m_hWnd, dcMem.GetSafeHdc());
	dcMem.FillSolidRect(0, 0, rect.Width(), rect.Height(), crBg);

	Gdiplus::Graphics g(dcMem.GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

	float sz = (float)min(rect.Width(), rect.Height()) - 2.0f;
	float ox = (rect.Width() - sz) * 0.5f;
	float oy = (rect.Height() - sz) * 0.5f;
	Gdiplus::RectF rf(ox, oy, sz, sz);

	// Keep hover/pressed visuals very close to avoid "blink" on click.
	const BOOL bPressed = (lpDIS->itemState & ODS_SELECTED) ? TRUE : FALSE;
	const BOOL bHot = (m_bHover || bPressed);

	
	// Press feedback without color \"blink\": nudge the icon by 1px when pressed.
	const int nPressOffset = bPressed ? 1 : 0;
	if (nPressOffset)
	{
		ox += (float)nPressOffset;
		oy += (float)nPressOffset;
		rf.X += (float)nPressOffset;
		rf.Y += (float)nPressOffset;
	}
COLORREF fill = bHot ? KFTC_PRIMARY : RGB(232, 240, 254); // #E8F0FE
	COLORREF txt  = bHot ? RGB(255, 255, 255) : KFTC_PRIMARY;

	Gdiplus::SolidBrush brFill(Gdiplus::Color(255,
		GetRValue(fill), GetGValue(fill), GetBValue(fill)));
	g.FillEllipse(&brFill, rf);

	
	// Subtle outline on press to give a clear click affordance.
	if (bPressed)
	{
		Gdiplus::Pen pen(Gdiplus::Color(90, 0, 0, 0), 1.0f);
		g.DrawEllipse(&pen, rf);
	}
Gdiplus::FontFamily ff(L"Malgun Gothic");
	Gdiplus::Font font(&ff, sz * 0.52f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	Gdiplus::SolidBrush brText(Gdiplus::Color(255,
		GetRValue(txt), GetGValue(txt), GetBValue(txt)));
	Gdiplus::StringFormat sf;
	sf.SetAlignment(Gdiplus::StringAlignmentCenter);
	sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
	g.DrawString(L"?", -1, &font, rf, &sf, &brText);

	// Blit to screen
	::BitBlt(lpDIS->hDC, rect.left, rect.top, rect.Width(), rect.Height(),
		dcMem.GetSafeHdc(), 0, 0, SRCCOPY);

	dcMem.SelectObject(pOldBmp);
}

// ============================================================================
// CModernPopover - floating info popover
// ============================================================================

BEGIN_MESSAGE_MAP(CModernPopover, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

HHOOK           CModernPopover::s_hMouseHook = NULL;
CModernPopover* CModernPopover::s_pPopoverInst = NULL;

CModernPopover::CModernPopover()
	: m_nArrowX(0), m_nCardW(0), m_nCardH(0), m_bVisible(FALSE)
{
}

/*static*/ void CModernPopover::RegisterPopoverClass()
{
	static bool s_registered = false;
	if (s_registered) return;
	s_registered = true;

	WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
	wc.style = CS_HREDRAW | CS_VREDRAW; // CS_DROPSHADOW :   ( ¥è) 
	wc.lpfnWndProc = ::DefWindowProc;
	wc.hInstance = AfxGetInstanceHandle();
	wc.hbrBackground = (HBRUSH)::GetStockObject(NULL_BRUSH); //   (  )
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
	path.AddArc(a, 0, 90); a.X = r.X;
	path.AddArc(a, 90, 90); path.CloseFigure();
}


void CModernPopover::AddPopoverPath(Gdiplus::GraphicsPath& path, const Gdiplus::RectF& cardRc,
	REAL radius, float arrowX, float arrowTipY, float arrowHalfW)
{
	// Build a SINGLE closed outline (card + arrow) to avoid seam lines.
	const float left   = cardRc.X;
	const float top    = cardRc.Y;
	const float right  = cardRc.X + cardRc.Width;
	const float bottom = cardRc.Y + cardRc.Height;

	const float r = (float)radius;
	const float d = r * 2.0f;

	// Clamp arrow base within rounded corners
	float arrowL = arrowX - arrowHalfW;
	float arrowR = arrowX + arrowHalfW;
	const float minX = left + r + 2.0f;
	const float maxX = right - r - 2.0f;
	if (arrowL < minX) { arrowL = minX; arrowR = arrowL + arrowHalfW * 2.0f; }
	if (arrowR > maxX) { arrowR = maxX; arrowL = arrowR - arrowHalfW * 2.0f; }

	path.Reset();
	path.StartFigure();

	// Start at top-left (after rounding)
	path.AddLine(left + r, top, arrowL, top);
	path.AddLine(arrowL, top, arrowX, arrowTipY);
	path.AddLine(arrowX, arrowTipY, arrowR, top);
	path.AddLine(arrowR, top, right - r, top);

	// Top-right corner
	path.AddArc(right - d, top, d, d, 270.0f, 90.0f);
	// Right edge
	path.AddLine(right, top + r, right, bottom - r);
	// Bottom-right corner
	path.AddArc(right - d, bottom - d, d, d, 0.0f, 90.0f);
	// Bottom edge
	path.AddLine(right - r, bottom, left + r, bottom);
	// Bottom-left corner
	path.AddArc(left, bottom - d, d, d, 90.0f, 90.0f);
	// Left edge
	path.AddLine(left, bottom - r, left, top + r);
	// Top-left corner
	path.AddArc(left, top, d, d, 180.0f, 90.0f);

	path.CloseFigure();
}




static int MeasurePopoverTextHeight(HWND hRef, const CString& title, const CString& body, int innerW)
{
	// Measure with the SAME font stack / sizes as the popover drawing (GDI+).
	// This prevents "last line clipped" issues caused by mismatched measurement vs render fonts.
	if (!hRef || innerW <= 0) return 0;

	HDC hdc = ::GetDC(hRef);
	if (!hdc) return 0;

	ModernUIGfx::EnsureGdiplusStartup();
	Gdiplus::Graphics g(hdc);
	g.SetPageUnit(Gdiplus::UnitPixel);

	Gdiplus::FontFamily ff(L"Malgun Gothic");
	Gdiplus::Font fTitle(&ff, (Gdiplus::REAL)ModernUIDpi::Scale(hRef, 13), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	Gdiplus::Font fBody(&ff, (Gdiplus::REAL)ModernUIDpi::Scale(hRef, 12), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

	Gdiplus::StringFormat sfTitle;
	sfTitle.SetAlignment(Gdiplus::StringAlignmentNear);
	sfTitle.SetLineAlignment(Gdiplus::StringAlignmentNear);
	sfTitle.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
	sfTitle.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);

	Gdiplus::StringFormat sfBody;
	sfBody.SetAlignment(Gdiplus::StringAlignmentNear);
	sfBody.SetLineAlignment(Gdiplus::StringAlignmentNear);
	sfBody.SetTrimming(Gdiplus::StringTrimmingWord);
	// allow wrapping (default). Do NOT set NoWrap here.

	auto MeasureWrapped = [&](const CString& s, Gdiplus::Font* f, Gdiplus::StringFormat* fmt)->int
	{
		if (s.IsEmpty()) return 0;
		std::wstring ws = kftc_to_wide(s);

		Gdiplus::RectF rc(0, 0, (Gdiplus::REAL)innerW, 4096.0f);
		Gdiplus::RectF bound;
		g.MeasureString(ws.c_str(), -1, f, rc, fmt, &bound);

		// GDI+ MeasureString tends to under-report height vs DrawString, especially with AA.
		// Add a small DPI-scaled slack so the last line doesn't look clipped.
		int h = (int)::ceil(bound.Height) + ModernUIDpi::Scale(hRef, 6);
		return h;
	};

	int hTitle = MeasureWrapped(title, &fTitle, &sfTitle);
	int hBody  = MeasureWrapped(body,  &fBody,  &sfBody);
	// Extra leading between explicit lines (\r\n / \n) to match modern mobile UI rhythm.
	{
		CString norm = body;

		// Normalize line endings: CRLF -> LF
		norm.Replace(_T("\r\n"), _T("\n"));

		int lines = 1;
		for (int i = 0; i < norm.GetLength(); ++i)
			if (norm[i] == _T('\n')) ++lines;

		if (lines > 1)
			hBody += ModernUIDpi::Scale(hRef, 2) * (lines - 1);
	}

	::ReleaseDC(hRef, hdc);
	return hTitle + hBody;
}

void CModernPopover::ShowAt(const CRect& anchorScrRc, LPCTSTR title,
	LPCTSTR body, CWnd* pParent)
{
	m_strTitle = title;
	m_strBody = body;

	RegisterPopoverClass();

	if (!GetSafeHwnd())
	{
		CreateEx(WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED,
			_T("KFTCModernPopover"), _T(""),
			WS_POPUP,
			0, 0, kPopW + 2 * kShadowPad, kPopMinH + kArrowH + kShadowPad,
			pParent ? pParent->GetSafeHwnd() : NULL, NULL);
	}

	HWND hRef = pParent ? pParent->GetSafeHwnd() : m_hWnd;

	int shadowPad = ModernUIDpi::Scale(hRef, kShadowPad);
	
	// ---- Auto size width (Toss-like) ----
	// Base padding inside the white card (must match drawing paddings)
	const int padL = ModernUIDpi::Scale(hRef, 16);
	const int padR = ModernUIDpi::Scale(hRef, 16);
	
	// Measure "ideal" single-line width for title/body using the same font stack as drawing.
	int idealTextW = 0;
	{
		HDC hdc = ::GetDC(hRef);
		if (hdc)
		{
			ModernUIGfx::EnsureGdiplusStartup();
			Gdiplus::Graphics gg(hdc);
			gg.SetPageUnit(Gdiplus::UnitPixel);
	
			Gdiplus::FontFamily ff(L"Malgun Gothic");
			Gdiplus::Font fTitle(&ff, (Gdiplus::REAL)ModernUIDpi::Scale(hRef, 13), Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
			Gdiplus::Font fBody(&ff,  (Gdiplus::REAL)ModernUIDpi::Scale(hRef, 12), Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
	
			Gdiplus::StringFormat sfNoWrap;
			sfNoWrap.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
			sfNoWrap.SetTrimming(Gdiplus::StringTrimmingNone);
	
			auto MeasureOneLineW = [&](const CString& s, Gdiplus::Font* f)->int
			{
				if (s.IsEmpty()) return 0;
				std::wstring ws = kftc_to_wide(s);
				Gdiplus::RectF bound;
				gg.MeasureString(ws.c_str(), -1, f, Gdiplus::PointF(0, 0), &sfNoWrap, &bound);
				return (int)::ceil(bound.Width);
			};
	
			idealTextW = max(MeasureOneLineW(m_strTitle, &fTitle), MeasureOneLineW(m_strBody, &fBody));
			::ReleaseDC(hRef, hdc);
		}
	}
	
	// Clamp width to a nice range so it doesn't get too narrow/wide.
	const int minCardW = ModernUIDpi::Scale(hRef, 210);
	const int maxCardW = ModernUIDpi::Scale(hRef, 340);
	int cardW = idealTextW > 0 ? (idealTextW + padL + padR) : ModernUIDpi::Scale(hRef, kPopW);
	cardW = max(minCardW, min(cardW, maxCardW));
	
	int popW = cardW + 2 * shadowPad;
	
	// ---- Auto size height (Toss-like) ----
	// Layout paddings (inside the white card)
	const int padT = ModernUIDpi::Scale(hRef, 14);
	const int padB = ModernUIDpi::Scale(hRef, 14);
	const int gapTB = ModernUIDpi::Scale(hRef, 8);

	int innerW = cardW - padL - padR;
	int textH = MeasurePopoverTextHeight(hRef, m_strTitle, m_strBody, innerW);

	// Title/body are stacked with a small gap when both exist
	if (!m_strTitle.IsEmpty() && !m_strBody.IsEmpty())
		textH += gapTB;

	m_nCardW = cardW;
	m_nCardH = max(ModernUIDpi::Scale(hRef, kPopMinH), padT + textH + padB);

	// Total window height must include shadow padding, arrow, and a bit of bottom room for shadow offset.
	int popH = shadowPad + ModernUIDpi::Scale(hRef, kArrowH) + m_nCardH + shadowPad + ModernUIDpi::Scale(hRef, kShadowOffY);

	// Center the visible card (not the full window) on the anchor icon
	int px = anchorScrRc.CenterPoint().x - shadowPad - cardW / 2;
	const int gapToAnchor = ModernUIDpi::Scale(hRef, 4); //   (îí )
	int py = anchorScrRc.bottom - shadowPad + gapToAnchor;

	int screenH = ::GetSystemMetrics(SM_CYSCREEN);
	if (py + popH > screenH - 10)
		py = anchorScrRc.top - popH + shadowPad - gapToAnchor;

	m_nArrowX = (anchorScrRc.left + (anchorScrRc.Width() / 2)) - px - ModernUIDpi::Scale(hRef, 1);
	if (m_nArrowX < shadowPad + 14) m_nArrowX = shadowPad + 14;
	if (m_nArrowX > shadowPad + cardW - 14) m_nArrowX = shadowPad + cardW - 14;

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
		s_hMouseHook = NULL;
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
	HDC hdcMem = ::CreateCompatibleDC(hdcScreen);

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = W;
	bmi.bmiHeader.biHeight = -H;   // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	BYTE* pvBits = NULL;
	HBITMAP hBmp = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS,
		(void**)&pvBits, NULL, 0);
	if (!hBmp) { ::DeleteDC(hdcMem); ::ReleaseDC(NULL, hdcScreen); return; }

	HBITMAP hOldBmp = (HBITMAP)::SelectObject(hdcMem, hBmp);
	::ZeroMemory(pvBits, W * H * 4);   // fully transparent initially

	ModernUIGfx::EnsureGdiplusStartup();

	{
		// GDI+ bitmap wraps the DIB (BGRA in memory == PixelFormat32bppARGB)
		Gdiplus::Bitmap bmpGdi(W, H, W * 4, PixelFormat32bppARGB, pvBits);
		Gdiplus::Graphics g(&bmpGdi);
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeNone);
		g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
		// Text crispness: snap to integer pixels to avoid half-pixel blur on layered windows.
		auto SnapF = [](Gdiplus::REAL v) { return (Gdiplus::REAL)((int)::floor(v + 0.5f)); };

		const int   arrowH = ModernUIDpi::Scale(m_hWnd, kArrowH);
		const int   arrowHW = ModernUIDpi::Scale(m_hWnd, 9);
		const int   shadowPad = ModernUIDpi::Scale(m_hWnd, kShadowPad);
		const int   shadowOffY = ModernUIDpi::Scale(m_hWnd, kShadowOffY);
		const float radius = ModernUIDpi::ScaleF(m_hWnd, 14.0f);

		// Card rect (accounts for shadow padding around window)
		const float cardX = (float)shadowPad;
		const float cardY = (float)(shadowPad + arrowH);
		const float cardW = (float)(W - 2 * shadowPad);
		const float cardH = (float)m_nCardH;
		Gdiplus::RectF cardRc(cardX, cardY, cardW, cardH);

		// -- Build popover shape (card + arrow) for shadow/body --
		Gdiplus::GraphicsPath popPath;
		AddPopoverPath(popPath, cardRc, radius, (float)m_nArrowX, (float)shadowPad, (float)arrowHW);

// -- Shadow (soft, Toss-like) --
		Gdiplus::GraphicsPath shadowPath;
		shadowPath.AddPath(&popPath, FALSE);
		Gdiplus::Matrix mx;
		mx.Translate(0.0f, (Gdiplus::REAL)shadowOffY);
		shadowPath.Transform(&mx);

		// fill a base shadow + multiple strokes to fake blur
		{
			Gdiplus::SolidBrush brShadowFill(Gdiplus::Color(38, 0, 0, 0));
			g.FillPath(&brShadowFill, &shadowPath);

			const int widths[] = { 22, 16, 12, 8, 4 };
			const int alphas[] = { 8, 10, 12, 16, 20 };
			for (int i = 0; i < 5; ++i)
			{
				Gdiplus::Pen pen(Gdiplus::Color(alphas[i], 0, 0, 0), (Gdiplus::REAL)ModernUIDpi::ScaleF(m_hWnd, (float)widths[i]));
				pen.SetLineJoin(Gdiplus::LineJoinRound);
				g.DrawPath(&pen, &shadowPath);
			}
		}

		// -- White card body (no border) --
		Gdiplus::SolidBrush brBody(Gdiplus::Color(255, 255, 255, 255));
		g.FillPath(&brBody, &popPath);

		// -- Title/body text (spacing similar to Toss) --
		const float padX = ModernUIDpi::ScaleF(m_hWnd, 16.0f);
		const float padTop = ModernUIDpi::ScaleF(m_hWnd, 14.0f);
		const float padBottom = ModernUIDpi::ScaleF(m_hWnd, 14.0f);
		const float gapTB = ModernUIDpi::ScaleF(m_hWnd, 8.0f);

		Gdiplus::FontFamily ff(L"Malgun Gothic");
		Gdiplus::Font fTitle(&ff, (Gdiplus::REAL)ModernUIDpi::Scale(m_hWnd, 13),
			Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
		Gdiplus::SolidBrush brTitle(Gdiplus::Color(255, 35, 45, 60));

		Gdiplus::StringFormat sfTitle;
		sfTitle.SetAlignment(Gdiplus::StringAlignmentNear);
		sfTitle.SetLineAlignment(Gdiplus::StringAlignmentNear);
		sfTitle.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
		sfTitle.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);

		const float textW = cardW - padX * 2.0f;
		float curY = cardY + padTop;

		// Title (single line)
		Gdiplus::RectF titleRc(cardX + padX, curY, textW, 0.0f);
		if (!m_strTitle.IsEmpty())
		{
			std::wstring wTitle = kftc_to_wide(m_strTitle);

			// Measure height precisely to avoid clipping.
			Gdiplus::RectF bound;
			g.MeasureString(wTitle.c_str(), -1, &fTitle,
				Gdiplus::RectF(titleRc.X, titleRc.Y, titleRc.Width, 256.0f),
				&sfTitle, &bound);

			float titleH = (float)((int)::ceil(bound.Height)) + (float)ModernUIDpi::Scale(m_hWnd, 2);
			titleRc.Height = max(titleH, (float)ModernUIDpi::Scale(m_hWnd, 18));

			titleRc.X = SnapF(titleRc.X);
			titleRc.Y = SnapF(titleRc.Y);
			g.DrawString(wTitle.c_str(), -1, &fTitle, titleRc, &sfTitle, &brTitle);
			curY += titleRc.Height;

			if (!m_strBody.IsEmpty())
				curY += gapTB;
		}

		// Body top/height are computed from the real card height.
		float bodyTop = curY;
		float bodyH = (cardY + cardH) - padBottom - bodyTop;
		if (bodyH < 0) bodyH = 0;

		Gdiplus::Font fBody(&ff, (Gdiplus::REAL)ModernUIDpi::Scale(m_hWnd, 12),
			Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
		Gdiplus::SolidBrush brBodyText(Gdiplus::Color(255, 92, 102, 118));
		Gdiplus::StringFormat sfBody;
		sfBody.SetAlignment(Gdiplus::StringAlignmentNear);
		sfBody.SetLineAlignment(Gdiplus::StringAlignmentNear);

		Gdiplus::RectF bodyRc(cardX + padX, bodyTop,
			cardW - padX * 2.0f, bodyH);
		bodyRc.X = SnapF(bodyRc.X);
		bodyRc.Y = SnapF(bodyRc.Y);

		// Draw body with a little extra leading between explicit lines to avoid a cramped look.
		CString normBody = m_strBody;
		normBody.Replace(_T("\r\n"), _T("\n"));

		const float extraLead = (float)ModernUIDpi::Scale(m_hWnd, 2);
		float y = bodyRc.Y;

		Gdiplus::StringFormat sfBodyLine;
		sfBodyLine.SetAlignment(sfBody.GetAlignment());
		sfBodyLine.SetLineAlignment(sfBody.GetLineAlignment());
		sfBodyLine.SetFormatFlags(sfBody.GetFormatFlags());
		sfBodyLine.SetTrimming(sfBody.GetTrimming());
		sfBodyLine.SetHotkeyPrefix(sfBody.GetHotkeyPrefix());
		//sfBodyLine.SetTabStops(0, nullptr);


		int start = 0;
		while (start <= normBody.GetLength())
		{
			int nl = normBody.Find(_T('\n'), start);
			CString line = (nl >= 0) ? normBody.Mid(start, nl - start) : normBody.Mid(start);
			start = (nl >= 0) ? (nl + 1) : (normBody.GetLength() + 1);

			std::wstring wLine = kftc_to_wide(line);

			// Measure this line block (it may wrap if long).
			Gdiplus::RectF bound;
			g.MeasureString(wLine.c_str(), -1, &fBody,
				Gdiplus::RectF(bodyRc.X, y, bodyRc.Width, 4096.0f),
				&sfBodyLine, &bound);

			float hLine = (float)((int)::ceil(bound.Height));
			if (hLine < (float)ModernUIDpi::Scale(m_hWnd, 16))
				hLine = (float)ModernUIDpi::Scale(m_hWnd, 16);

			Gdiplus::RectF lineRc(bodyRc.X, y, bodyRc.Width, hLine);
			lineRc.X = SnapF(lineRc.X);
			lineRc.Y = SnapF(lineRc.Y);

			g.DrawString(wLine.c_str(), -1, &fBody, lineRc, &sfBodyLine, &brBodyText);

			y += hLine + extraLead;

			if (y > bodyRc.Y + bodyRc.Height + 1.0f)
				break;
		}
	}

	// Premultiply alpha (required for ULW_ALPHA / AC_SRC_ALPHA)
	for (int i = 0; i < W * H; ++i)
	{
		BYTE a = pvBits[i * 4 + 3];
		if (a > 0 && a < 255)
		{
			pvBits[i * 4 + 0] = static_cast<BYTE>(pvBits[i * 4 + 0] * a / 255);
			pvBits[i * 4 + 1] = static_cast<BYTE>(pvBits[i * 4 + 1] * a / 255);
			pvBits[i * 4 + 2] = static_cast<BYTE>(pvBits[i * 4 + 2] * a / 255);
		}
	}

	POINT          ptDst = { rcWin.left, rcWin.top };
	SIZE           szWnd = { W, H };
	POINT          ptSrc = { 0, 0 };
	BLENDFUNCTION  bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
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