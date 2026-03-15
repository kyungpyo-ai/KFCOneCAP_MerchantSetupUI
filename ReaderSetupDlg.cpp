// ReaderSetupDlg.cpp : implementation file
//

#include "stdafx.h"
#include "common.h"
#include "resource.h"
#include "ReaderSetupDlg.h"

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

static void GdipAddRoundRect(Gdiplus::GraphicsPath& p, float x, float y, float w, float h, float r)
{
	float d = r * 2.f;
	Gdiplus::RectF a(x, y, d, d);
	p.AddArc(a, 180.f, 90.f); a.X = x + w - d;
	p.AddArc(a, 270.f, 90.f); a.Y = y + h - d;
	p.AddArc(a, 0.f, 90.f); a.X = x;
	p.AddArc(a, 90.f, 90.f); p.CloseFigure();
}

static void FillRoundRect(Gdiplus::Graphics& g, const CRect& rc, int radius, COLORREF fill, COLORREF border, int borderW = 1)
{
	float x = (float)rc.left, y = (float)rc.top;
	float w = (float)rc.Width(), h = (float)rc.Height();
	float r = (float)radius;

	{
		Gdiplus::GraphicsPath path;
		GdipAddRoundRect(path, x, y, w, h, r);
		Gdiplus::SolidBrush br(Gdiplus::Color(255, GetRValue(fill), GetGValue(fill), GetBValue(fill)));
		g.FillPath(&br, &path);
	}
	if (borderW > 0)
	{
		Gdiplus::GraphicsPath path;
		float bw = (float)borderW;
		GdipAddRoundRect(path, x + bw * 0.5f, y + bw * 0.5f, w - bw, h - bw, r);
		Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(border), GetGValue(border), GetBValue(border)), bw);
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
	GdipAddRoundRect(path, (float)rc.left + bw * 0.5f, (float)rc.top + bw * 0.5f,
		(float)rc.Width() - bw, (float)rc.Height() - bw, (float)radius);
	Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(border), GetGValue(border), GetBValue(border)), bw);
	g.DrawPath(&pen, &path);
}
int CReaderSetupDlg::SX(int v) const
{
	return ModernUIDpi::Scale(m_hWnd, v);
}

