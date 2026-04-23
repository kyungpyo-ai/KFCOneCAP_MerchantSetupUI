// ReaderSetupDlg.cpp : implementation file
//

#include "stdafx.h"
#include "common.h"
#include "resource.h"
#include "ReaderSetupDlg.h"

#include "ModernMessageBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include <vector>

using namespace std;

namespace
{
	// 무결성 테이블 컬럼 비율은 리스트 컬럼 폭 계산과 커스텀 헤더/본문 그리기에서
	// 같은 값을 공유해야 한다. 한 곳에서만 관리해 헤더/본문 어긋남을 방지한다.
	static const int kIntegrityColumnRatios[] = { 20, 11, 8, 18, 23, 20 };
	static const int kIntegrityColumnCount = sizeof(kIntegrityColumnRatios) / sizeof(kIntegrityColumnRatios[0]);

	// Reader 카드 액션 버튼은 폭이 제각각이면 정렬감이 무너지므로
	// 레이아웃/로딩 상태 모두 동일한 폭을 사용한다.
	static const int kReaderActionButtonWidth = 100;

	static BOOL IsSamePortLabel(const CString& left, const CString& right)
	{
		CString a(left), b(right);
		a.Trim();
		b.Trim();
		return a.Compare(b) == 0;
	}

	static void ApplyFontIfWindow(CWnd& wnd, CFont& font)
	{
		if (::IsWindow(wnd.GetSafeHwnd()))
			wnd.SetFont(&font);
	}
}

static HISTORY_COL_INFO col_info[] =
{
	{"체크일시"			, LVCFMT_CENTER	, 14 },
	{"포트"			, LVCFMT_CENTER	, 11 },
	{"결과"			,	 LVCFMT_CENTER	, 8 },
	{"모듈ID"			, LVCFMT_CENTER	, 13 },
	{"리더기식별번호"	, LVCFMT_CENTER	, 19 },
	{"POS식별번호"		, LVCFMT_CENTER	, 17 },
	{NULL			, NULL			, 0	}
};

// Thin forwarding wrapper - delegates to shared ModernUIGfx::AddRoundRect
static void GdipAddRoundRect(Gdiplus::GraphicsPath& p, float x, float y, float w, float h, float r)
{
	Gdiplus::RectF rect(x, y, w, h);
	ModernUIGfx::AddRoundRect(p, rect, r);
}

static void FillRoundRect(Gdiplus::Graphics& g, const CRect& rc, int radius, COLORREF fill, COLORREF border, int borderW = 1)
{
	float x = (float)rc.left, y = (float)rc.top;
	float w = (float)rc.Width(), h = (float)rc.Height();
	float r = (float)radius;

	Gdiplus::GraphicsPath path;
	Gdiplus::RectF rect(x, y, w, h);
	ModernUIGfx::AddRoundRect(path, rect, r);

	Gdiplus::SolidBrush br(Gdiplus::Color(255, GetRValue(fill), GetGValue(fill), GetBValue(fill)));
	g.FillPath(&br, &path);

	if (borderW > 0)
	{
		Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(border), GetGValue(border), GetBValue(border)), (float)borderW);
		pen.SetAlignment(Gdiplus::PenAlignmentInset);
		g.DrawPath(&pen, &path);
	}
}

static void GdipAddTopRoundRect(Gdiplus::GraphicsPath& p, float x, float y, float w, float h, float r)
{
	float d = r * 2.f;
	p.StartFigure();
	p.AddArc(x, y, d, d, 180.f, 90.f);
	p.AddLine(x + r, y, x + w - r, y);
	p.AddArc(x + w - d, y, d, d, 270.f, 90.f);
	p.AddLine(x + w, y + r, x + w, y + h);
	p.AddLine(x + w, y + h, x, y + h);
	p.AddLine(x, y + h, x, y + r);
	p.CloseFigure();
}

static void FillTopRoundRect(Gdiplus::Graphics& g, const CRect& rc, int radius, COLORREF fill)
{
	Gdiplus::GraphicsPath path;
	GdipAddTopRoundRect(path, (float)rc.left, (float)rc.top, (float)rc.Width(), (float)rc.Height(), (float)radius);
	Gdiplus::SolidBrush br(Gdiplus::Color(255, GetRValue(fill), GetGValue(fill), GetBValue(fill)));
	g.FillPath(&br, &path);
}

static void DrawRoundRectBorder(Gdiplus::Graphics& g, const CRect& rc, int radius, COLORREF border, int borderW = 1)
{
	if (borderW <= 0)
		return;
	Gdiplus::GraphicsPath path;
	float bw = (float)borderW;
	Gdiplus::RectF rect2((float)rc.left + bw * 0.5f, (float)rc.top + bw * 0.5f, (float)rc.Width() - bw, (float)rc.Height() - bw);
	ModernUIGfx::AddRoundRect(path, rect2, (float)radius);
	Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(border), GetGValue(border), GetBValue(border)), bw);
	g.DrawPath(&pen, &path);
}
int CReaderSetupDlg::SX(int v) const
{
	return ModernUIDpi::Scale(m_hWnd, v);
}

static BOOL IsCompactScreen() { return ::GetSystemMetrics(SM_CYSCREEN) <= 800; }

void CReaderSetupDlg::EnsureFonts()
{
	if (m_fontTitle.GetSafeHandle())
		return;

	LOGFONT lf = { 0 };
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);

	// [FIX] 폰트 페이스를 먼저 일괄 적용 (Pretendard 등)
	ModernUIFont::ApplyUIFontFace(lf);
	const BOOL bCF = IsCompactScreen();

	// Title (메인 타이틀)
	lf.lfHeight = -SX(bCF ? 15 : 18);
	lf.lfWeight = FW_BOLD;
	m_fontTitle.CreateFontIndirect(&lf);

	// Sub (부제목 - ShopSetupDlg처럼 강조)
	lf.lfHeight = -SX(bCF ? 11 : 13);
	lf.lfWeight = FW_BOLD; // FW_NORMAL -> FW_BOLD
	m_fontSub.CreateFontIndirect(&lf);

	// Section title (섹션 제목 - 포트 설정, 무결성 등)
	lf.lfHeight = -SX(bCF ? 13 : 15);
	lf.lfWeight = FW_BOLD;
	lf.lfQuality = CLEARTYPE_QUALITY; // [FIX] 카드 제목에도 ClearType 안티앨리어싱을 강제 적용하여 완벽하게 부드럽게 렌더링
	m_fontSection.CreateFontIndirect(&lf);

	// Normal (콤보박스, 버튼 등 본문)
	lf.lfHeight = -SX(bCF ? 12 : 14);
	lf.lfWeight = FW_MEDIUM; // FW_NORMAL -> FW_MEDIUM (약간 도톰하게)
	m_fontNormal.CreateFontIndirect(&lf);

	// Label (일반 라벨 및 토글 텍스트 - 깨짐 방지 완벽 세팅)
	lf.lfHeight = -SX(bCF ? 12 : 14);
	lf.lfWeight = FW_BOLD;                 // FW_NORMAL -> FW_BOLD
	lf.lfQuality = CLEARTYPE_QUALITY;      // [FIX] GDI 렌더링 품질 안정화 (ShopSetupDlg 비법)
	m_fontLabel.CreateFontIndirect(&lf);

	// Small (리스트 데이터 등)
	lf.lfHeight = -SX(bCF ? 10 : 12);
	lf.lfWeight = FW_NORMAL;
	m_fontSmall.CreateFontIndirect(&lf);

	ModernUIGfx::EnsureGdiplusStartup();
}

void CReaderSetupDlg::HideLegacyStatics()
{
	CWnd* pChild = GetWindow(GW_CHILD);
	while (pChild)
	{
		TCHAR cls[32] = { 0 };
		::GetClassName(pChild->GetSafeHwnd(), cls, 31);

		if (_tcscmp(cls, _T("Static")) == 0)
		{
			CString txt;
			pChild->GetWindowText(txt);
			txt.Trim();

			// 이것만 숨김 (제목은 숨기지 않음)
			if (txt == _T("리더기1") || txt == _T("리더기2") || txt == _T("조회 범위"))
			{
				pChild->ShowWindow(SW_HIDE);
			}
		}

		pChild = pChild->GetNextWindow();
	}
}

void CReaderSetupDlg::CalcLayoutRects(
	CRect& inner,
	CRect& card1, CRect& card2,
	CRect& infoTitleArea,
	CRect& queryBox,
	CRect& listRc,
	CRect& okRc, CRect& cancelRc,
	CPoint& sec1TitlePt,
	CPoint& sec2TitlePt
) const
{
	CRect rc; GetClientRect(&rc);
	const BOOL bC = IsCompactScreen();

	const int margin = SX(20);
	inner = CRect(rc.left + margin, rc.top + margin, rc.right - margin, rc.bottom - margin);

	const int titleBlock = SX(bC ? 62 : 80);
	const int sectionTitleH = SX(bC ? 24 : 28);
	const int sectionTitleTop = SX(bC ? 10 : 12);
	const int infoSectionTitleTop = SX(2);
	const int sectionTitleGap = SX(12);
	const int infoTitleGap = -SX(10);
	const int sectionBoxPad = SX(bC ? 16 : 20);
	const int cardH = SX(bC ? 102 : 128);
	const int cardGap = SX(bC ? 8 : 12);
	const int queryH = SX(bC ? 44 : 56);
	const int queryGap = SX(bC ? 12 : 16);
	const int infoBottomPad = SX(bC ? 14 : 8);
	const int bottomBtnH = SX(bC ? 36 : 42);
	const int bottomGap = SX(bC ? 12 : 16);
	const int bottomArea = bottomBtnH + bottomGap + SX(8);

	int y = inner.top + titleBlock;

	const int sectionLeft = inner.left + SX(18);
	const int sectionRight = inner.right - SX(18);
	const int cardLeft = sectionLeft + sectionBoxPad;
	const int cardRight = sectionRight - sectionBoxPad;

	sec1TitlePt = CPoint(sectionLeft + SX(24), y + sectionTitleTop);
	int sec1ContentTop = y + sectionTitleTop + sectionTitleH + sectionTitleGap;

	card1 = CRect(cardLeft, sec1ContentTop, cardRight, sec1ContentTop + cardH);
	card2 = CRect(cardLeft, card1.bottom + cardGap, cardRight, card1.bottom + cardGap + cardH);
	y = card2.bottom + sectionBoxPad + SX(bC ? 18 : 28);

	sec2TitlePt = CPoint(sectionLeft + SX(24), y + infoSectionTitleTop);
	int sec2ContentTop = y + sectionTitleTop + sectionTitleH + infoTitleGap;

	queryBox = CRect(sectionLeft + sectionBoxPad, sec2ContentTop,
		sectionRight - sectionBoxPad, sec2ContentTop + queryH);
	y = queryBox.bottom + queryGap;

	int listTop = y;
	int listBottomLimit = inner.bottom - bottomArea - infoBottomPad - sectionBoxPad;
	int listH = listBottomLimit - listTop;
	if (listH < SX(bC ? 138 : 166)) listH = SX(bC ? 138 : 166);
	if (listH > SX(bC ? 138 : 166)) listH = SX(bC ? 138 : 166);
	listRc = CRect(sectionLeft + sectionBoxPad, listTop,
		sectionRight - sectionBoxPad, listTop + listH);

	// ShopSetupDlg 하단 버튼과 동일한 크기/간격/위치 규칙 적용
	// ShopSetupDlg와 동일한 하단 버튼 규칙 적용
	const int buttonW = SX(110);
	const int buttonH = SX(bC ? 32 : 36);
	const int buttonGap = SX(12);
	const int BUTTON_BOTTOM = SX(bC ? 12 : 18);

	int totalW = buttonW * 2 + buttonGap;
	int bx = (rc.Width() - totalW) / 2;
	int by = rc.bottom - (SX(bC ? 10 : 22) + BUTTON_BOTTOM + buttonH);

	okRc = CRect(bx, by, bx + buttonW, by + buttonH);
	cancelRc = CRect(bx + buttonW + buttonGap, by, bx + buttonW + buttonGap + buttonW, by + buttonH);
	infoTitleArea = CRect(sec2TitlePt.x, sec2TitlePt.y, inner.right, sec2TitlePt.y + sectionTitleH);
}