void CReaderSetupDlg::EnsureFonts()
{
	if (m_fontTitle.GetSafeHandle())
		return;

	LOGFONT lf = { 0 };
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);

	// Title
	lf.lfHeight = -SX(18);
	lf.lfWeight = FW_BOLD;
	ModernUIFont::ApplyUIFontFace(lf);
	m_fontTitle.CreateFontIndirect(&lf);

	// Sub
	lf.lfHeight = -SX(13);
	lf.lfWeight = FW_NORMAL;
	m_fontSub.CreateFontIndirect(&lf);

	// Section title
	lf.lfHeight = -SX(15);
	lf.lfWeight = FW_BOLD;
	m_fontSection.CreateFontIndirect(&lf);

	// Normal
	lf.lfHeight = -SX(14);
	lf.lfWeight = FW_NORMAL;
	m_fontNormal.CreateFontIndirect(&lf);

	// Label
	lf.lfHeight = -SX(14);
	lf.lfWeight = FW_NORMAL;
	m_fontLabel.CreateFontIndirect(&lf);

	// Small
	lf.lfHeight = -SX(12);
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

	const int margin = SX(20);
	inner = CRect(rc.left + margin, rc.top + margin, rc.right - margin, rc.bottom - margin);

	const int titleBlock = SX(84);
	const int sectionTitleH = SX(28);
	const int sectionTitleTop = SX(12);
	const int infoSectionTitleTop = SX(2);
	const int sectionTitleGap = SX(10);
	const int infoTitleGap = -SX(10);
	const int sectionBoxPad = SX(20);
	const int cardH = SX(128);
	const int cardGap = SX(16);
	const int queryH = SX(56);
	const int queryGap = SX(16);
	const int infoBottomPad = SX(8);
	const int bottomBtnH = SX(42);
	const int bottomGap = SX(16);
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
	y = card2.bottom + sectionBoxPad + SX(30);

	sec2TitlePt = CPoint(sectionLeft + SX(24), y + infoSectionTitleTop);
	int sec2ContentTop = y + sectionTitleTop + sectionTitleH + infoTitleGap;

	queryBox = CRect(sectionLeft + sectionBoxPad, sec2ContentTop,
		sectionRight - sectionBoxPad, sec2ContentTop + queryH);
	y = queryBox.bottom + queryGap;

	int listTop = y;
	int listBottomLimit = inner.bottom - bottomArea - infoBottomPad - sectionBoxPad;
	int listH = listBottomLimit - listTop;
	if (listH < SX(166)) listH = SX(166);
	if (listH > SX(166)) listH = SX(166);
	listRc = CRect(sectionLeft + sectionBoxPad, listTop,
		sectionRight - sectionBoxPad, listTop + listH);

	// ShopSetupDlg 하단 버튼과 동일한 크기/간격/위치 규칙 적용
	// ShopSetupDlg와 동일한 하단 버튼 규칙 적용
	const int buttonW = SX(110);
	const int buttonH = SX(36);
	const int buttonGap = SX(8);
	const int buttonBottom = SX(18);
	const int dialogBottomPad = SX(22);
	int totalW = buttonW * 2 + buttonGap;
	int bx = (rc.Width() - totalW) / 2;
	int by = rc.bottom - (dialogBottomPad + buttonBottom + buttonH);

	okRc = CRect(bx, by, bx + buttonW, by + buttonH);
	cancelRc = CRect(bx + buttonW + buttonGap, by, bx + buttonW + buttonGap + buttonW, by + buttonH);
	infoTitleArea = CRect(sec2TitlePt.x, sec2TitlePt.y, inner.right, sec2TitlePt.y + sectionTitleH);
}

CRect CReaderSetupDlg::CalcPortSectionBox(const CRect& card1, const CRect& card2) const
{
	return CRect(card1.left - SX(24), card1.top - SX(56), card1.right + SX(24), card2.bottom + SX(18));
}

CRect CReaderSetupDlg::CalcIntegritySectionBox(const CRect& queryBox, const CRect& listRc) const
{
	return CRect(queryBox.left - SX(24), queryBox.top - SX(48), queryBox.right + SX(24), listRc.bottom + SX(2));
}