CRect CReaderSetupDlg::CalcPortSectionBox(const CRect& card1, const CRect& card2) const
{
	return CRect(card1.left - SX(24), card1.top - SX(IsCompactScreen() ? 46 : 56), card1.right + SX(24), card2.bottom + SX(IsCompactScreen() ? 12 : 18));
}

CRect CReaderSetupDlg::CalcIntegritySectionBox(const CRect& queryBox, const CRect& listRc) const
{
	return CRect(queryBox.left - SX(24), queryBox.top - SX(IsCompactScreen() ? 38 : 48), queryBox.right + SX(24), listRc.bottom + SX(IsCompactScreen() ? 10 : 2));
}

// 데이터 저장용 ListCtrl의 컬럼 폭도 커스텀 테이블 비율과 동일하게 맞춘다.
// 실제 화면은 OnPaint가 그리더라도, 데이터 접근 시 컬럼 정의가 일관돼야 유지보수가 쉽다.
void CReaderSetupDlg::RecalcIntegrityColumns()
{
	if (!m_bUiInitialized || !::IsWindow(m_integrity_list.GetSafeHwnd()))
		return;

	CRect rcList;
	m_integrity_list.GetClientRect(&rcList);
	if (rcList.Width() <= 20)
		return;

	// 가로 스크롤이 생기지 않도록 현재 리스트 폭에 맞춰 컬럼 폭을 다시 계산한다.
	// 비율 합계는 100이며, 마지막 컬럼에서 남는 폭을 흡수한다.
	const int* ratios = kIntegrityColumnRatios;
	const int colCount = kIntegrityColumnCount;
	// 스크롤바와 본문 사이에 여백을 조금 더 둬서 마지막 컬럼이 답답해 보이지 않도록 한다.
	const int bodyWidth = max(120, rcList.Width() - ::GetSystemMetrics(SM_CXVSCROLL) - SX(8));

	int used = 0;
	for (int i = 0; i < colCount; ++i)
	{
		int cx = (i == colCount - 1) ? (bodyWidth - used) : (bodyWidth * ratios[i]) / 100;
		if (cx < SX(52))
			cx = SX(52);
		m_integrity_list.SetColumnWidth(i, cx);
		used += cx;
	}
}

void CReaderSetupDlg::NormalizeIntegrityScrollPos()
{
	const int itemCount = (::IsWindow(m_integrity_list.GetSafeHwnd())) ? m_integrity_list.GetItemCount() : 0;
	const int visibleRows = GetIntegrityVisibleRows();
	int maxScroll = itemCount - visibleRows;
	if (maxScroll < 0)
		maxScroll = 0;
	if (m_nIntegrityScrollPos < 0)
		m_nIntegrityScrollPos = 0;
	if (m_nIntegrityScrollPos > maxScroll)
		m_nIntegrityScrollPos = maxScroll;
}

// 현재 요구사항: 무결성 표는 한 화면에 3행만 노출한다.
int CReaderSetupDlg::GetIntegrityVisibleRows() const
{
	return 3;
}

BOOL CReaderSetupDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	const int itemCount = (::IsWindow(m_integrity_list.GetSafeHwnd())) ? m_integrity_list.GetItemCount() : 0;
	const int visibleRows = GetIntegrityVisibleRows();
	if (itemCount <= visibleRows)
		return CDialog::OnMouseWheel(nFlags, zDelta, pt);
	m_nIntegrityScrollPos += (zDelta > 0) ? -1 : 1;
	NormalizeIntegrityScrollPos();
	Invalidate(FALSE);
	return TRUE;
}

void CReaderSetupDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	const int itemCount = (::IsWindow(m_integrity_list.GetSafeHwnd())) ? m_integrity_list.GetItemCount() : 0;
	const int visibleRows = GetIntegrityVisibleRows();
	if (itemCount > visibleRows && !m_rcIntegrityScrollBar.IsRectEmpty() && m_rcIntegrityScrollBar.PtInRect(point))
	{
		if (point.y < m_rcIntegrityScrollThumb.top)
			m_nIntegrityScrollPos -= visibleRows;
		else if (point.y > m_rcIntegrityScrollThumb.bottom)
			m_nIntegrityScrollPos += visibleRows;
		else
		{
			// Start thumb drag
			m_bDraggingThumb = TRUE;
			m_nDragStartY = point.y;
			m_nDragStartScrollPos = m_nIntegrityScrollPos;
			SetCapture();
			return;
		}
		NormalizeIntegrityScrollPos();
		Invalidate(FALSE);
		return;
	}

	// 배경/빈 영역을 클릭하면 포커스를 확인 버튼으로 넘겨서
	// 스킨 콤보박스의 파란 포커스 테두리가 남지 않도록 정리한다.
	UINT hit = 0;
	CWnd* pChild = ChildWindowFromPoint(point, hit);
	if (pChild == this || pChild == NULL)
	{
		if (::IsWindow(m_btnOk.GetSafeHwnd()))
			m_btnOk.SetFocus();
		else if (::IsWindow(GetSafeHwnd()))
			SetFocus();
	}

	CDialog::OnLButtonDown(nFlags, point);
}

void CReaderSetupDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDraggingThumb)
	{
		const int itemCount = (::IsWindow(m_integrity_list.GetSafeHwnd())) ? m_integrity_list.GetItemCount() : 0;
		const int visibleRows = GetIntegrityVisibleRows();
		if (itemCount > visibleRows && !m_rcIntegrityScrollBar.IsRectEmpty())
		{
			const int thumbH = m_rcIntegrityScrollThumb.Height();
			const int thumbTravel = max(1, m_rcIntegrityScrollBar.Height() - thumbH);
			const int maxScroll = itemCount - visibleRows;
			int delta = point.y - m_nDragStartY;
			m_nIntegrityScrollPos = m_nDragStartScrollPos + (maxScroll * delta) / thumbTravel;
			NormalizeIntegrityScrollPos();
			Invalidate(FALSE);
		}
		return;
	}
	CDialog::OnMouseMove(nFlags, point);
}

void CReaderSetupDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDraggingThumb)
	{
		m_bDraggingThumb = FALSE;
		ReleaseCapture();
		return;
	}
	CDialog::OnLButtonUp(nFlags, point);
}

void CReaderSetupDlg::ApplyEnableStateToButtons(int readerIndex, BOOL bEnable)
{
	const COLORREF cardBgEnabled = RGB(255, 255, 255);
	const COLORREF cardBgDisabled = RGB(251, 252, 253);
	const COLORREF cardBg = bEnable ? cardBgEnabled : cardBgDisabled;

	const bool r1 = (readerIndex == 1);

	CModernButton* btns[] = {
		r1 ? &m_reader_init1     : &m_reader_init2,
		r1 ? &m_status_check1    : &m_status_check2,
		r1 ? &m_keydown1         : &m_keydown2,
		r1 ? &m_integrity_check1 : &m_integrity_check2,
		r1 ? &m_update1          : &m_update2,
	};
	for (auto* p : btns) { p->EnableWindow(bEnable); p->SetUnderlayColor(cardBg); }

	CModernToggleSwitch* toggles[] = {
		r1 ? &m_togglePortOpen1 : &m_togglePortOpen2,
		r1 ? &m_toggleMultipad1 : &m_toggleMultipad2,
	};
	for (auto* p : toggles) { p->EnableWindow(bEnable); p->SetUnderlayColor(cardBg); }

	CSkinnedComboBox& comport = r1 ? m_comport1 : m_comport2;
	comport.SetUnderlayColor(cardBg);
	comport.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME);

	if (r1) m_bReader1Enabled = bEnable;
	else    m_bReader2Enabled = bEnable;
}

void CReaderSetupDlg::UpdateReaderEnableState(int readerIndex)
{
	if (m_nBusyReaderIndex == readerIndex)
		return;
	CString sel;
	if (readerIndex == 1)
		m_comport1.GetWindowText(sel);
	else
		m_comport2.GetWindowText(sel);

	sel.Trim();

	// 콤보박스는 "항상" 활성 유지(요구사항)
	if (readerIndex == 1) m_comport1.EnableWindow(TRUE);
	else                  m_comport2.EnableWindow(TRUE);

	// 미사용이면 비활성, 그 외는 활성
	BOOL bEnable = (sel != _T("미사용"));
	ApplyEnableStateToButtons(readerIndex, bEnable);

	Invalidate(FALSE);
}