// 데이터 저장용 ListCtrl의 컬럼 폭도 커스텀 테이블 비율과 동일하게 맞춘다.
// 실제 화면은 OnPaint가 그리더라도, 데이터 접근 시 컬럼 정의가 일관돼야 유지보수가 쉽다.
void CReaderSetupDlg::RecalcIntegrityColumns()
{
	if (!m_bUIReady || !::IsWindow(m_integrity_list.GetSafeHwnd()))
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
			const int trackH = m_rcIntegrityScrollBar.Height();
			const int thumbH = m_rcIntegrityScrollThumb.Height();
			const int thumbTravel = max(1, trackH - thumbH);
			const int maxScroll = itemCount - visibleRows;
			int relY = point.y - m_rcIntegrityScrollBar.top - thumbH / 2;
			if (relY < 0) relY = 0;
			if (relY > thumbTravel) relY = thumbTravel;
			m_nIntegrityScrollPos = (maxScroll * relY) / thumbTravel;
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

void CReaderSetupDlg::ApplyEnableStateToButtons(int readerIndex, BOOL bEnable)
{
	const COLORREF cardBgEnabled = RGB(255, 255, 255);
	const COLORREF cardBgDisabled = RGB(251, 252, 253);
	const COLORREF cardBg = bEnable ? cardBgEnabled : cardBgDisabled;

	if (readerIndex == 1)
	{
		m_reader_init1.EnableWindow(bEnable);
		m_status_check1.EnableWindow(bEnable);
		m_keydown1.EnableWindow(bEnable);
		m_integrity_check1.EnableWindow(bEnable);
		m_update1.EnableWindow(bEnable);
		m_togglePortOpen1.EnableWindow(bEnable);
		m_toggleMultipad1.EnableWindow(bEnable);
		m_bReader1Enabled = bEnable;

		m_reader_init1.SetUnderlayColor(cardBg);
		m_status_check1.SetUnderlayColor(cardBg);
		m_keydown1.SetUnderlayColor(cardBg);
		m_integrity_check1.SetUnderlayColor(cardBg);
		m_update1.SetUnderlayColor(cardBg);
		m_togglePortOpen1.SetUnderlayColor(cardBg);
		m_toggleMultipad1.SetUnderlayColor(cardBg);
		m_comport1.SetUnderlayColor(cardBg);
		m_comport1.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME);
	}
	else
	{
		m_reader_init2.EnableWindow(bEnable);
		m_status_check2.EnableWindow(bEnable);
		m_keydown2.EnableWindow(bEnable);
		m_integrity_check2.EnableWindow(bEnable);
		m_update2.EnableWindow(bEnable);
		m_togglePortOpen2.EnableWindow(bEnable);
		m_toggleMultipad2.EnableWindow(bEnable);
		m_bReader2Enabled = bEnable;

		m_reader_init2.SetUnderlayColor(cardBg);
		m_status_check2.SetUnderlayColor(cardBg);
		m_keydown2.SetUnderlayColor(cardBg);
		m_integrity_check2.SetUnderlayColor(cardBg);
		m_update2.SetUnderlayColor(cardBg);
		m_togglePortOpen2.SetUnderlayColor(cardBg);
		m_toggleMultipad2.SetUnderlayColor(cardBg);
		m_comport2.SetUnderlayColor(cardBg);
		m_comport2.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME);
	}
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
	if (!m_bUIReady) return;

	if (!::IsWindow(m_comport1.GetSafeHwnd()) ||
		!::IsWindow(m_reader_init1.GetSafeHwnd()) ||
		!::IsWindow(m_integrity_list.GetSafeHwnd()))
		return;

	CRect inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc;
	CPoint sec1Pt, sec2Pt;
	CalcLayoutRects(inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc, sec1Pt, sec2Pt);

	const int padL = SX(64);
	const int padR = SX(22);
	const int comboW = SX(178);
	const int btnW = SX(kReaderActionButtonWidth);
	const int btnH = SX(37);
	const int gap = SX(8);
	const int toggleW = SX(52);
	const int toggleH = SX(28);
	const int rowComboY = SX(34);
	const int rowBtnY = SX(84);
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
		cb.SendMessage(CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)(SX(37) - 2));
		cb.SendMessage(CB_SETITEMHEIGHT, (WPARAM)0,  (LPARAM)(SX(37) - 2));

			int xTogglePad = card.right - padR - toggleW - SX(22);
			int xTogglePadLabel = xTogglePad - textGap - multiLabelW;
			int xToggleOpen = xTogglePadLabel - toggleBlockGap - (toggleW + textGap + openLabelW);
			if (xToggleOpen < x0 + comboW + SX(16))
				xToggleOpen = x0 + comboW + SX(16);
			int yToggle = yCombo + (SX(37) - toggleH) / 2;
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
	if (m_bUIReady) {
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
	int qComboW = SX(120);
	CRect rcSearchCombo;
	m_search_date.GetWindowRect(&rcSearchCombo);
	int qVisibleH = SX(37);
	int qy = queryBox.top + (queryBox.Height() - qVisibleH) / 2 + SX(2);
	m_search_date.SetWindowPos(NULL, qx, qy, qComboW, SX(220), SWP_NOZORDER | SWP_NOACTIVATE);
	m_search_date.SendMessage(CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)(SX(37) - 2));
	m_search_date.SendMessage(CB_SETITEMHEIGHT, (WPARAM)0,  (LPARAM)(SX(37) - 2));
	m_btnSearch.SetWindowPos(NULL, qx + qComboW + SX(12), qy, SX(78), qVisibleH, SWP_NOZORDER | SWP_NOACTIVATE);

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
	m_bUIReady = FALSE;
	m_bFitDone = FALSE;
	m_bReader1Enabled = FALSE;
	m_bReader2Enabled = FALSE;
	m_nIntegrityScrollPos = 0;
	m_nLoadingButtonID = 0;
	m_nLoadingTimerID = 0;
	m_nLoadingAnimTimerID = 0;
	m_nBusyReaderIndex = 0;
	m_bBusySearch = FALSE;
	m_rcIntegrityScrollBar.SetRectEmpty();
	m_rcIntegrityScrollThumb.SetRectEmpty();
	m_pGdipSecFamily = nullptr;
	m_pGdipSecFont = nullptr;
	m_pGdipHdrTitleFont = nullptr;
	m_pGdipHdrSubFont = nullptr;
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
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_DRAWITEM()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_PORT_OPEN_INFO,  OnBnClickedPortOpenInfo)
	ON_BN_CLICKED(IDC_BTN_MULTIPAD1_INFO,  OnBnClickedMultipad1Info)
	ON_BN_CLICKED(IDC_BTN_MULTIPAD2_INFO,  OnBnClickedMultipad2Info)
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
		UpdateReaderEnableState(readerIndex);
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
	
	m_comport1.SetTextPx(14);
	m_comport2.SetTextPx(14);
	ApplyFontIfWindow(m_search_date, m_fontNormal);
	m_search_date.SetTextPx(14);
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
	m_reader_init1.SetButtonStyle(ButtonStyle::Download);
	m_status_check1.SetButtonStyle(ButtonStyle::Download);
	m_keydown1.SetButtonStyle(ButtonStyle::Download);
	m_integrity_check1.SetButtonStyle(ButtonStyle::Download);
	m_update1.SetButtonStyle(ButtonStyle::Download);
	m_reader_init2.SetButtonStyle(ButtonStyle::Download);
	m_status_check2.SetButtonStyle(ButtonStyle::Download);
	m_keydown2.SetButtonStyle(ButtonStyle::Download);
	m_integrity_check2.SetButtonStyle(ButtonStyle::Download);
	m_update2.SetButtonStyle(ButtonStyle::Download);

	if (CWnd* pOK = GetDlgItem(IDOK))
		m_btnOk.SubclassDlgItem(IDOK, this);
	if (CWnd* pCancel = GetDlgItem(IDCANCEL))
		m_btnCancel.SubclassDlgItem(IDCANCEL, this);
	m_btnOk.SetButtonStyle(ButtonStyle::Primary);
	m_btnCancel.SetButtonStyle(ButtonStyle::Default);

	m_btnSearch.SubclassDlgItem(IDC_SEARCH, this);
	m_btnSearch.SetButtonStyle(ButtonStyle::Download);

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

	m_togglePortOpen1.SetCheck(BST_UNCHECKED);
	m_togglePortOpen2.SetCheck(BST_UNCHECKED);
	m_toggleMultipad1.SetCheck(BST_UNCHECKED);
	m_toggleMultipad2.SetCheck(BST_UNCHECKED);
	// Setup text labels inside toggle controls (ShopSetupDlg style)
	m_togglePortOpen1.SetFont(&m_fontLabel);
	m_togglePortOpen1.SetWindowText(_T("포트 열기"));
	m_togglePortOpen1.SetTextSizePx(13);
	m_togglePortOpen1.SetNoWrapEllipsis(TRUE);
	m_toggleMultipad1.SetFont(&m_fontLabel);
	m_toggleMultipad1.SetWindowText(_T("멀티패드 여부"));
	m_toggleMultipad1.SetTextSizePx(13);
	m_toggleMultipad1.SetNoWrapEllipsis(TRUE);
	m_toggleMultipad2.SetFont(&m_fontLabel);
	m_toggleMultipad2.SetWindowText(_T("멀티패드 여부"));
	m_toggleMultipad2.SetTextSizePx(13);
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
		SetReaderCardBusy(m_nBusyReaderIndex, TRUE);
	}

	m_nLoadingAnimTimerID = 0x4810;
	m_nLoadingTimerID = 0x4811;
	SetTimer(m_nLoadingAnimTimerID, 33, NULL);
	SetTimer(m_nLoadingTimerID, 1200, NULL);
	Invalidate(FALSE);
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
		SetReaderCardBusy(m_nBusyReaderIndex, FALSE);

	m_nBusyReaderIndex = 0;
	m_nLoadingButtonID = 0;
	if (bRefresh)
		Invalidate(FALSE);
}