// 카드/조회 영역/하단 버튼을 현재 DPI와 창 크기에 맞춰 다시 배치한다.
void CReaderSetupDlg::LayoutControls()
{
	if (!m_bUiInitialized) return;

	if (!::IsWindow(m_comport1.GetSafeHwnd()) ||
		!::IsWindow(m_reader_init1.GetSafeHwnd()) ||
		!::IsWindow(m_integrity_list.GetSafeHwnd()))
		return;

	CRect inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc;
	CPoint sec1Pt, sec2Pt;
	CalcLayoutRects(inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc, sec1Pt, sec2Pt);

	const BOOL bCL = IsCompactScreen();
	const int padL = SX(bCL ? 52 : 64);
	const int padR = SX(22);
	const int comboW = SX(bCL ? 152 : 178);
	const int btnW = SX(kReaderActionButtonWidth);
	const int btnH = SX(bCL ? 30 : 37);
	const int gap = SX(8);
	const int toggleW = SX(bCL ? 44 : 52);
	const int toggleH = SX(28);
	const int rowComboY = SX(bCL ? 28 : 34);
	const int rowBtnY = SX(bCL ? 68 : 84);
	const int openLabelW = SX(56);
	const int multiLabelW = SX(92);
	const int textGap = SX(8);
	const int toggleBlockGap = SX(44);

	auto placeReaderCard = [&](const CRect& card,
		CSkinnedComboBox& cb,
		CModernButton& bInit, CModernButton& bStatus,
		CModernButton& bKey, CModernButton& bInteg,
		CModernButton& bUpdate,
		CModernToggleSwitch& tgOpen, CModernToggleSwitch& tgPad)
		{
			int x0 = card.left + padL;
			int yCombo = card.top + rowComboY;
			cb.SetWindowPos(NULL, x0, yCombo, comboW, SX(220), SWP_NOZORDER | SWP_NOACTIVATE);
		cb.SendMessage(CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)(btnH - 2));
		cb.SendMessage(CB_SETITEMHEIGHT, (WPARAM)0,  (LPARAM)(btnH - 2));

			int xTogglePad = card.right - padR - toggleW - SX(22);
			int xTogglePadLabel = xTogglePad - textGap - multiLabelW;
			int xToggleOpen = xTogglePadLabel - toggleBlockGap - (toggleW + textGap + openLabelW);
			if (xToggleOpen < x0 + comboW + SX(16))
				xToggleOpen = x0 + comboW + SX(16);
			int yToggle = yCombo + (btnH - toggleH) / 2;
			tgOpen.SetWindowPos(NULL, xToggleOpen, yToggle,
				toggleW + textGap + openLabelW, toggleH, SWP_NOZORDER | SWP_NOACTIVATE);

			tgPad.SetWindowPos(NULL, xTogglePadLabel, yToggle,
				toggleW + textGap + multiLabelW, toggleH, SWP_NOZORDER | SWP_NOACTIVATE);

			int yBtn = card.top + rowBtnY;
			int bx = x0;
			bInit.SetWindowPos(NULL, bx, yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
			bStatus.SetWindowPos(NULL, bx + (btnW + gap), yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
			bKey.SetWindowPos(NULL, bx + (btnW + gap) * 2, yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
			bInteg.SetWindowPos(NULL, bx + (btnW + gap) * 3, yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
			bUpdate.SetWindowPos(NULL, bx + (btnW + gap) * 4, yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
		};

	placeReaderCard(card1, m_comport1,
		m_reader_init1, m_status_check1, m_keydown1, m_integrity_check1, m_update1,
		m_togglePortOpen1, m_toggleMultipad1);
	placeReaderCard(card2, m_comport2,
		m_reader_init2, m_status_check2, m_keydown2, m_integrity_check2, m_update2,
		m_togglePortOpen2, m_toggleMultipad2);
	// Hide port-open toggle for reader 2 (not used)
	if (::IsWindow(m_togglePortOpen2.GetSafeHwnd()))
		m_togglePortOpen2.ShowWindow(SW_HIDE);

	// Place info icon buttons to the right of each toggle
	if (m_bUiInitialized) {
		auto PlaceInfoAfterToggle = [&](CModernToggleSwitch& tg, CInfoIconButton& btn, BOOL bShow) {
			if (!tg.GetSafeHwnd() || !btn.GetSafeHwnd()) return;
			CRect tgRc; tg.GetWindowRect(&tgRc); ScreenToClient(&tgRc);
			const int sz = SX(18);
			btn.SetWindowPos(NULL, tgRc.right + SX(4), tgRc.top + (tgRc.Height()-sz)/2, sz, sz, SWP_NOZORDER|SWP_NOACTIVATE);
			btn.ShowWindow(bShow ? SW_SHOW : SW_HIDE);
		};
		PlaceInfoAfterToggle(m_togglePortOpen1, m_btnPortOpenInfo,  TRUE);
		PlaceInfoAfterToggle(m_toggleMultipad1,  m_btnMultipad1Info, TRUE);
		PlaceInfoAfterToggle(m_toggleMultipad2,  m_btnMultipad2Info, TRUE);
	}

	int qx = queryBox.left;
	int qComboW = SX(bCL ? 100 : 120);
	CRect rcSearchCombo;
	m_search_date.GetWindowRect(&rcSearchCombo);
	int qVisibleH = btnH;
	int qy = queryBox.top + SX(bCL ? 8 : 10);
	m_search_date.SetWindowPos(NULL, qx, qy, qComboW, SX(220), SWP_NOZORDER | SWP_NOACTIVATE);
	m_search_date.SendMessage(CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)(btnH - 2));
	m_search_date.SendMessage(CB_SETITEMHEIGHT, (WPARAM)0,  (LPARAM)(btnH - 2));
	m_btnSearch.SetWindowPos(NULL, qx + qComboW + SX(12), qy, SX(bCL ? 64 : 78), qVisibleH, SWP_NOZORDER | SWP_NOACTIVATE);

	// 커스텀 표는 OnPaint에서 직접 그리고, 실제 리스트는 데이터 저장용으로만 숨긴다.
	m_integrity_list.SetWindowPos(NULL, -10000, -10000, 1, 1,
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
	m_btnOk.SetWindowPos(NULL, okRc.left, okRc.top, okRc.Width(), okRc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
	m_btnCancel.SetWindowPos(NULL, cancelRc.left, cancelRc.top, cancelRc.Width(), cancelRc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

/////////////////////////////////////////////////////////////////////////////
// CReaderSetupDlg dialog/////////////////////////////////////////////////////////////////////////////
// CReaderSetupDlg dialog


CReaderSetupDlg::CReaderSetupDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CReaderSetupDlg::IDD, pParent)
{
	m_bUiInitialized = FALSE;
	m_bFitDone = FALSE;
	m_bReader1Enabled = FALSE;
	m_bReader2Enabled = FALSE;
	m_nIntegrityScrollPos = 0;
	m_nLoadingButtonID = 0;
	m_nLoadingTimerID = 0;
	m_nLoadingAnimTimerID = 0;
	m_nPortOpenTimerID = 0;
	m_nBusyReaderIndex = 0;
	m_bBusySearch = FALSE;
	m_rcIntegrityScrollBar.SetRectEmpty();
	m_rcIntegrityScrollThumb.SetRectEmpty();
	m_pGdipSecFamily = nullptr;
	m_pGdipSecFont = nullptr;
	m_pGdipHdrTitleFont = nullptr;
	m_pGdipHdrSubFont = nullptr;
	// [버그 수정] 스크롤바 드래그 상태 변수들을 명시적으로 초기화하여 쓰레기값에 의한 스크롤 튕김 방지
	m_bDraggingThumb = FALSE;
	m_nDragStartY = 0;
	m_nDragStartScrollPos = 0;
	//{{AFX_DATA_INIT(CReaderSetupDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CReaderSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReaderSetupDlg)
	DDX_Control(pDX, IDC_INTEGRITY_LIST, m_integrity_list);
	DDX_Control(pDX, IDC_SEARCH_DATE, m_search_date);
	DDX_Control(pDX, IDC_READER_INIT2, m_reader_init2);
	DDX_Control(pDX, IDC_READER_INIT1, m_reader_init1);
	DDX_Control(pDX, IDC_STATUS_CHECK2, m_status_check2);
	DDX_Control(pDX, IDC_STATUS_CHECK1, m_status_check1);
	DDX_Control(pDX, IDC_KEYDOWN2, m_keydown2);
	DDX_Control(pDX, IDC_KEYDOWN1, m_keydown1);
	DDX_Control(pDX, IDC_INTEGRITY_CHECK2, m_integrity_check2);
	DDX_Control(pDX, IDC_INTEGRITY_CHECK1, m_integrity_check1);
	DDX_Control(pDX, IDC_COMPORT2, m_comport2);
	DDX_Control(pDX, IDC_COMPORT1, m_comport1);
	DDX_Control(pDX, IDC_UPDATE1, m_update1);
	DDX_Control(pDX, IDC_UPDATE2, m_update2);
	DDX_Control(pDX, IDC_PORT_OPEN1, m_togglePortOpen1);
	DDX_Control(pDX, IDC_PORT_OPEN2, m_togglePortOpen2);
	DDX_Control(pDX, IDC_MULTIPAD1, m_toggleMultipad1);
	DDX_Control(pDX, IDC_MULTIPAD2, m_toggleMultipad2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReaderSetupDlg, CDialog)
	//{{AFX_MSG_MAP(CReaderSetupDlg)
	ON_CBN_SELCHANGE(IDC_COMPORT1, OnSelchangeComport1)
	ON_CBN_SELCHANGE(IDC_COMPORT2, OnSelchangeComport2)
	//}}AFX_MSG_MAP
	ON_WM_PAINT()
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_DRAWITEM()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_PORT_OPEN_INFO,  OnBnClickedPortOpenInfo)
	ON_BN_CLICKED(IDC_BTN_MULTIPAD1_INFO,  OnBnClickedMultipad1Info)
	ON_BN_CLICKED(IDC_BTN_MULTIPAD2_INFO,  OnBnClickedMultipad2Info)
	ON_BN_CLICKED(IDC_PORT_OPEN1, OnPortOpen1Clicked)
	ON_MESSAGE(WM_READER_DONE,    OnReaderDone)
	ON_MESSAGE(WM_PORT_OPEN_DONE, OnPortOpenDone)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReaderSetupDlg message handlers/////////////////////////////////////////////////////////////////////////////
// CReaderSetupDlg message handlers

void CReaderSetupDlg::GetNTComPort(vector<int>& ports)
{
	HKEY hKey = NULL;
	if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Hardware\\DeviceMap\\SerialComm"),
	                   0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return;

	int m_nPortNo = 0;
	TCHAR name[255];
	DWORD nameLen;
	BYTE data[255];
	DWORD dataLen;
	for (;;)
	{
		nameLen = sizeof(name);
		dataLen = sizeof(data);
		int err = ::RegEnumValue(hKey, (DWORD)m_nPortNo, name, &nameLen, NULL, NULL, data, &dataLen);
		if (err != ERROR_SUCCESS)
			break;
		char* ptr = (char*)data;
		while ((*ptr < '0') || (*ptr > '9'))
		{
			if (*ptr == 0x00)
				break;
			ptr++;
		}
		ports.push_back(atoi(ptr));
		m_nPortNo++;
	}
	::RegCloseKey(hKey);
}

void CReaderSetupDlg::SetReaderCardBusy(int readerIndex, BOOL bBusy)
{
	CSkinnedComboBox* pCombo = (readerIndex == 1) ? &m_comport1 : &m_comport2;
	CModernToggleSwitch* pToggleOpen = (readerIndex == 1) ? &m_togglePortOpen1 : &m_togglePortOpen2;
	CModernToggleSwitch* pToggleMulti = (readerIndex == 1) ? &m_toggleMultipad1 : &m_toggleMultipad2;
	CModernButton* buttons[] =
	{
		(readerIndex == 1) ? &m_reader_init1 : &m_reader_init2,
		(readerIndex == 1) ? &m_status_check1 : &m_status_check2,
		(readerIndex == 1) ? &m_keydown1 : &m_keydown2,
		(readerIndex == 1) ? &m_integrity_check1 : &m_integrity_check2,
		(readerIndex == 1) ? &m_update1 : &m_update2,
	};

	for (int i = 0; i < (int)(sizeof(buttons) / sizeof(buttons[0])); ++i)
	{
		if (!buttons[i]->GetSafeHwnd())
			continue;
		if (!bBusy)
			buttons[i]->EnableWindow(TRUE);
		else if (buttons[i]->GetDlgCtrlID() != (int)m_nLoadingButtonID)
			buttons[i]->EnableWindow(FALSE);
	}

	if (pCombo->GetSafeHwnd()) pCombo->EnableWindow(!bBusy);
	if (pToggleOpen->GetSafeHwnd()) pToggleOpen->EnableWindow(!bBusy);
	if (pToggleMulti->GetSafeHwnd()) pToggleMulti->EnableWindow(!bBusy);

    if (!bBusy)
    {
        UpdateReaderEnableState(readerIndex);
        ApplyAopRestrictions();
    }
}

void CReaderSetupDlg::SetSearchBusy(BOOL bBusy)
{
	m_bBusySearch = bBusy;
	if (m_search_date.GetSafeHwnd())
		m_search_date.EnableWindow(!bBusy);
	if (m_btnSearch.GetSafeHwnd() && !bBusy)
		m_btnSearch.EnableWindow(TRUE);
	Invalidate(FALSE);
}

BOOL CReaderSetupDlg::IsReaderCardButton(UINT nID) const
{
	switch (nID)
	{
	case IDC_READER_INIT1:
	case IDC_STATUS_CHECK1:
	case IDC_KEYDOWN1:
	case IDC_INTEGRITY_CHECK1:
	case IDC_UPDATE1:
	case IDC_READER_INIT2:
	case IDC_STATUS_CHECK2:
	case IDC_KEYDOWN2:
	case IDC_INTEGRITY_CHECK2:
	case IDC_UPDATE2:
		return TRUE;
	}
	return FALSE;
}

CString CReaderSetupDlg::MakeLoadingText(UINT nButtonID) const
{
	switch (nButtonID)
	{
	case IDC_READER_INIT1:
	case IDC_READER_INIT2:
		return _T("초기화중...");
	case IDC_STATUS_CHECK1:
	case IDC_STATUS_CHECK2:
		return _T("확인중...");
	case IDC_KEYDOWN1:
	case IDC_KEYDOWN2:
		return _T("다운로드중...");
	case IDC_INTEGRITY_CHECK1:
	case IDC_INTEGRITY_CHECK2:
		return _T("체크중...");
	case IDC_UPDATE1:
	case IDC_UPDATE2:
		return _T("업데이트중...");
	case IDC_SEARCH:
		return _T("조회중...");
	}
	return _T("처리중...");
}

// COM 포트 콤보는 항상 "미사용"을 맨 위에 두고, 실제 포트는 COM 01 형태로 정규화해 넣는다.
void CReaderSetupDlg::InitPortComboItems()
{
	vector<int> ports;
	GetNTComPort(ports);

	m_comport1.ResetContent();
	m_comport2.ResetContent();
	m_comport1.AddString(_T("미사용"));
	m_comport2.AddString(_T("미사용"));

	for (size_t i = 0; i < ports.size(); ++i)
	{
		CString item;
		item.Format(_T("COM %02d"), ports[i]);
		m_comport1.AddString(item);
		m_comport2.AddString(item);
	}
}

// 저장된 COM 포트를 복원하고, 현재 PC에 없는 포트는 '(사용불가)' 표기를 붙여서라도
// 사용자가 과거 설정값을 볼 수 있게 한다.
void CReaderSetupDlg::LoadSavedPortSelections()
{
	CString com_port1 = AfxGetApp()->GetProfileString(SERIAL_PORT_SECTION, COMPORT1_FIELD, _T(""));
	CString com_port2 = AfxGetApp()->GetProfileString(SERIAL_PORT_SECTION, COMPORT2_FIELD, _T(""));

	if (com_port1.IsEmpty())
	{
		com_port1 = _T("미사용");
		AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, COMPORT1_FIELD, com_port1);
	}
	if (com_port2.IsEmpty())
	{
		com_port2 = _T("미사용");
		AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, COMPORT2_FIELD, com_port2);
	}

	auto applySelection = [&](CSkinnedComboBox& combo, const CString& savedValue)
		{
			combo.SetCurSel(0);
			const int count = combo.GetCount();
			for (int i = 0; i < count; ++i)
			{
				CString item;
				combo.GetLBText(i, item);
				if (IsSamePortLabel(item, savedValue))
				{
					combo.SetCurSel(i);
					return;
				}
			}

			if (!IsSamePortLabel(savedValue, _T("미사용")))
			{
				CString unavailable = savedValue + _T("(사용불가)");
				combo.AddString(unavailable);
				combo.SetCurSel(combo.GetCount() - 1);
			}
			else
			{
				combo.SetCurSel(0);
			}
		};

	applySelection(m_comport1, com_port1);
	applySelection(m_comport2, com_port2);

	// Load port always open state: "0" = ON, "1" = OFF (default)
	CString portAlwaysOpen = AfxGetApp()->GetProfileString(SERIAL_PORT_SECTION, PORT_ALWAYS_OPEN, _T("1"));
	m_togglePortOpen1.SetToggled(portAlwaysOpen == _T("0"));

	// Load multipad toggle state: "1" = OFF (default), "0" = ON
	CString mp1 = AfxGetApp()->GetProfileString(SERIAL_PORT_SECTION, MULTIPAD1_FIELD, _T("1"));
	CString mp2 = AfxGetApp()->GetProfileString(SERIAL_PORT_SECTION, MULTIPAD2_FIELD, _T("1"));
	m_toggleMultipad1.SetToggled(mp1 == _T("0"));
	m_toggleMultipad2.SetToggled(mp2 == _T("0"));
}

void CReaderSetupDlg::InitSearchDateCombo()
{
	m_search_date.ResetContent();
	m_search_date.AddString(_T("오늘"));
	m_search_date.AddString(_T("7일"));
	m_search_date.AddString(_T("30일"));
	m_search_date.AddString(_T("100일"));
	m_search_date.SetCurSel(0);
}

// 실제 ListCtrl은 커스텀 테이블의 데이터 저장소로만 쓰고, 화면 표시 자체는 OnPaint에서 담당한다.
void CReaderSetupDlg::InitIntegrityListColumns()
{
	DWORD exStyle = LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT;
#ifdef LVS_EX_DOUBLEBUFFER
	exStyle |= LVS_EX_DOUBLEBUFFER;
#endif
	ListView_SetExtendedListViewStyle(m_integrity_list.GetSafeHwnd(), exStyle);
	m_integrity_list.ModifyStyle(0, LVS_SHOWSELALWAYS);

	while (m_integrity_list.DeleteColumn(0))
	{
		// 기존 컬럼 제거
	}

	for (int i = 0; col_info[i].column_name != NULL; ++i)
		m_integrity_list.InsertColumn(i, col_info[i].column_name, col_info[i].format, col_info[i].width * 8);

	m_integrity_list.SetBkColor(RGB(255, 255, 255));
	m_integrity_list.SetTextBkColor(RGB(255, 255, 255));
	m_integrity_list.ShowWindow(SW_HIDE);
}

void CReaderSetupDlg::ApplyDialogFonts()
{
	ApplyFontIfWindow(m_comport1, m_fontNormal);
	ApplyFontIfWindow(m_comport2, m_fontNormal);
	ApplyFontIfWindow(m_search_date, m_fontNormal);

	// [FIX] 14 하드코딩 대신 폰트에 적용된 실제 렌더링 픽셀 높이를 그대로 추출하여 동기화
	LOGFONT lfCombo = { 0 };
	m_fontNormal.GetLogFont(&lfCombo);
	int exactFontPx = abs(lfCombo.lfHeight);

	m_comport1.SetTextPx(exactFontPx);
	m_comport2.SetTextPx(exactFontPx);
	m_search_date.SetTextPx(exactFontPx);

	ApplyFontIfWindow(m_reader_init1, m_fontNormal);
	ApplyFontIfWindow(m_status_check1, m_fontNormal);
	ApplyFontIfWindow(m_keydown1, m_fontNormal);
	ApplyFontIfWindow(m_integrity_check1, m_fontNormal);
	ApplyFontIfWindow(m_update1, m_fontNormal);
	ApplyFontIfWindow(m_reader_init2, m_fontNormal);
	ApplyFontIfWindow(m_status_check2, m_fontNormal);
	ApplyFontIfWindow(m_keydown2, m_fontNormal);
	ApplyFontIfWindow(m_integrity_check2, m_fontNormal);
	ApplyFontIfWindow(m_update2, m_fontNormal);
	ApplyFontIfWindow(m_btnSearch, m_fontNormal);
	ApplyFontIfWindow(m_btnOk, m_fontNormal);
	ApplyFontIfWindow(m_btnCancel, m_fontNormal);
	ApplyFontIfWindow(m_integrity_list, m_fontNormal);
}

void CReaderSetupDlg::InitModernButtonStyles()
{
	// 리더 카드 액션 버튼은 전부 동일한 Secondary 계열 스타일을 써서 시선이 분산되지 않게 한다.
	m_reader_init1.SetButtonStyle(ButtonStyle::Reader);
	m_status_check1.SetButtonStyle(ButtonStyle::Reader);
	m_keydown1.SetButtonStyle(ButtonStyle::Reader);
	m_integrity_check1.SetButtonStyle(ButtonStyle::Reader);
	m_update1.SetButtonStyle(ButtonStyle::Reader);
	m_reader_init2.SetButtonStyle(ButtonStyle::Reader);
	m_status_check2.SetButtonStyle(ButtonStyle::Reader);
	m_keydown2.SetButtonStyle(ButtonStyle::Reader);
	m_integrity_check2.SetButtonStyle(ButtonStyle::Reader);
	m_update2.SetButtonStyle(ButtonStyle::Reader);

	if (CWnd* pOK = GetDlgItem(IDOK))
		m_btnOk.SubclassDlgItem(IDOK, this);
	if (CWnd* pCancel = GetDlgItem(IDCANCEL))
		m_btnCancel.SubclassDlgItem(IDCANCEL, this);
	m_btnOk.SetButtonStyle(ButtonStyle::Primary);
	m_btnCancel.SetButtonStyle(ButtonStyle::Default);

	m_btnSearch.SubclassDlgItem(IDC_SEARCH, this);
	m_btnSearch.SetButtonStyle(ButtonStyle::Reader);

	m_reader_init1.SetWindowText(_T("초기화"));
	m_status_check1.SetWindowText(_T("상태체크"));
	m_keydown1.SetWindowText(_T("키다운로드"));
	m_integrity_check1.SetWindowText(_T("무결성체크"));
	m_update1.SetWindowText(_T("업데이트"));
	m_reader_init2.SetWindowText(_T("초기화"));
	m_status_check2.SetWindowText(_T("상태체크"));
	m_keydown2.SetWindowText(_T("키다운로드"));
	m_integrity_check2.SetWindowText(_T("무결성체크"));
	m_update2.SetWindowText(_T("업데이트"));
	m_btnOk.SetWindowText(_T("확인"));
	m_btnCancel.SetWindowText(_T("취소"));
	m_btnSearch.SetWindowText(_T("조회"));
}

void CReaderSetupDlg::InitSampleIntegrityRows()
{
	if (m_integrity_list.GetItemCount() > 0)
		return;

	const TCHAR* sampleRows[][6] =
	{
		{ _T("20260308091234"), _T("COM 01"), _T("00"), _T("RDR-1001"), _T("##SPAY-8800Q3001"), _T("KFTCONECAP3001") },
		{ _T("20260308091234"), _T("COM 01"), _T("04"), _T("RDR-1001"), _T("##SPAY-8800Q3001"), _T("KFTCONECAP3001") },
		{ _T("20260308091234"), _T("COM 02"), _T("00"), _T("RDR-2003"), _T("DAULPAY633RDK201"), _T("KFTCONECAP3001") },
		{ _T("20260308091234"), _T("COM 03"), _T("00"), _T("RDR-1010"), _T("DAULPAY633RDK201"), _T("KFTCONECAP3001") },
		{ _T("20260308091234"), _T("COM 01"), _T("00"), _T("RDR-1001"), _T("##SPAY-8800Q3001"), _T("KFTCONECAP3001") },
		{ _T("20260308091234"), _T("COM 04"), _T("00"), _T("RDR-3011"), _T("##SPAY-8800Q3001"), _T("KFTCONECAP3001") },
	};

	for (int row = 0; row < 6; ++row)
	{
		int idx = m_integrity_list.InsertItem(row, sampleRows[row][0]);
		for (int col = 1; col < 6; ++col)
			m_integrity_list.SetItemText(idx, col, sampleRows[row][col]);
	}
}

void CReaderSetupDlg::InitToggleAndUnderlayColors()
{
	const COLORREF cardBgEnabled = RGB(255, 255, 255);
	const COLORREF cardBgDisabled = RGB(251, 252, 253);
	const COLORREF infoCardBg = RGB(248, 249, 251);

	// [FIX] m_fontLabel의 실제 렌더링 픽셀 높이를 추출하여 토글 텍스트 크기 완벽 동기화
	LOGFONT lfLabel = { 0 };
	m_fontLabel.GetLogFont(&lfLabel);
	int exactLabelPx = abs(lfLabel.lfHeight);

	// Setup text labels inside toggle controls
	m_togglePortOpen1.SetFont(&m_fontLabel);
	m_togglePortOpen1.SetWindowText(_T("포트 열기"));
	m_togglePortOpen1.SetTextSizePx(exactLabelPx);
	m_togglePortOpen1.SetNoWrapEllipsis(TRUE);
	m_toggleMultipad1.SetFont(&m_fontLabel);
	m_toggleMultipad1.SetWindowText(_T("멀티패드 여부"));
	m_toggleMultipad1.SetTextSizePx(exactLabelPx);
	m_toggleMultipad1.SetNoWrapEllipsis(TRUE);
	m_toggleMultipad2.SetFont(&m_fontLabel);
	m_toggleMultipad2.SetWindowText(_T("멀티패드 여부"));
	m_toggleMultipad2.SetTextSizePx(exactLabelPx);
	m_toggleMultipad2.SetNoWrapEllipsis(TRUE);

	m_togglePortOpen1.SetUnderlayColor(cardBgEnabled);
	m_togglePortOpen2.SetUnderlayColor(cardBgDisabled);
	m_toggleMultipad1.SetUnderlayColor(cardBgEnabled);
	m_toggleMultipad2.SetUnderlayColor(cardBgDisabled);
	m_reader_init1.SetUnderlayColor(cardBgEnabled);
	m_status_check1.SetUnderlayColor(cardBgEnabled);
	m_keydown1.SetUnderlayColor(cardBgEnabled);
	m_integrity_check1.SetUnderlayColor(cardBgEnabled);
	m_update1.SetUnderlayColor(cardBgEnabled);
	m_reader_init2.SetUnderlayColor(cardBgDisabled);
	m_status_check2.SetUnderlayColor(cardBgDisabled);
	m_keydown2.SetUnderlayColor(cardBgDisabled);
	m_integrity_check2.SetUnderlayColor(cardBgDisabled);
	m_update2.SetUnderlayColor(cardBgDisabled);
	m_comport1.SetUnderlayColor(cardBgEnabled);
	m_comport2.SetUnderlayColor(cardBgDisabled);
	m_search_date.SetUnderlayColor(infoCardBg);
	m_btnSearch.SetUnderlayColor(infoCardBg);
	m_btnOk.SetUnderlayColor(RGB(255, 255, 255));
	m_btnCancel.SetUnderlayColor(RGB(255, 255, 255));
}

void CReaderSetupDlg::StartLoadingOperation(UINT nButtonID)
{
	if (m_nLoadingButtonID != 0)
		return;

	CModernButton* pButton = NULL;
	switch (nButtonID)
	{
	case IDC_READER_INIT1: pButton = &m_reader_init1; break;
	case IDC_STATUS_CHECK1: pButton = &m_status_check1; break;
	case IDC_KEYDOWN1: pButton = &m_keydown1; break;
	case IDC_INTEGRITY_CHECK1: pButton = &m_integrity_check1; break;
	case IDC_UPDATE1: pButton = &m_update1; break;
	case IDC_READER_INIT2: pButton = &m_reader_init2; break;
	case IDC_STATUS_CHECK2: pButton = &m_status_check2; break;
	case IDC_KEYDOWN2: pButton = &m_keydown2; break;
	case IDC_INTEGRITY_CHECK2: pButton = &m_integrity_check2; break;
	case IDC_UPDATE2: pButton = &m_update2; break;
	case IDC_SEARCH: pButton = &m_btnSearch; break;
	default: return;
	}

	m_nLoadingButtonID = nButtonID;
	pButton->SetLoading(TRUE, MakeLoadingText(nButtonID));

	if (nButtonID == IDC_SEARCH)
	{
		SetSearchBusy(TRUE);
	}
	else
	{
		m_nBusyReaderIndex = (nButtonID == IDC_READER_INIT2 || nButtonID == IDC_STATUS_CHECK2 || nButtonID == IDC_KEYDOWN2 || nButtonID == IDC_INTEGRITY_CHECK2 || nButtonID == IDC_UPDATE2) ? 2 : 1;
		SetReaderCardBusy(1, TRUE);
		SetReaderCardBusy(2, TRUE);
		// Also disable the loading button itself
		if (pButton->GetSafeHwnd()) pButton->EnableWindow(FALSE);
	}

	m_nLoadingAnimTimerID = 0x4810;
	m_nLoadingTimerID = 0x4811;
	SetTimer(m_nLoadingAnimTimerID, 33, NULL);
	SetTimer(m_nLoadingTimerID, 3000, NULL);
}

void CReaderSetupDlg::FinishLoadingOperation(BOOL bRefresh)
{
	if (m_nLoadingAnimTimerID != 0)
		KillTimer(m_nLoadingAnimTimerID);
	if (m_nLoadingTimerID != 0)
		KillTimer(m_nLoadingTimerID);
	m_nLoadingAnimTimerID = 0;
	m_nLoadingTimerID = 0;

	if (m_nLoadingButtonID != 0)
	{
		CModernButton* pButton = NULL;
		switch (m_nLoadingButtonID)
		{
		case IDC_READER_INIT1: pButton = &m_reader_init1; break;
		case IDC_STATUS_CHECK1: pButton = &m_status_check1; break;
		case IDC_KEYDOWN1: pButton = &m_keydown1; break;
		case IDC_INTEGRITY_CHECK1: pButton = &m_integrity_check1; break;
		case IDC_UPDATE1: pButton = &m_update1; break;
		case IDC_READER_INIT2: pButton = &m_reader_init2; break;
		case IDC_STATUS_CHECK2: pButton = &m_status_check2; break;
		case IDC_KEYDOWN2: pButton = &m_keydown2; break;
		case IDC_INTEGRITY_CHECK2: pButton = &m_integrity_check2; break;
		case IDC_UPDATE2: pButton = &m_update2; break;
		case IDC_SEARCH: pButton = &m_btnSearch; break;
		}
		if (pButton != NULL)
			pButton->SetLoading(FALSE);
	}

	if (m_bBusySearch)
		SetSearchBusy(FALSE);
	if (m_nBusyReaderIndex != 0)
	{
		SetReaderCardBusy(1, FALSE);
		SetReaderCardBusy(2, FALSE);
	}

	m_nBusyReaderIndex = 0;
	m_nLoadingButtonID = 0;
	if (bRefresh)
		Invalidate(FALSE);
	if (::IsWindow(m_btnOk.GetSafeHwnd()))
		m_btnOk.SetFocus();
}

struct PortOpenParam
{
    HWND hWnd;
    BOOL bDesired;   // desired toggle state after thread completes
};

static UINT PortOpenWorkerThread(LPVOID pParam)
{

    PortOpenParam* p = (PortOpenParam*)pParam;
    // TODO: actual port open/close operation here
    Sleep(2500);  // TODO: remove - simulates operation delay for animation testing
    BOOL bSuccess = TRUE;
    ::PostMessage(p->hWnd, WM_PORT_OPEN_DONE, bSuccess, (LPARAM)p->bDesired);
    delete p;

    return 0;
}

void CReaderSetupDlg::OnPortOpen1Clicked()
{
    BOOL bDesired = m_togglePortOpen1.IsToggled(); // already flipped by OnLButtonUp
    m_togglePortOpen1.SetToggled(!bDesired);       // revert before showing confirm dialog

    // TODO: replace message text with Korean strings in Visual Studio
    LPCTSTR pMsg = bDesired ? _T("Open port?") : _T("Close port?");
    if (CModernMessageBox::Question(pMsg, this) != IDYES)
        return; // cancelled - toggle already reverted

    m_togglePortOpen1.SetToggled(bDesired);        // confirmed - apply new state
    m_togglePortOpen1.SetPending(TRUE);            // start spinner animation
    m_nBusyReaderIndex = 1;
    SetReaderCardBusy(1, TRUE);  // disable all reader1 controls while port open runs

    PortOpenParam* p = new PortOpenParam();
    p->hWnd     = m_hWnd;
    p->bDesired = bDesired;
    AfxBeginThread(PortOpenWorkerThread, p);
    m_nPortOpenTimerID = SetTimer(0x4820, 10000, NULL);  // 10-second timeout
}

LRESULT CReaderSetupDlg::OnReaderDone(WPARAM wParam, LPARAM lParam)
{
	BOOL bSuccess = (BOOL)wParam;
	int  nCtrlID  = (int)HIWORD(lParam);
	BOOL bDesired = (BOOL)LOWORD(lParam);

	FinishLoadingOperation(TRUE);
	return 0;
}

LRESULT CReaderSetupDlg::OnPortOpenDone(WPARAM wParam, LPARAM lParam)
{
	if (m_nPortOpenTimerID) { KillTimer(m_nPortOpenTimerID); m_nPortOpenTimerID = 0; }
	BOOL bSuccess = (BOOL)wParam;
	BOOL bDesired = (BOOL)lParam;

	BOOL bFinalState = bDesired;
	if (!bSuccess)
	{
		bFinalState = !bDesired; // revert on failure
		m_togglePortOpen1.SetToggled(bFinalState);
	}
	m_togglePortOpen1.SetPending(FALSE);          // stop spinner, animate to final state
	m_nBusyReaderIndex = 0;
	SetReaderCardBusy(1, FALSE);  // re-enable all reader1 controls
	// When reader1 port is ON, set reader2 comport to unused and disable buttons
	if (bFinalState)
	{
		m_comport2.SetCurSel(0);
		m_comport2.EnableWindow(FALSE);
		ApplyEnableStateToButtons(2, FALSE);
		Invalidate(FALSE);
	}
	else
	{
		// Port open turned OFF: re-enable reader2 controls
		UpdateReaderEnableState(2);
		Invalidate(FALSE);
	}

	// Save port open state to registry: "0" = ON, "1" = OFF
	if (bSuccess)
		AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, PORT_ALWAYS_OPEN, bFinalState ? _T("0") : _T("1"));

	CModernMessageBox::Warning(_T("확인"), this);

	if (::IsWindow(m_btnOk.GetSafeHwnd()))
		m_btnOk.SetFocus();
	return 0;
}

BOOL CReaderSetupDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// --- 1) GDI+/윈도우 기본 스타일 준비 ---
	ModernUIGfx::EnsureGdiplusStartup();
	ModifyStyle(0, WS_CLIPCHILDREN);

	// --- 2) DPI/폰트/숨길 레거시 Static 초기화 ---
	EnsureFonts();
	m_brushBg.CreateSolidBrush(RGB(249, 250, 252));
	HideLegacyStatics();

	// --- 3) 입력 컨트롤 데이터 초기화 ---
	InitPortComboItems();
	LoadSavedPortSelections();
	InitSearchDateCombo();
	InitIntegrityListColumns();
	InitSampleIntegrityRows();
	NormalizeIntegrityScrollPos();

	// --- 4) 폰트/ModernUI 스타일/배경색 적용 ---
	ApplyDialogFonts();
	InitModernButtonStyles();
	InitToggleAndUnderlayColors();

	// Create info icon buttons and popover
	{
		auto CreateInfoBtn = [&](CInfoIconButton& btn, UINT id) {
			btn.Create(_T(""), WS_CHILD | BS_OWNERDRAW, CRect(0, 0, SX(22), SX(22)), this, id);
		};
		CreateInfoBtn(m_btnPortOpenInfo,  IDC_BTN_PORT_OPEN_INFO);
		CreateInfoBtn(m_btnMultipad1Info, IDC_BTN_MULTIPAD1_INFO);
		CreateInfoBtn(m_btnMultipad2Info, IDC_BTN_MULTIPAD2_INFO);
		m_btnPortOpenInfo.SetUnderlayColor(RGB(255, 255, 255));
		m_btnMultipad1Info.SetUnderlayColor(RGB(255, 255, 255));
		m_btnMultipad2Info.SetUnderlayColor(RGB(251, 252, 253));
	}

	// 레이아웃 계산 전에 UI 준비 상태를 TRUE로 올려야 LayoutControls가 정상 동작한다.
	m_bUiInitialized = TRUE;

	// --- 5) 창 크기/컨트롤 배치 적용 ---
	FitWindowToLayout();
	LayoutControls();
	RecalcIntegrityColumns();

	// --- 6) 최종 상태 반영 ---
	UpdateReaderEnableState(1);
	UpdateReaderEnableState(2);
    ApplyAopRestrictions();
	// If port1 is ON, disable comport2 combobox after UpdateReaderEnableState
	if (m_togglePortOpen1.IsToggled())
		m_comport2.EnableWindow(FALSE);


		
	// 초기 포커스는 확인 버튼으로 넘겨 포커스 링이 콤보에 남지 않게 한다.
	ModernUIWindow::ApplyWhiteTitleBar(this->GetSafeHwnd());
	TakeSnapshot();
	GetDlgItem(IDOK)->SetFocus();
	return FALSE;
}

CSize CReaderSetupDlg::CalcMinClientSize() const
{
	const BOOL bC = IsCompactScreen();
	const int margin = SX(20);
	const int innerW = SX(bC ? 660 : 720);
	const int titleArea = SX(bC ? 68 : 92);
	const int sectionPad = SX(bC ? 16 : 24);
	const int sectionTitleTop = SX(bC ? 10 : 12);
	const int infoSectionTitleTop = SX(2);
	const int sectionTitleH = SX(bC ? 24 : 28);
	const int sectionTitleGap = SX(12);
	const int infoTitleGap = -SX(10);
	const int cardH = SX(bC ? 96 : 128);
	const int cardGap = SX(bC ? 8 : 12);
	const int betweenSections = SX(bC ? 18 : 26);
	const int queryH = SX(bC ? 44 : 62);
	const int queryGap = SX(bC ? 12 : 16);
	const int listMinH = SX(bC ? 138 : 152);
	const int infoBottomPad = SX(bC ? 14 : 18);
	const int bottomBtnH = SX(bC ? 36 : 42);
	const int bottomGap = SX(bC ? 12 : 16);
	const int bottomArea = bottomBtnH + bottomGap + SX(8);

	int clientW = innerW + margin * 2;
	int clientH = 0;
	clientH += margin;
	clientH += titleArea;
	clientH += sectionTitleTop + sectionTitleH + sectionTitleGap;
	clientH += cardH + cardGap + cardH + sectionPad;
	clientH += betweenSections;
	clientH += sectionTitleTop + sectionTitleH + infoTitleGap;
	clientH += queryH + queryGap + listMinH + infoBottomPad;
	clientH += bottomArea;
	clientH += margin;
	return CSize(clientW, clientH);
}

void CReaderSetupDlg::FitWindowToLayout()
{
	if (m_bFitDone) return;
	m_bFitDone = TRUE;

	CSize needClient = CalcMinClientSize();

	// 클라이언트 크기를 기준으로 실제 윈도우 크기 계산
	CRect rcWnd(0, 0, needClient.cx, needClient.cy);
	AdjustWindowRectEx(&rcWnd, GetStyle(), FALSE, GetExStyle());

	RECT rcWork; SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
	int wW = min(rcWnd.Width(),  (int)(rcWork.right  - rcWork.left));
	int wH = min(rcWnd.Height(), (int)(rcWork.bottom - rcWork.top));
	SetWindowPos(NULL, 0, 0, wW, wH, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	CenterWindow();
}

void CReaderSetupDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (!m_bUiInitialized)
		return;

	if (GetSafeHwnd())
	{
		LayoutControls();
		RecalcIntegrityColumns();
	}
}
BOOL CReaderSetupDlg::OnEraseBkgnd(CDC* pDC)
{
	return TRUE; // 전체를 OnPaint에서 칠함
}

void CReaderSetupDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
	CDialog::OnDrawItem(nIDCtl, lpDIS);
}

void CReaderSetupDlg::OnPaint()
{
	CPaintDC dc(this);
	EnsureFonts();

	CRect rc; GetClientRect(&rc);

	CDC memDC; memDC.CreateCompatibleDC(&dc);
	CBitmap bmp; bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
	CBitmap* oldBmp = memDC.SelectObject(&bmp);

	memDC.FillSolidRect(rc, RGB(249, 250, 252));
	memDC.SetBkMode(TRANSPARENT);
	Gdiplus::Graphics gPaint(memDC.GetSafeHdc());
	gPaint.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	gPaint.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	const BOOL bCP = IsCompactScreen();

	const int cardMarginL = SX(20);
	const int cardMarginT = SX(10);
	const int cardMarginR = SX(20);
	const int cardMarginB = SX(bCP ? 12 : 20);
	CRect mainCard(rc.left + cardMarginL, rc.top + cardMarginT, rc.right - cardMarginR, rc.bottom - cardMarginB);
	{
		Gdiplus::Graphics gShad(memDC.GetSafeHdc());
		gShad.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		gShad.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
		float mX = (float)mainCard.left, mY = (float)mainCard.top;
		float mW = (float)mainCard.Width(), mH = (float)mainCard.Height();
		// 수정: 16px 곡률 및 2단계 연한 그림자 (ShopSetup과 동일)
		float mR = (float)SX(16);
		for (int sh = 1; sh <= 2; sh++)
		{
			Gdiplus::GraphicsPath shPath;
			// Y축 오프셋을 sh * 1.5f로 주어 아래로 부드럽게 퍼지게 함
			GdipAddRoundRect(shPath, mX, mY + (float)sh * 1.5f, mW, mH, mR);
			// 알파값을 8로 고정하여 아주 연하게 처리 (핵심)
			Gdiplus::SolidBrush shBrush(Gdiplus::Color(8, 0, 0, 0));
			gPaint.FillPath(&shBrush, &shPath);
		}
	}
	// [FIX] 메인 카드 테두리 제거 (borderW = 0)
	FillRoundRect(gPaint, mainCard, SX(16), RGB(255, 255, 255), RGB(255, 255, 255), 0);

	const int iconSize = SX(bCP ? 36 : 44);
	const int iconX    = mainCard.left + SX(14);
	const int iconY    = mainCard.top  + SX(16);

	{
		wchar_t wTitle[128] = {}, wSub[256] = {};
		MultiByteToWideChar(CP_ACP, 0, _T("리더기 설정"), -1, wTitle, 128);
		MultiByteToWideChar(CP_ACP, 0, _T("리더기 연결 및 제어 설정을 관리합니다"), -1, wSub, 256);
		ModernUIHeader::Draw(memDC.GetSafeHdc(),
			(float)iconX, (float)iconY, (float)iconSize,
			ModernUIHeader::IconType::CardTerminal,
			wTitle, wSub,
			(HFONT)m_fontTitle.GetSafeHandle(),
			(HFONT)m_fontSub.GetSafeHandle(),
			mainCard.left + SX(6), mainCard.top + SX(bCP ? 60 : 74), mainCard.right - SX(6), bCP ? 23.0f : 26.0f, bCP ? 3.0f : 0.0f);
	}

	CRect inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc;
	CPoint sec1Pt, sec2Pt;
	CalcLayoutRects(inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc, sec1Pt, sec2Pt);
	CRect portSection = CalcPortSectionBox(card1, card2);
	CRect integritySection = CalcIntegritySectionBox(queryBox, listRc);



	{
		Gdiplus::Graphics gSecShad(memDC.GetSafeHdc());
		gSecShad.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		gSecShad.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
		auto drawSecCard = [&](const CRect& sc)
		{
				float scR = (float)SX(16); // 곡률 16px로 통일
				// 섹션 카드도 메인 카드와 동일한 그림자 규칙 적용
				for (int sh = 1; sh <= 2; sh++)
				{
					Gdiplus::GraphicsPath sp;
					GdipAddRoundRect(sp, (float)sc.left, (float)sc.top + (float)sh * 1.5f, (float)sc.Width(), (float)sc.Height(), scR);
					gPaint.FillPath(&Gdiplus::SolidBrush(Gdiplus::Color(8, 0, 0, 0)), &sp);
				}
				Gdiplus::GraphicsPath cp;
				GdipAddRoundRect(cp, (float)sc.left, (float)sc.top, (float)sc.Width(), (float)sc.Height(), scR);
				// 배경색(#FAFBFD) 채우기
				gPaint.FillPath(&Gdiplus::SolidBrush(Gdiplus::Color(255, 250, 251, 253)), &cp);
				// [FIX] ShopSetupDlg와 동일하게 섹션 카드 외곽선(Pen) 렌더링 삭제
			};
		drawSecCard(portSection);
		drawSecCard(integritySection);
	}
	int hdrH = SX(bCP ? 36 : 44);

	// [FIX] 헤더 구분선을 정확히 헤더 하단에 위치시킵니다.
	memDC.FillSolidRect(portSection.left + SX(16), portSection.top + hdrH, portSection.Width() - SX(32), 1, RGB(242, 244, 247));
	memDC.FillSolidRect(integritySection.left + SX(16), integritySection.top + hdrH, integritySection.Width() - SX(32), 1, RGB(242, 244, 247));

	// [FIX] 개별 타이틀 좌표(sec1Pt) 대신 카드(Section)의 시작 좌표를 전달합니다.
	DrawSectionTitle(memDC, CPoint(portSection.left, portSection.top), _T("포트 설정"));
	DrawSectionTitle(memDC, CPoint(integritySection.left, integritySection.top), _T("무결성 체크 정보"));

	auto drawReaderCard = [&](const CRect& r, BOOL enabled, int num)
		{
			COLORREF bg = enabled ? RGB(255, 255, 255) : RGB(252, 253, 255);
			// [RESTORE] 리더기 내부 카드는 영역 구분을 위해 테두리(Border)를 다시 활성화합니다. (borderW = 1)
			COLORREF br = enabled ? KFTC_BORDER : RGB(232, 237, 244);
			FillRoundRect(gPaint, r, SX(10), bg, br, 1);

			const int badgeSize = SX(34);
			CRect badge(r.left + SX(bCP ? 13 : 16), r.top + SX(bCP ? 11 : 14), r.left + SX(bCP ? 13 : 16) + badgeSize, r.top + SX(bCP ? 11 : 14) + badgeSize);
			COLORREF badgeBg = enabled ? BLUE_500 : RGB(190, 199, 209);
			FillRoundRect(gPaint, badge, SX(6), badgeBg, badgeBg, 1);
			CString numText; numText.Format(_T("%d"), num);
			memDC.SelectObject(&m_fontNormal);
			memDC.SetTextColor(RGB(255, 255, 255));
			memDC.DrawText(numText, badge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			CString label; label.Format(_T("리더기%d - COM 포트"), num);
			memDC.SelectObject(&m_fontLabel);
			// [FIX] ShopSetupDlg 라벨과 완벽하게 동일한 색상 톤 적용
			memDC.SetTextColor(enabled ? RGB(115, 125, 142) : RGB(156, 163, 175));
				CRect rcLbl(r.left + SX(bCP ? 52 : 64), badge.top - SX(4), r.right - SX(8), r.top + SX(bCP ? 28 : 34) - SX(4));
				memDC.DrawText(label, rcLbl, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

			const int comboW = SX(bCP ? 152 : 178);
			const int btnH = SX(bCP ? 30 : 37);
			const int x0 = r.left + SX(bCP ? 52 : 64);
			const int yCombo = r.top + SX(bCP ? 28 : 34);
			const int toggleW = SX(bCP ? 44 : 52);
			const int padR = SX(22);
			const int textGap = SX(8);
			const int openLabelW = SX(56);
			const int multiLabelW = SX(92);
			const int togglePadX = r.right - padR - toggleW - SX(22);
			const int togglePadLabelX = togglePadX - textGap - multiLabelW;
			int toggleOpenX = togglePadLabelX - SX(26) - toggleW;
			if (toggleOpenX < x0 + comboW + SX(70))
				toggleOpenX = x0 + comboW + SX(70);
			const int xToggleOpenLabel = toggleOpenX - textGap - openLabelW;
			if (num == 1)
				memDC.TextOut(xToggleOpenLabel, yCombo + (btnH - SX(14)) / 2, _T("포트 열기"));
			memDC.TextOut(togglePadLabelX, yCombo + (btnH - SX(14)) / 2, _T("멀티패드 여부"));
		};

	drawReaderCard(card1, m_bReader1Enabled, 1);
	drawReaderCard(card2, m_bReader2Enabled, 2);

	// 기본 ListCtrl은 숨기고, 무결성 테이블 전체를 직접 그린다.
	// 헤더/본문/스크롤을 한 함수에서 맞춰 그려야 Modern 스타일이 유지된다.
	{
		CRect tableOuter = listRc;
		tableOuter.DeflateRect(1, 1);
		tableOuter.OffsetRect(0, -SX(bCP ? 0 : 14));
		const COLORREF tableBorder = KFTC_TBL_BORDER;
		const COLORREF headerBg    = KFTC_TBL_HDR_BG;
		const COLORREF headerLine  = KFTC_TBL_HDR_LINE;
		const COLORREF bodyLine    = KFTC_TBL_ROW_LINE;
		const COLORREF bodyBg      = KFTC_CARD_BG;
		const COLORREF altRowBg    = KFTC_TBL_ALT_ROW;
		const COLORREF headerText  = KFTC_TBL_HDR_TEXT;
		const COLORREF bodyText    = KFTC_TBL_BODY_TXT;
		const COLORREF emptyText   = KFTC_TBL_EMPTY_TXT;
		const COLORREF scrollTrack = KFTC_TBL_SCRL_TRK;
		const COLORREF scrollThumb = KFTC_TBL_SCRL_THB;

		FillRoundRect(gPaint, tableOuter, SX(8), bodyBg, tableBorder, 0);

		const int* ratios = kIntegrityColumnRatios;
		const int colCount = kIntegrityColumnCount;
		const int headerH = SX(bCP ? 30 : 36);
		const int rowH = SX(bCP ? 32 : 38);
		const int textPad = SX(10);
		const int visibleRows = GetIntegrityVisibleRows();

		CRect headerRc = tableOuter;
		headerRc.DeflateRect(1, 1);
		headerRc.bottom = headerRc.top + headerH;
		FillTopRoundRect(gPaint, headerRc, SX(8), headerBg);
		memDC.FillSolidRect(headerRc.left + SX(10), headerRc.bottom - 1, headerRc.Width() - SX(20), 1, RGB(238, 241, 247));

		const int itemCount = (m_bUiInitialized && m_integrity_list.GetSafeHwnd()) ? m_integrity_list.GetItemCount() : 0;
		const BOOL bNeedScroll = (itemCount > visibleRows);
		const int scrollW = bNeedScroll ? SX(10) : 0;
		const int scrollRightPad = bNeedScroll ? SX(2) : 0;

		int widths[colCount] = { 0, };
		int usableW = headerRc.Width() - (bNeedScroll ? (scrollW + SX(8) + scrollRightPad) : 0);
		int usedW = 0;
		for (int i = 0; i < colCount; ++i)
		{
			widths[i] = (i == colCount - 1) ? (usableW - usedW) : (usableW * ratios[i]) / 100;
			usedW += widths[i];
		}

		CRect bodyRc(tableOuter.left + 1, headerRc.bottom, tableOuter.right - 1, tableOuter.bottom - 1);
		memDC.FillSolidRect(bodyRc, bodyBg);
		CRect contentRc = bodyRc;
		contentRc.DeflateRect(SX(6), SX(bCP ? 2 : 6));
		if (bNeedScroll)
			contentRc.right -= (scrollW + SX(8) + scrollRightPad);

		int cellX = headerRc.left;
		for (int i = 0; i < colCount; ++i)
		{
			CRect headerCell(cellX, headerRc.top, cellX + widths[i], headerRc.bottom);
			CRect textRc = headerCell;
			textRc.DeflateRect(textPad, 0);
			memDC.SelectObject(&m_fontNormal);
			memDC.SetTextColor(headerText);
			memDC.DrawText(col_info[i].column_name, textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			if (i < colCount - 1)
				memDC.FillSolidRect(headerCell.right - 1, headerRc.top + SX(11), 1, headerRc.Height() - SX(22), headerLine);
			cellX += widths[i];
		}

		m_rcIntegrityScrollBar.SetRectEmpty();
		m_rcIntegrityScrollThumb.SetRectEmpty();
		NormalizeIntegrityScrollPos();

		if (m_bBusySearch)
		{
			const int skeletonH = SX(12);
			const int skeletonGapY = SX(14);
			const COLORREF skeletonBg = RGB(241, 244, 248);
			const COLORREF skeletonHl = RGB(228, 234, 241);
			for (int row = 0; row < visibleRows; ++row)
			{
				CRect rowRc(contentRc.left, contentRc.top + row * rowH, contentRc.right, contentRc.top + (row + 1) * rowH);
				if (rowRc.bottom > contentRc.bottom)
					rowRc.bottom = contentRc.bottom;
				int drawX = bodyRc.left;
				for (int col = 0; col < colCount; ++col)
				{
					CRect cellRc(drawX, rowRc.top, drawX + widths[col], rowRc.bottom);
					CRect skRc = cellRc;
					skRc.DeflateRect(textPad, (rowH - skeletonH) / 2 + skeletonGapY / 4);
					if (skRc.Width() > SX(12))
						skRc.right = skRc.left + max(SX(20), min(skRc.Width(), (col == 0) ? SX(100) : (cellRc.Width() * 3 / 5)));
					FillRoundRect(gPaint, skRc, SX(4), skeletonBg, skeletonBg, 1);
					drawX += widths[col];
				}
			}
			memDC.SelectObject(&m_fontSmall);
			memDC.SetTextColor(RGB(120, 130, 146));
			CRect loadingTextRc = bodyRc;
			loadingTextRc.top = contentRc.bottom - SX(30);
			memDC.DrawText(_T("조회 중입니다..."), loadingTextRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
		else if (itemCount <= 0)
		{
			memDC.SelectObject(&m_fontSmall);
			memDC.SetTextColor(emptyText);
			memDC.DrawText(_T("조회된 무결성 체크 정보가 없습니다."), bodyRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
		else
		{
			for (int row = 0; row < visibleRows; ++row)
			{
				const int dataRow = m_nIntegrityScrollPos + row;
				if (dataRow >= itemCount)
					break;
				CRect rowRc(contentRc.left, contentRc.top + row * rowH, contentRc.right, contentRc.top + (row + 1) * rowH);
				if (rowRc.bottom > contentRc.bottom)
					rowRc.bottom = contentRc.bottom;
				if (dataRow % 2 == 1)
					memDC.FillSolidRect(rowRc, altRowBg);
				if (row < visibleRows - 1 && rowRc.bottom < contentRc.bottom)
					memDC.FillSolidRect(rowRc.left + SX(8), rowRc.bottom - 1, rowRc.Width() - SX(16), 1, bodyLine);
				int drawX = bodyRc.left;
				for (int col = 0; col < colCount; ++col)
				{
					CRect cellRc(drawX, rowRc.top, drawX + widths[col], rowRc.bottom);
					CString cellText = m_integrity_list.GetItemText(dataRow, col);
					CRect drawRc = cellRc;
					drawRc.DeflateRect(textPad, 0);
					if (col == 2) // 결과 컬럼 (Index 2)
					{
						CRect chipRc = cellRc;
						// 좌우 여백을 SX(4)로 줄여서 "00" 같은 숫자가 꽉 차게 보이도록 설정
						chipRc.DeflateRect(SX(4), SX(7));

						// "00"이면 녹색 계열, 그 외 모든 숫자는 빨간색 계열 테마
						BOOL bNormal = (cellText == _T("00"));
						COLORREF bgColor = bNormal ? RGB(232, 252, 241) : RGB(255, 235, 235);
						COLORREF txtColor = bNormal ? RGB(21, 128, 61) : RGB(220, 38, 38);

						// 배지 배경 그리기 (곡률 6px)
						FillRoundRect(gPaint, chipRc, SX(6), bgColor, bgColor, 0);

						memDC.SelectObject(&m_fontSmall);
						memDC.SetTextColor(txtColor);
						// cellText를 그대로 사용하여 "00", "04" 등이 그대로 출력됨
						memDC.DrawText(cellText, chipRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
					}
					else
					{
						// 나머지 컬럼은 기존처럼 일반 텍스트로 출력
						memDC.SelectObject(&m_fontSmall);
						memDC.SetTextColor(bodyText);
						memDC.DrawText(cellText, drawRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
					}
					drawX += widths[col];
				}
			}
			if (bNeedScroll)
			{
				const int scrollLeftGap = SX(4);
				m_rcIntegrityScrollBar = CRect(contentRc.right + scrollLeftGap, contentRc.top,
					contentRc.right + scrollLeftGap + scrollW, contentRc.top + rowH * visibleRows);
				m_rcIntegrityScrollBar.right -= scrollRightPad;
				const int maxScroll = itemCount - visibleRows;
				int thumbH = (m_rcIntegrityScrollBar.Height() * visibleRows) / itemCount;
				if (thumbH < SX(18)) thumbH = SX(18);
				if (thumbH > m_rcIntegrityScrollBar.Height()) thumbH = m_rcIntegrityScrollBar.Height();
				int thumbTravel = max(0, m_rcIntegrityScrollBar.Height() - thumbH);
				int thumbTop = m_rcIntegrityScrollBar.top;
				if (maxScroll > 0 && thumbTravel > 0)
					thumbTop += (thumbTravel * m_nIntegrityScrollPos) / maxScroll;
				FillRoundRect(gPaint, m_rcIntegrityScrollBar, SX(4), scrollTrack, scrollTrack, 1);
				m_rcIntegrityScrollThumb = CRect(m_rcIntegrityScrollBar.left, thumbTop, m_rcIntegrityScrollBar.right, thumbTop + thumbH);
				FillRoundRect(gPaint, m_rcIntegrityScrollThumb, SX(4), scrollThumb, scrollThumb, 1);
			}
		}

		// 마지막에 라운드 외곽선을 다시 그려 모서리/상단 프레임이 배경에 묻히지 않도록 한다.
		DrawRoundRectBorder(gPaint, tableOuter, SX(8), tableBorder, 1);
		DrawRoundRectBorder(gPaint, CRect(tableOuter.left + 1, tableOuter.top + 1, tableOuter.right - 1, tableOuter.bottom - 1), SX(7), RGB(232, 237, 244), 1);
	}
	dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);
	memDC.SelectObject(oldBmp);
}



BOOL CReaderSetupDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT nID = LOWORD(wParam);
	int  nCode = (int)(short)HIWORD(wParam);

	// CSkinnedComboBox는 ON_CONTROL_REFLECT(CBN_SELCHANGE) 때문에
	// 부모 다이얼로그의 메시지맵보다 먼저 알림을 소비한다.
	// 따라서 부모가 반드시 받아야 하는 포트 변경 알림은 OnCommand에서 선점한다.
	if (nCode == CBN_SELCHANGE)
	{
		if (nID == IDC_COMPORT1) { OnSelchangeComport1(); return TRUE; }
		if (nID == IDC_COMPORT2) { OnSelchangeComport2(); return TRUE; }
	}

	if (nCode == BN_CLICKED)
	{
		switch (nID)
		{
		case IDC_READER_INIT1:     OnReaderInit(1);    return TRUE;
		case IDC_READER_INIT2:     OnReaderInit(2);    return TRUE;
		case IDC_STATUS_CHECK1:    OnStatusCheck(1);   return TRUE;
		case IDC_STATUS_CHECK2:    OnStatusCheck(2);   return TRUE;
		case IDC_KEYDOWN1:         OnKeyDown(1);       return TRUE;
		case IDC_KEYDOWN2:         OnKeyDown(2);       return TRUE;
		case IDC_INTEGRITY_CHECK1: OnIntegrityCheck(1); return TRUE;
		case IDC_INTEGRITY_CHECK2: OnIntegrityCheck(2); return TRUE;
		case IDC_UPDATE1:          OnUpdate(1);        return TRUE;
		case IDC_UPDATE2:          OnUpdate(2);        return TRUE;
		case IDC_SEARCH:           OnSearch(TRUE);         return TRUE;
		}
	}

	return CDialog::OnCommand(wParam, lParam);
}

void CReaderSetupDlg::OnSelchangeComport1()
{
	CString value;
	m_comport1.GetWindowText(value);
	UpdateReaderEnableState(1);
    ApplyAopRestrictions();
}

void CReaderSetupDlg::OnSelchangeComport2()
{
	CString value;
	m_comport2.GetWindowText(value);
	UpdateReaderEnableState(2);
}



void CReaderSetupDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_nLoadingAnimTimerID)
	{
		if (m_nLoadingButtonID != 0)
		{
			if (CWnd* pWnd = GetDlgItem(m_nLoadingButtonID))
			{ static_cast<CModernButton*>(pWnd)->DrawLoadingFrame(); }
			if (m_bBusySearch)
				Invalidate(FALSE);
		}
		return;
	}
	if (nIDEvent == m_nLoadingTimerID)
	{
		FinishLoadingOperation(TRUE);
		return;
	}
	if (nIDEvent == m_nPortOpenTimerID)
	{
		KillTimer(m_nPortOpenTimerID);
		m_nPortOpenTimerID = 0;
		::PostMessage(m_hWnd, WM_PORT_OPEN_DONE, FALSE, (LPARAM)FALSE);  // timeout -> failure
		return;
	}
	CDialog::OnTimer(nIDEvent);
}

// ============================================================================
// OnCtlColor - label/static transparent bg, dialog background brush
// ============================================================================
HBRUSH CReaderSetupDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	if (nCtlColor == CTLCOLOR_STATIC && pWnd)
	{
		pDC->SetBkMode(TRANSPARENT);
		// [FIX] ShopSetupDlg와 동일하게 시각적 무게감을 줄인 soft label gray 적용
		pDC->SetTextColor(RGB(115, 125, 142));
		return m_brushBg;
	}
	if (nCtlColor == CTLCOLOR_DLG)
		return m_brushBg;
	return hbr;
}
// ============================================================================
// DrawSectionTitle - section accent bar + title text (matches ShopSetupDlg DrawGroupLabels)
// ============================================================================
void CReaderSetupDlg::DrawSectionTitle(CDC& dc, CPoint pt, LPCTSTR text)
{
	// pt는 이제 카드의 좌상단(Left, Top) 좌표입니다.
	int hdrH = SX(IsCompactScreen() ? 36 : 44);

	Gdiplus::Graphics gSec(dc.GetSafeHdc());
	gSec.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	gSec.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

	const float barW = 4.0f;
	const float barH = 14.0f;
	const float barR = 2.0f;
	const float bd = barR * 2.0f;

	// [FIX] ShopSetupDlg와 동일하게 Bar 여백 동기화 (좌측 16px, 수직 중앙 정렬)
	const float barX = (float)pt.x + (float)SX(16);
	const float barY = (float)pt.y + ((float)hdrH - barH) * 0.5f;

	Gdiplus::GraphicsPath bp;
	bp.AddArc(barX, barY, bd, bd, 180, 90);
	bp.AddArc(barX + barW - bd, barY, bd, bd, 270, 90);
	bp.AddArc(barX + barW - bd, barY + barH - bd, bd, bd, 0, 90);
	bp.AddArc(barX, barY + barH - bd, bd, bd, 90, 90);
	bp.CloseFigure();

	// [FIX] ShopSetupDlg의 메인 테마 파란색 바 색상(RGB 0, 96, 210)으로 통일
	Gdiplus::SolidBrush barBr(Gdiplus::Color(255, 0, 96, 210));
	gSec.FillPath(&barBr, &bp);

	CFont* pOldSec = dc.SelectObject(&m_fontSection);
	dc.SetTextColor(RGB(26, 32, 44));

	// [FIX] 텍스트 역시 Bar 우측 6px 부터 시작하고, hdrH 안에서 수직 중앙 정렬
	CRect rcSec(pt.x + SX(16) + (int)barW + SX(6), pt.y, pt.x + SX(300), pt.y + hdrH);
	dc.DrawText(text, rcSec, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	dc.SelectObject(pOldSec);
}
BOOL CReaderSetupDlg::DestroyWindow()
{
	FinishLoadingOperation(FALSE);
	if (m_brushBg.GetSafeHandle()) m_brushBg.DeleteObject();
	if (m_popover.GetSafeHwnd()) m_popover.Hide();
	delete m_pGdipSecFont;   m_pGdipSecFont = nullptr;
	delete m_pGdipSecFamily; m_pGdipSecFamily = nullptr;
	delete m_pGdipHdrTitleFont; m_pGdipHdrTitleFont = nullptr;
	delete m_pGdipHdrSubFont;   m_pGdipHdrSubFont = nullptr;
	return CDialog::DestroyWindow();
}

void CReaderSetupDlg::ShowInfoPopover(CInfoIconButton& btn, LPCTSTR szTitle, LPCTSTR szBody)
{
	if (m_popover.IsVisible()) { m_popover.Hide(); return; }
	CRect rc; btn.GetWindowRect(&rc);
	m_popover.ShowAt(rc, szTitle, szBody, this);
}

void CReaderSetupDlg::OnBnClickedPortOpenInfo()
{
	ShowInfoPopover(m_btnPortOpenInfo, _T("포트 열기"), _T("포트 열기 사용 여부\n· ON : 리더기 포트 항시 연결\n· OFF : 거래시에만 리더기 포트 연결\n※ON의 경우 결제 속도 향상\n※리더기1만 사용하는 경우에만 ON 가능"));
}

void CReaderSetupDlg::OnBnClickedMultipad1Info()
{
	ShowInfoPopover(m_btnMultipad1Info, _T("멀티패드 여부"), _T("멀티패드 여부 설정\n· ON : QR 기능 사용 가능\n· OFF : QR 기능 사용 불가\n※스캐너가 부착된 리더기의 경우에도 ON 설정"));
}

void CReaderSetupDlg::OnBnClickedMultipad2Info()
{
	ShowInfoPopover(m_btnMultipad2Info, _T("멀티패드 여부"), _T("멀티패드 여부 설정\n· ON : QR 기능 사용 가능\n· OFF : QR 기능 사용 불가\n※스캐너가 부착된 리더기의 경우에도 ON 설정"));
}


// ============================================================
// Button action handlers - TODO: implement each function
// ============================================================

void CReaderSetupDlg::OnReaderInit(int readerIndex)
{
	StartLoadingOperation((readerIndex == 1) ? IDC_READER_INIT1 : IDC_READER_INIT2);

	// 포트 가져오기
	CString port;
	if (readerIndex == 1)
		m_comport1.GetWindowText(port);
	else
		m_comport2.GetWindowText(port);

	// TODO: port 사용해서 리더기 통신
}

void CReaderSetupDlg::OnStatusCheck(int readerIndex)
{
	StartLoadingOperation((readerIndex == 1) ? IDC_STATUS_CHECK1 : IDC_STATUS_CHECK2);
	// TODO: readerIndex 1 or 2
}

void CReaderSetupDlg::OnKeyDown(int readerIndex)
{
	StartLoadingOperation((readerIndex == 1) ? IDC_KEYDOWN1 : IDC_KEYDOWN2);
	// TODO: readerIndex 1 or 2
}

void CReaderSetupDlg::OnIntegrityCheck(int readerIndex)
{
	StartLoadingOperation((readerIndex == 1) ? IDC_INTEGRITY_CHECK1 : IDC_INTEGRITY_CHECK2);
	// TODO: readerIndex 1 or 2
}

void CReaderSetupDlg::OnUpdate(int readerIndex)
{
	StartLoadingOperation((readerIndex == 1) ? IDC_UPDATE1 : IDC_UPDATE2);
	// TODO: readerIndex 1 or 2
}

void CReaderSetupDlg::OnSearch(BOOL isLoading)
{
	if (isLoading)
		StartLoadingOperation(IDC_SEARCH);

	m_integrity_list.DeleteAllItems();

	// Determine sample count based on selected period
	int nSel = (m_search_date.GetSafeHwnd()) ? m_search_date.GetCurSel() : 0;
	int nCount = 3;           // index 0: 1-day  -> 3 rows
	if (nSel == 1) nCount = 5;            // index 1: 7-day  -> 5 rows
	else if (nSel >= 2) nCount = 10;      // index 2: 30-day, index 3: 100-day -> 10 rows

	const TCHAR* rows[][6] =
	{
		{ _T("20260308091234"), _T("COM 01"), _T("00"), _T("RDR-1001"), _T("##SPAY-8800Q3001"),  _T("KFTCONECAP3001") },
		{ _T("20260308091234"), _T("COM 01"), _T("04"), _T("RDR-1001"), _T("##SPAY-8800Q3001"),  _T("KFTCONECAP3001") },
		{ _T("20260308091234"), _T("COM 02"), _T("00"), _T("RDR-2003"), _T("DAULPAY633RDK201"),  _T("KFTCONECAP3001") },
		{ _T("20260308091234"), _T("COM 03"), _T("00"), _T("RDR-1010"), _T("DAULPAY633RDK201"),  _T("KFTCONECAP3001") },
		{ _T("20260308091234"), _T("COM 01"), _T("00"), _T("RDR-1001"), _T("##SPAY-8800Q3001"),  _T("KFTCONECAP3001") },
		{ _T("20260309154312"), _T("COM 04"), _T("00"), _T("RDR-3011"), _T("##SPAY-8800Q3001"),  _T("KFTCONECAP3001") },
		{ _T("20260310143022"), _T("COM 02"), _T("00"), _T("RDR-2003"), _T("DAULPAY633RDK201"),  _T("KFTCONECAP3001") },
		{ _T("20260311083045"), _T("COM 01"), _T("00"), _T("RDR-1001"), _T("##SPAY-8800Q3001"),  _T("KFTCONECAP3001") },
		{ _T("20260312120011"), _T("COM 03"), _T("01"), _T("RDR-1010"), _T("DAULPAY633RDK201"),  _T("KFTCONECAP3001") },
		{ _T("20260313175533"), _T("COM 04"), _T("00"), _T("RDR-3011"), _T("##SPAY-8800Q3001"),  _T("KFTCONECAP3001") },
	};

	for (int r = 0; r < nCount; ++r)
	{
		int idx = m_integrity_list.InsertItem(r, rows[r][0]);
		for (int c = 1; c < 6; ++c)
			m_integrity_list.SetItemText(idx, c, rows[r][c]);
	}

	m_nIntegrityScrollPos = 0;
	NormalizeIntegrityScrollPos();
	Invalidate(FALSE);

	if (isLoading)
	{
		::Sleep(2000); // simulate search delay
		SendMessage(WM_READER_DONE, TRUE, IDC_SEARCH);
	}
}
void CReaderSetupDlg::OnOK()
{
	CString port1, port2;
	m_comport1.GetWindowText(port1);
	m_comport2.GetWindowText(port2);
	AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, COMPORT1_FIELD, port1);
	AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, COMPORT2_FIELD, port2);
	// Save multipad toggle state: "0" = ON, "1" = OFF
	AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, MULTIPAD1_FIELD, m_toggleMultipad1.IsToggled() ? _T("0") : _T("1"));
	AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, MULTIPAD2_FIELD, m_toggleMultipad2.IsToggled() ? _T("0") : _T("1"));
	CDialog::OnOK();
}

void CReaderSetupDlg::OnCancel()
{
	if (m_popover.GetSafeHwnd()) m_popover.Hide();
	if (HasChanges())
	{
		if (CModernMessageBox::Question(_T("변경된 내용이 있습니다.저장하지 않고 종료하시겠습니까?"), this) != IDYES)
			return;
	}
	CDialog::OnCancel();
}
void CReaderSetupDlg::TakeSnapshot()
{
	m_snap.cmbPort1     = m_comport1.GetCurSel();
	m_snap.cmbPort2     = m_comport2.GetCurSel();
	m_snap.tglPortOpen1 = m_togglePortOpen1.IsToggled();
	m_snap.tglPortOpen2 = m_togglePortOpen2.IsToggled();
	m_snap.tglMultipad1 = m_toggleMultipad1.IsToggled();
	m_snap.tglMultipad2 = m_toggleMultipad2.IsToggled();
}

BOOL CReaderSetupDlg::HasChanges() const
{
	if (m_comport1.GetCurSel()           != m_snap.cmbPort1)     return TRUE;
	if (m_comport2.GetCurSel()           != m_snap.cmbPort2)     return TRUE;
	if (m_toggleMultipad1.IsToggled()  != m_snap.tglMultipad1) return TRUE;
	if (m_toggleMultipad2.IsToggled()  != m_snap.tglMultipad2) return TRUE;
	return FALSE;
}

void CReaderSetupDlg::ApplyAopRestrictions()
{
    CString interlock = AfxGetApp()->GetProfileString(_T("SERIALPORT"), _T("INTERLOCK"), _T(""));
    if (interlock != _T("AOP")) return;

    // Disable all controls of reader 2
    ApplyEnableStateToButtons(2, FALSE);
    m_comport2.EnableWindow(FALSE);

    // Disable reader 1 init and update buttons
    m_reader_init1.EnableWindow(FALSE);
    m_update1.EnableWindow(FALSE);
    m_togglePortOpen1.EnableWindow(FALSE);

    Invalidate(FALSE);
}