BOOL CReaderSetupDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// --- 1) GDI+/윈도우 기본 스타일 준비 ---
	ModernUIGfx::EnsureGdiplusStartup();
	ModifyStyle(0, WS_CLIPCHILDREN);

	// --- 2) DPI/폰트/숨길 레거시 Static 초기화 ---
	EnsureFonts();
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
	m_bUIReady = TRUE;

	// --- 5) 창 크기/컨트롤 배치 적용 ---
	FitWindowToLayout();
	LayoutControls();
	RecalcIntegrityColumns();

	// --- 6) 최종 상태 반영 ---
	UpdateReaderEnableState(1);
	UpdateReaderEnableState(2);

	// 초기 포커스는 확인 버튼으로 넘겨 포커스 링이 콤보에 남지 않게 한다.
	ModernUIWindow::ApplyWhiteTitleBar(this->GetSafeHwnd());
	TakeSnapshot();
	GetDlgItem(IDOK)->SetFocus();
	return FALSE;
}

CSize CReaderSetupDlg::CalcMinClientSize() const
{
	const int margin = SX(20);
	const int innerW = SX(720);
	const int titleArea = SX(92);
	const int sectionPad = SX(24);
	const int sectionTitleTop = SX(12);
	const int infoSectionTitleTop = SX(2);
	const int sectionTitleH = SX(28);
	const int sectionTitleGap = SX(10);
	const int infoTitleGap = -SX(10);
	const int cardH = SX(128);
	const int cardGap = SX(16);
	const int betweenSections = SX(26);
	const int queryH = SX(62);
	const int queryGap = SX(12);
	const int listMinH = SX(152);
	const int infoBottomPad = SX(18);
	const int bottomBtnH = SX(42);
	const int bottomGap = SX(16);
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

	SetWindowPos(NULL, 0, 0,
		rcWnd.Width(), rcWnd.Height(),
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	CenterWindow();
}

void CReaderSetupDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (!m_bUIReady)
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

	const int cardMarginL = SX(20);
	const int cardMarginT = SX(10);
	const int cardMarginR = SX(20);
	const int cardMarginB = SX(20);
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
	// 본체: 곡률 16px 적용 및 테두리 색상을 더 연하게(RGB 232, 235, 242) 변경
	FillRoundRect(gPaint, mainCard, SX(16), RGB(255, 255, 255), RGB(232, 235, 242), 1);

	const int iconSize = SX(38);
	const int iconX = mainCard.left + SX(6);
	const int iconY = mainCard.top + SX(18);
	CRect rcIcon(iconX, iconY, iconX + iconSize, iconY + iconSize);
	// GDI+ anti-aliased icon: card terminal (reader device - screen, keypad, card slot)
	{
		HDC hIco = memDC.GetSafeHdc();
		Gdiplus::Graphics gIco(hIco);
		gIco.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		const float bx = (float)iconX, by = (float)iconY, bsz = (float)iconSize;

		auto MRR = [](Gdiplus::GraphicsPath& p,
			float x, float y, float w, float h, float r) {
				float d = r * 2.f;
				Gdiplus::RectF a(x, y, d, d);
				p.AddArc(a, 180.f, 90.f); a.X = x + w - d;
				p.AddArc(a, 270.f, 90.f); a.Y = y + h - d;
				p.AddArc(a, 0.f, 90.f); a.X = x;
				p.AddArc(a, 90.f, 90.f); p.CloseFigure();
			};

		// gradient blue background
		{
			Gdiplus::GraphicsPath bp;
			MRR(bp, bx, by, bsz, bsz, 8.f);
			Gdiplus::LinearGradientBrush grad(
				Gdiplus::PointF(bx, by), Gdiplus::PointF(bx, by + bsz),
				Gdiplus::Color(255, 60, 130, 245),
				Gdiplus::Color(255, 28, 76, 210));
			gIco.FillPath(&grad, &bp);
		}

		// Card terminal icon: centered, all elements strictly inside body
		const float tW = bsz * 0.52f, tH = bsz * 0.60f;
		const float tX = bx + (bsz - tW) * 0.5f;
		const float tY = by + (bsz - tH) * 0.5f;
		{
			Gdiplus::GraphicsPath iconPath(Gdiplus::FillModeAlternate);
			MRR(iconPath, tX, tY, tW, tH, bsz * 0.09f);
			const float si = tW * 0.12f;
			iconPath.AddRectangle(Gdiplus::RectF(tX+si, tY+tH*0.08f, tW-si*2.0f, tH*0.26f));
			const float slot = (tW - tW*0.22f) / 3.0f;
			const float bW = slot * 0.64f, bH = tH * 0.09f;
			const float kX0 = tX + tW*0.11f + (slot - bW)*0.5f;
			const float kY0 = tY + tH*0.08f + tH*0.26f + tH*0.10f;
			for (int ki = 0; ki < 3; ki++)
				for (int kj = 0; kj < 2; kj++)
					MRR(iconPath, kX0+ki*slot, kY0+kj*tH*0.14f, bW, bH, 0.8f);
			Gdiplus::SolidBrush wb(Gdiplus::Color(255, 255, 255, 255));
			gIco.FillPath(&wb, &iconPath);
		}
		{
			Gdiplus::Pen slotPen(Gdiplus::Color(255, 255, 255, 255), 1.5f);
			slotPen.SetStartCap(Gdiplus::LineCapRound);
			slotPen.SetEndCap(Gdiplus::LineCapRound);
			gIco.DrawLine(&slotPen,
				Gdiplus::PointF(tX + tW*0.18f, tY + tH*0.91f),
				Gdiplus::PointF(tX + tW*0.82f, tY + tH*0.91f));
		}
	}

	{
// ShopSetup의 kHdrTitleGap(13)과 시각적 균형을 위해 SX(14)로 통일
		const int tx = rcIcon.right + SX(12);
// -22.0f에서 -20.0f로 수정하여 텍스트의 수직 중앙 정렬을 보정
		const int titleY = iconY + iconSize / 2 - SX(22);
		wchar_t wTitle[128] = {}, wSub[256] = {};
		MultiByteToWideChar(CP_ACP, 0, _T("리더기 설정"), -1, wTitle, 128);
		MultiByteToWideChar(CP_ACP, 0, _T("리더기 연결 및 제어 설정을 관리합니다"), -1, wSub, 256);
		CFont* pOldF = memDC.SelectObject(&m_fontTitle);
		memDC.SetTextColor(RGB(18, 24, 40));
		CRect rcHdrTitle(tx, titleY, tx + SX(300), titleY + SX(28));
		::DrawTextW(memDC.GetSafeHdc(), wTitle, -1, (LPRECT)&rcHdrTitle, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
		memDC.SelectObject(&m_fontSub);
		memDC.SetTextColor(RGB(130, 142, 162));
		CRect rcHdrSub(tx, titleY + SX(26), tx + SX(360), titleY + SX(46));
		::DrawTextW(memDC.GetSafeHdc(), wSub, -1, (LPRECT)&rcHdrSub, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
		memDC.SelectObject(pOldF);
	}
	// 시작점을 mainCard.left + SX(6)으로 변경하여 절대좌표 26px(아이콘 시작점)에 맞춤
	// 길이를 mainCard.Width() - SX(12)로 변경하여 우측 여백도 26px로 대칭을 맞춤
	memDC.FillSolidRect(mainCard.left + SX(6), mainCard.top + SX(74), mainCard.Width() - SX(12), 1, RGB(242, 244, 247));

	CRect inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc;
	CPoint sec1Pt, sec2Pt;
	CalcLayoutRects(inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc, sec1Pt, sec2Pt);
	CRect portSection = CalcPortSectionBox(card1, card2);
	CRect integritySection = CalcIntegritySectionBox(queryBox, listRc);

	auto drawSectionTitle = [&](const CPoint& pt, LPCTSTR text)
		{
			Gdiplus::Graphics gSec(memDC.GetSafeHdc());
			gSec.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
			gSec.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
			// Accent bar: 4x16px rounded rect (matches ShopSetupDlg DrawMinCard)
			const float barW = 4.0f, barH = 14.0f, barR = 2.0f, bd = barR * 2.0f;
			const float barX = (float)(pt.x - SX(10));
			const float barY = (float)pt.y + ((float)SX(28) - barH) * 0.5f;
			Gdiplus::GraphicsPath bp;
			bp.AddArc(barX, barY, bd, bd, 180, 90);
			bp.AddArc(barX + barW - bd, barY, bd, bd, 270, 90);
			bp.AddArc(barX + barW - bd, barY + barH - bd, bd, bd, 0, 90);
			bp.AddArc(barX, barY + barH - bd, bd, bd, 90, 90);
			bp.CloseFigure();
			Gdiplus::SolidBrush barBr(Gdiplus::Color(255, 0, 96, 210));
			gSec.FillPath(&barBr, &bp);
			// Section title text: GDI DrawText
			CFont* pOldSec = memDC.SelectObject(&m_fontSection);
			memDC.SetTextColor(RGB(26, 32, 44));
			CRect rcSec(pt.x, pt.y, pt.x + SX(300), pt.y + SX(28));
			memDC.DrawText(text, rcSec, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			memDC.SelectObject(pOldSec);
		};

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
				// 섹션 외곽선 추가 (더 깔끔한 경계선)
				Gdiplus::Pen secPen(Gdiplus::Color(255, 238, 241, 247), 1.0f);
				gPaint.DrawPath(&secPen, &cp);
		};
		drawSecCard(portSection);
		drawSecCard(integritySection);
	}
	// 섹션 내부 구분선 색상을 더 연하게 변경
	memDC.FillSolidRect(portSection.left + SX(16), portSection.top + SX(43), portSection.Width() - SX(32), 1, RGB(242, 244, 247));
	// 모든 내부 구분선을 연한 회색(RGB 242, 244, 247)으로 통일
	memDC.FillSolidRect(integritySection.left + SX(16), integritySection.top + SX(43), integritySection.Width() - SX(32), 1, RGB(242, 244, 247));
	drawSectionTitle(sec1Pt, _T("포트 설정"));
	drawSectionTitle(sec2Pt, _T("무결성 체크 정보"));

	auto drawReaderCard = [&](const CRect& r, BOOL enabled, int num)
		{
			COLORREF bg = enabled ? RGB(255, 255, 255) : RGB(252, 253, 255);
			COLORREF br = enabled ? RGB(214, 226, 246) : RGB(232, 237, 244);
			FillRoundRect(gPaint, r, SX(10), bg, br, 1);

			const int badgeSize = SX(34);
			CRect badge(r.left + SX(16), r.top + SX(14), r.left + SX(16) + badgeSize, r.top + SX(14) + badgeSize);
			COLORREF badgeBg = enabled ? RGB(0, 96, 210) : RGB(190, 199, 209);
			FillRoundRect(gPaint, badge, SX(6), badgeBg, badgeBg, 1);
			CString numText; numText.Format(_T("%d"), num);
			memDC.SelectObject(&m_fontNormal);
			memDC.SetTextColor(RGB(255, 255, 255));
			memDC.DrawText(numText, badge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			CString label; label.Format(_T("리더기%d - COM 포트"), num);
			memDC.SelectObject(&m_fontLabel);
			memDC.SetTextColor(enabled ? RGB(107, 114, 128) : RGB(156, 163, 175));
			memDC.TextOut(r.left + SX(64), r.top + SX(14), label);

			const int comboW = SX(178);
			const int btnH = SX(37);
			const int x0 = r.left + SX(64);
			const int yCombo = r.top + SX(34);
			const int toggleW = SX(52);
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
		tableOuter.OffsetRect(0, -SX(14));
		const COLORREF tableBorder = RGB(214, 223, 235);
		const COLORREF headerBg = RGB(250, 251, 253);
		const COLORREF headerLine = RGB(238, 241, 247);
		const COLORREF bodyLine = RGB(242, 245, 249);
		const COLORREF bodyBg = RGB(255, 255, 255);
		const COLORREF altRowBg = RGB(251, 252, 255);
		const COLORREF headerText = RGB(26, 32, 44);
		const COLORREF bodyText = RGB(92, 104, 122);
		const COLORREF emptyText = RGB(160, 168, 180);
		const COLORREF scrollTrack = RGB(243, 246, 250);
		const COLORREF scrollThumb = RGB(196, 205, 216);

		FillRoundRect(gPaint, tableOuter, SX(8), bodyBg, tableBorder, 0);

		const int* ratios = kIntegrityColumnRatios;
		const int colCount = kIntegrityColumnCount;
		const int headerH = SX(36);
		const int rowH = SX(38);
		const int textPad = SX(10);
		const int visibleRows = GetIntegrityVisibleRows();

		CRect headerRc = tableOuter;
		headerRc.DeflateRect(1, 1);
		headerRc.bottom = headerRc.top + headerH;
		FillTopRoundRect(gPaint, headerRc, SX(8), headerBg);
		memDC.FillSolidRect(headerRc.left + SX(10), headerRc.bottom - 1, headerRc.Width() - SX(20), 1, RGB(238, 241, 247));

		const int itemCount = (m_bUIReady && m_integrity_list.GetSafeHwnd()) ? m_integrity_list.GetItemCount() : 0;
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
		contentRc.DeflateRect(SX(6), SX(6));
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
		case IDC_SEARCH:           OnSearch();         return TRUE;
		}
	}

	return CDialog::OnCommand(wParam, lParam);
}

void CReaderSetupDlg::OnSelchangeComport1()
{
	CString value;
	m_comport1.GetWindowText(value);
	UpdateReaderEnableState(1);
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
				pWnd->Invalidate(FALSE);
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
	CDialog::OnTimer(nIDEvent);
}

BOOL CReaderSetupDlg::DestroyWindow()
{
	FinishLoadingOperation(FALSE);
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

void CReaderSetupDlg::OnSearch()
{
	StartLoadingOperation(IDC_SEARCH);
	// TODO: implement search logic
}
void CReaderSetupDlg::OnOK()
{
	CString port1, port2;
	m_comport1.GetWindowText(port1);
	m_comport2.GetWindowText(port2);
	AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, COMPORT1_FIELD, port1);
	AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, COMPORT2_FIELD, port2);
	// TODO: save toggle states (PORT_OPEN1/2, MULTIPAD1/2)
	CDialog::OnOK();
}

void CReaderSetupDlg::OnCancel()
{
	if (m_popover.GetSafeHwnd()) m_popover.Hide();
	if (HasChanges())
	{
		if (MessageBox(_T("변경된 내용이 있습니다.저장하지 않고 종료하시겠습니까?"), _T("확인"), MB_YESNO | MB_ICONQUESTION) != IDYES)
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
	if (m_togglePortOpen1.IsToggled() != m_snap.tglPortOpen1) return TRUE;
	if (m_togglePortOpen2.IsToggled() != m_snap.tglPortOpen2) return TRUE;
	if (m_toggleMultipad1.IsToggled()  != m_snap.tglMultipad1) return TRUE;
	if (m_toggleMultipad2.IsToggled()  != m_snap.tglMultipad2) return TRUE;
	return FALSE;
}
