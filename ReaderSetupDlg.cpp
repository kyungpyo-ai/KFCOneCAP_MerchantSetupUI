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

static HISTORY_COL_INFO col_info[] = 
{
	{"체크일시"			, LVCFMT_CENTER	, 14 },
	{"COM 포트"			, LVCFMT_CENTER	, 11 },
	{"결과"			,	 LVCFMT_CENTER	, 8 },
	{"모듈ID"			, LVCFMT_CENTER	, 13 },
	{"리더기식별번호"	, LVCFMT_CENTER	, 19 },
	{"POS식별번호"		, LVCFMT_CENTER	, 17 },
	{NULL			, NULL			, 0	}
} ;

static void GdipAddRoundRect(Gdiplus::GraphicsPath& p, float x, float y, float w, float h, float r)
{
	float d = r * 2.f;
	Gdiplus::RectF a(x, y, d, d);
	p.AddArc(a, 180.f, 90.f); a.X = x + w - d;
	p.AddArc(a, 270.f, 90.f); a.Y = y + h - d;
	p.AddArc(a,   0.f, 90.f); a.X = x;
	p.AddArc(a,  90.f, 90.f); p.CloseFigure();
}

static void FillRoundRect(CDC* pDC, const CRect& rc, int radius, COLORREF fill, COLORREF border, int borderW = 1)
{
	Gdiplus::Graphics g(pDC->GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

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

static void FillTopRoundRect(CDC* pDC, const CRect& rc, int radius, COLORREF fill)
{
	Gdiplus::Graphics g(pDC->GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	Gdiplus::GraphicsPath path;
	GdipAddTopRoundRect(path, (float)rc.left, (float)rc.top, (float)rc.Width(), (float)rc.Height(), (float)radius);
	Gdiplus::SolidBrush br(Gdiplus::Color(255, GetRValue(fill), GetGValue(fill), GetBValue(fill)));
	g.FillPath(&br, &path);
}

static void DrawRoundRectBorder(CDC* pDC, const CRect& rc, int radius, COLORREF border, int borderW = 1)
{
	if (borderW <= 0)
		return;

	Gdiplus::Graphics g(pDC->GetSafeHdc());
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	Gdiplus::GraphicsPath path;
	float bw = (float)borderW;
	GdipAddRoundRect(path, (float)rc.left + bw * 0.5f, (float)rc.top + bw * 0.5f,
		(float)rc.Width() - bw, (float)rc.Height() - bw, (float)radius);
	Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(border), GetGValue(border), GetBValue(border)), bw);
	g.DrawPath(&pen, &path);
}
int CReaderSetupDlg::SX(int v) const
{
	// DPI 스케일 (96 기준)
	return (v * m_dpi) / 96;
}

void CReaderSetupDlg::EnsureFonts()
{
	if (m_fontTitle.GetSafeHandle())
		return;

	LOGFONT lf = { 0 };
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);

	// Title
	lf.lfHeight = -SX(20);
	lf.lfWeight = FW_BOLD;
	lstrcpy(lf.lfFaceName, _T("맑은 고딕"));
	m_fontTitle.CreateFontIndirect(&lf);

	// Sub
	lf.lfHeight = -SX(12);
	lf.lfWeight = FW_NORMAL;
	m_fontSub.CreateFontIndirect(&lf);

	// Section title
	lf.lfHeight = -SX(14);
	lf.lfWeight = FW_BOLD;
	m_fontSection.CreateFontIndirect(&lf);

	// Normal
	lf.lfHeight = -SX(13);
	lf.lfWeight = FW_NORMAL;
	m_fontNormal.CreateFontIndirect(&lf);

	// Label
	lf.lfHeight = -SX(13);
	lf.lfWeight = FW_NORMAL;
	m_fontLabel.CreateFontIndirect(&lf);

	// Small
	lf.lfHeight = -SX(12);
	lf.lfWeight = FW_NORMAL;
	m_fontSmall.CreateFontIndirect(&lf);
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

	const int titleBlock = SX(92);
	const int sectionTitleH = SX(28);
	const int sectionTitleTop = SX(10);
	const int sectionTitleGap = SX(12);
	const int infoTitleGap = SX(4);
	const int sectionBoxPad = SX(24);
	const int cardH = SX(126);
	const int cardGap = SX(14);
	const int queryH = SX(56);
	const int queryGap = SX(18);
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
	y = card2.bottom + sectionBoxPad + SX(10);

	sec2TitlePt = CPoint(sectionLeft + SX(24), y + sectionTitleTop);
	int sec2ContentTop = y + sectionTitleTop + sectionTitleH + infoTitleGap;

	queryBox = CRect(sectionLeft + sectionBoxPad, sec2ContentTop,
		sectionRight - sectionBoxPad, sec2ContentTop + queryH);
	y = queryBox.bottom + queryGap;

	int listTop = y;
	int listBottomLimit = inner.bottom - bottomArea - infoBottomPad - sectionBoxPad;
	int listH = listBottomLimit - listTop;
	if (listH < SX(208)) listH = SX(208);
	if (listH > SX(208)) listH = SX(208);
	listRc = CRect(sectionLeft + sectionBoxPad, listTop,
		sectionRight - sectionBoxPad, listTop + listH);

	// ShopSetupDlg 하단 버튼과 동일한 크기/간격/위치 규칙 적용
	const int buttonW = SX(110);
	const int buttonH = SX(36);
	const int buttonGap = SX(8);
	const int buttonBottom = SX(10);
	const int buttonTopGap = SX(8);
	int totalW = buttonW * 2 + buttonGap;
	int bx = (rc.Width() - totalW) / 2;
	int by = listRc.bottom + buttonTopGap;
	int maxBy = rc.bottom - (SX(12) + buttonBottom + buttonH);
	if (by > maxBy)
		by = maxBy;

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
	return CRect(queryBox.left - SX(24), queryBox.top - SX(56), queryBox.right + SX(24), listRc.bottom + SX(4));
}

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
	const int ratios[] = { 20, 13, 7, 14, 23, 23 };
	const int colCount = sizeof(ratios) / sizeof(ratios[0]);
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

int CReaderSetupDlg::GetIntegrityVisibleRows() const
{
	return 4;
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
	const COLORREF cardBgEnabled  = RGB(255, 255, 255);
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
	const int btnW = SX(78);
	const int btnH = SX(36);
	const int gap = SX(8);
	const int toggleW = SX(52);
	const int toggleH = SX(28);
	const int rowComboY = SX(34);
	const int rowBtnY = SX(76);
	const int openLabelW = SX(56);
	const int multiLabelW = SX(74);
	const int textGap = SX(8);
	const int toggleBlockGap = SX(18);

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

		int xTogglePad = card.right - padR - toggleW;
		int xTogglePadLabel = xTogglePad - textGap - multiLabelW;
		int xToggleOpen = xTogglePadLabel - toggleBlockGap - toggleW;
		if (xToggleOpen < x0 + comboW + SX(70))
			xToggleOpen = x0 + comboW + SX(70);
		tgOpen.SetWindowPos(NULL, xToggleOpen, yCombo + (btnH - toggleH) / 2,
			toggleW, toggleH, SWP_NOZORDER | SWP_NOACTIVATE);

		tgPad.SetWindowPos(NULL, xTogglePad, yCombo + (btnH - toggleH) / 2,
			toggleW, toggleH, SWP_NOZORDER | SWP_NOACTIVATE);

		int yBtn = card.top + rowBtnY;
		int bx = x0;
		bInit.SetWindowPos(NULL, bx, yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
		bStatus.SetWindowPos(NULL, bx + (btnW + gap), yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
		bKey.SetWindowPos(NULL, bx + (btnW + gap) * 2, yBtn, btnW + SX(6), btnH, SWP_NOZORDER | SWP_NOACTIVATE);
		bInteg.SetWindowPos(NULL, bx + (btnW + gap) * 3 + SX(6), yBtn, btnW + SX(10), btnH, SWP_NOZORDER | SWP_NOACTIVATE);
		bUpdate.SetWindowPos(NULL, bx + (btnW + gap) * 4 + SX(16), yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
	};

	placeReaderCard(card1, m_comport1,
		m_reader_init1, m_status_check1, m_keydown1, m_integrity_check1, m_update1,
		m_togglePortOpen1, m_toggleMultipad1);
	placeReaderCard(card2, m_comport2,
		m_reader_init2, m_status_check2, m_keydown2, m_integrity_check2, m_update2,
		m_togglePortOpen2, m_toggleMultipad2);

	int qx = queryBox.left;
	int qComboW = SX(120);
	CRect rcSearchCombo;
	m_search_date.GetWindowRect(&rcSearchCombo);
	int qVisibleH = rcSearchCombo.Height();
	if (qVisibleH <= 0)
		qVisibleH = SX(28);
	int qy = queryBox.top + (queryBox.Height() - qVisibleH) / 2 + SX(2);
	m_search_date.SetWindowPos(NULL, qx, qy, qComboW, SX(220), SWP_NOZORDER | SWP_NOACTIVATE);
	m_btnSearch.SetWindowPos(NULL, qx + qComboW + SX(12), qy, SX(62), qVisibleH, SWP_NOZORDER | SWP_NOACTIVATE);

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
	m_dpi = 96;
	m_bReader1Enabled = FALSE;
	m_bReader2Enabled = FALSE;
	m_nIntegrityScrollPos = 0;
	m_rcIntegrityScrollBar.SetRectEmpty();
	m_rcIntegrityScrollThumb.SetRectEmpty();
	//{{AFX_DATA_INIT(CReaderSetupDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CReaderSetupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReaderSetupDlg)
	DDX_Control(pDX, IDC_INTEGRITY_LIST,    m_integrity_list);
	DDX_Control(pDX, IDC_SEARCH_DATE,       m_search_date);
	DDX_Control(pDX, IDC_READER_INIT2,      m_reader_init2);
	DDX_Control(pDX, IDC_READER_INIT1,      m_reader_init1);
	DDX_Control(pDX, IDC_STATUS_CHECK2,     m_status_check2);
	DDX_Control(pDX, IDC_STATUS_CHECK1,     m_status_check1);
	DDX_Control(pDX, IDC_KEYDOWN2,          m_keydown2);
	DDX_Control(pDX, IDC_KEYDOWN1,          m_keydown1);
	DDX_Control(pDX, IDC_INTEGRITY_CHECK2,  m_integrity_check2);
	DDX_Control(pDX, IDC_INTEGRITY_CHECK1,  m_integrity_check1);
	DDX_Control(pDX, IDC_COMPORT2,          m_comport2);
	DDX_Control(pDX, IDC_COMPORT1,          m_comport1);
	DDX_Control(pDX, IDC_UPDATE1,           m_update1);
	DDX_Control(pDX, IDC_UPDATE2,           m_update2);
	DDX_Control(pDX, IDC_PORT_OPEN1,        m_togglePortOpen1);
	DDX_Control(pDX, IDC_PORT_OPEN2,        m_togglePortOpen2);
	DDX_Control(pDX, IDC_MULTIPAD1,         m_toggleMultipad1);
	DDX_Control(pDX, IDC_MULTIPAD2,         m_toggleMultipad2);
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
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReaderSetupDlg message handlers/////////////////////////////////////////////////////////////////////////////
// CReaderSetupDlg message handlers

int CReaderSetupDlg::GetWindowsVersion()
{
	OSVERSIONINFO osv;
	osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	CString WindowsPlatform;
	if(GetVersionEx(&osv))
	{
		// note: szCSDVersion =  service pack  release
		CString ServiceRelease = osv.szCSDVersion;
		switch(osv.dwPlatformId)
		{
		case VER_PLATFORM_WIN32s:				//Win32s on Windows 3.1.
			return WINDOWS_VERSION_3;

		case VER_PLATFORM_WIN32_WINDOWS:		//WIN32 on 95 or 98
			if(osv.dwMinorVersion == 0)
				return WINDOWS_VERSION_95;
			else
				return WINDOWS_VERSION_98;

		case VER_PLATFORM_WIN32_NT:				//Win32 on Windows NT.
			return WINDOWS_VERSION_NT;

		default:
			AfxMessageBox("Failed to get correct Operating System.", MB_OK);
			return 0;
		}
	}

	return 0;
}

void CReaderSetupDlg::GetNTComPort(vector<int>& ports)
{
	LONG result;
	HKEY hKey;
	
	result = ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T( "Hardware\\DeviceMap\\SerialComm" ), 0, KEY_READ, &hKey ) ;
	int m_nPortNo = 0 ;
	TCHAR name[255] ;
	DWORD nameLen ;
	BYTE data[255] ;
	DWORD dataLen ;
	for(;;)
	{
		nameLen = sizeof( name ) ;
		dataLen = sizeof( data ) ;
		int err = ::RegEnumValue( hKey, ( DWORD )m_nPortNo, name, &nameLen, NULL, NULL, data, &dataLen ) ;
		if( err != ERROR_SUCCESS )
			break ;
		char *ptr = ( char* )data ;
		while( ( *ptr < '0' ) || ( *ptr > '9' ) )
		{
			if( *ptr == 0x00 )
				break ;
			ptr++ ;
		}
		ports.push_back(atoi(ptr));
		m_nPortNo++ ;
	}
	::RegCloseKey( hKey ) ;
}

void CReaderSetupDlg::GetWidowsComPort(vector<int>& ports)
{
	LONG result;
	HKEY hKey;
	TCHAR tmpdata[10] ;
	unsigned long tmpsize ;
	char strCOM[10] ;

	result = ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Hardware\\DeviceMap\\SerialComm", 0, KEY_QUERY_VALUE, &hKey ) ;
	for( int i = 0 ; i < 99 ; i++ )
	{
		tmpsize = sizeof( tmpdata ) ;
		sprintf( strCOM, "COM%d", i+1 ) ;
		result = ::RegQueryValueEx( hKey, strCOM, NULL, NULL, ( BYTE* )tmpdata, &tmpsize ) ;
		if( result == ERROR_SUCCESS )	// cannot read com info
		{
			ports.push_back(i + 1);
		}
	}
	::RegCloseKey( hKey ) ;
}

BOOL CReaderSetupDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	ModernUIGfx::EnsureGdiplusStartup();

	ModifyStyle(0, WS_CLIPCHILDREN);
	


	vector<int> ports;
	int windows_platform = GetWindowsVersion();
	if (windows_platform == WINDOWS_VERSION_95 || windows_platform == WINDOWS_VERSION_98)
		GetWidowsComPort(ports);
	else
		GetNTComPort(ports);

	m_comport1.AddString("미사용");
	m_comport2.AddString("미사용");

	int i;
	char buff[64];
	for(i = 0 ; i < ports.size(); i++) {
		sprintf(buff, "COM %02d", ports[i]);
		m_comport1.AddString(buff);
		m_comport2.AddString(buff);
	}

	CString com_port1 = AfxGetApp()->GetProfileString(SERIAL_PORT_SECTION, COMPORT1_FIELD, _T(""));
	CString com_port2 = AfxGetApp()->GetProfileString(SERIAL_PORT_SECTION, COMPORT2_FIELD, _T(""));

	if (com_port1.GetLength() == 0) {
		com_port1 = "미사용";
		AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, COMPORT1_FIELD, com_port1);
	}
	if (com_port2.GetLength() == 0) {
		com_port2 = "미사용";
		AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, COMPORT2_FIELD, com_port2);
	}

	m_comport1.SetCurSel(0);
	m_comport2.SetCurSel(0);
	for (i = 0; i < ports.size(); ++i) {
		sprintf(buff, "COM %02d", ports[i]);

		if (com_port1.Compare(buff) == 0)
			m_comport1.SetCurSel(i + 1);
		if (com_port2.Compare(buff) == 0)
			m_comport2.SetCurSel(i + 1);
	}

	if (m_comport1.GetCurSel() == 0 && com_port1.Compare("미사용") != 0) {
		com_port1 += "(사용불가)";
		m_comport1.AddString(com_port1);
		m_comport1.SetCurSel(ports.size() + 1);
	}
	if (m_comport2.GetCurSel() == 0 && com_port2.Compare("미사용") != 0) {
		com_port2 += "(사용불가)";
		m_comport2.AddString(com_port2);
		m_comport2.SetCurSel(ports.size() + 1);
	}

	if (com_port1.Compare("미사용") == 0) {
		m_reader_init1.EnableWindow(FALSE);
		m_status_check1.EnableWindow(FALSE);
		m_keydown1.EnableWindow(FALSE);
		m_integrity_check1.EnableWindow(FALSE);
	}
	if (com_port2.Compare("미사용") == 0) {
		m_reader_init2.EnableWindow(FALSE);
		m_status_check2.EnableWindow(FALSE);
		m_keydown2.EnableWindow(FALSE);
		m_integrity_check2.EnableWindow(FALSE);
	}

	m_search_date.AddString("오늘");
	m_search_date.AddString("7일");
	m_search_date.AddString("30일");
	m_search_date.AddString("100일");

	m_search_date.SetCurSel(0);

	// 폰트와 텍스트 크기를 ShopSetupDlg 느낌에 가깝게 맞춘다.
	m_comport1.SetFont(&m_fontNormal);
	m_comport2.SetFont(&m_fontNormal);
	m_search_date.SetFont(&m_fontNormal);
	m_reader_init1.SetFont(&m_fontNormal);
	m_status_check1.SetFont(&m_fontNormal);
	m_keydown1.SetFont(&m_fontNormal);
	m_integrity_check1.SetFont(&m_fontNormal);
	m_update1.SetFont(&m_fontNormal);
	m_reader_init2.SetFont(&m_fontNormal);
	m_status_check2.SetFont(&m_fontNormal);
	m_keydown2.SetFont(&m_fontNormal);
	m_integrity_check2.SetFont(&m_fontNormal);
	m_update2.SetFont(&m_fontNormal);
	m_btnSearch.SetFont(&m_fontNormal);
	m_btnOk.SetFont(&m_fontNormal);
	m_btnCancel.SetFont(&m_fontNormal);
	m_integrity_list.SetFont(&m_fontNormal);

	DWORD exStyle = LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT;
#ifdef LVS_EX_DOUBLEBUFFER
	exStyle |= LVS_EX_DOUBLEBUFFER;
#endif
	ListView_SetExtendedListViewStyle(m_integrity_list.GetSafeHwnd(), exStyle);
	m_integrity_list.ModifyStyle(0, LVS_SHOWSELALWAYS);
	for(i = 0 ; col_info[i].column_name != NULL ; ++i)
		m_integrity_list.InsertColumn(i, col_info[i].column_name, col_info[i].format, col_info[i].width * 8);



	// ... (기존 포트 목록/레지스트리/초기 Enable 로직 그대로 유지)

	// DPI/폰트/레이아웃/구 static 숨김
	HDC hdc = ::GetDC(m_hWnd);
	m_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
	::ReleaseDC(m_hWnd, hdc);

	EnsureFonts();
	HideLegacyStatics();

	// 폰트와 텍스트 크기를 ShopSetupDlg 느낌에 가깝게 맞춘다.
	m_comport1.SetFont(&m_fontNormal);
	m_comport2.SetFont(&m_fontNormal);
	m_search_date.SetFont(&m_fontNormal);
	m_reader_init1.SetFont(&m_fontNormal);
	m_status_check1.SetFont(&m_fontNormal);
	m_keydown1.SetFont(&m_fontNormal);
	m_integrity_check1.SetFont(&m_fontNormal);
	m_update1.SetFont(&m_fontNormal);
	m_reader_init2.SetFont(&m_fontNormal);
	m_status_check2.SetFont(&m_fontNormal);
	m_keydown2.SetFont(&m_fontNormal);
	m_integrity_check2.SetFont(&m_fontNormal);
	m_update2.SetFont(&m_fontNormal);
	m_btnSearch.SetFont(&m_fontNormal);
	m_btnOk.SetFont(&m_fontNormal);
	m_btnCancel.SetFont(&m_fontNormal);
	m_integrity_list.SetFont(&m_fontNormal);

	// === ModernUI 스타일 적용 ===
	// 콤보박스 스킨

	// 액션 버튼 스타일 (Secondary: 회색 테두리)
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

	// 하단 확인/취소 버튼 스타일
	if (CWnd* pOK = GetDlgItem(IDOK))
		m_btnOk.SubclassDlgItem(IDOK, this);
	if (CWnd* pCancel = GetDlgItem(IDCANCEL))
		m_btnCancel.SubclassDlgItem(IDCANCEL, this);
	m_btnOk.SetButtonStyle(ButtonStyle::Primary);
	m_btnCancel.SetButtonStyle(ButtonStyle::Default);

	// 조회 버튼
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
	m_integrity_list.SetBkColor(RGB(255, 255, 255));
	m_integrity_list.SetTextBkColor(RGB(255, 255, 255));
	m_integrity_list.ShowWindow(SW_HIDE);

	// Custom table preview sample rows
	if (m_integrity_list.GetItemCount() == 0)
	{
		const TCHAR* sampleRows[][6] =
		{
			{ _T("20260308091234"), _T("COM 01"), _T("00"),   _T("RDR-1001"), _T("##SPAY-8800Q3001"), _T("KFTCONECAP3001") },
			{ _T("20260308091234"), _T("COM 01"), _T("04"),   _T("RDR-1001"), _T("##SPAY-8800Q3001"), _T("KFTCONECAP3001") },
			{ _T("20260308091234"), _T("COM 02"), _T("00"), _T("RDR-2003"), _T("DAULPAY633RDK201"), _T("KFTCONECAP3001") },
			{ _T("20260308091234"), _T("COM 03"), _T("00"),   _T("RDR-1010"), _T("DAULPAY633RDK201"), _T("KFTCONECAP3001") },
			{ _T("20260308091234"), _T("COM 01"), _T("00"),   _T("RDR-1001"), _T("##SPAY-8800Q3001"), _T("KFTCONECAP3001") },
			{ _T("20260308091234"), _T("COM 04"), _T("00"), _T("RDR-3011"), _T("##SPAY-8800Q3001"), _T("KFTCONECAP3001") },
		};

		for (int row = 0; row < 6; ++row)
		{
			int idx = m_integrity_list.InsertItem(row, sampleRows[row][0]);
			for (int col = 1; col < 6; ++col)
				m_integrity_list.SetItemText(idx, col, sampleRows[row][col]);
		}
	}
	NormalizeIntegrityScrollPos();

	// 토글 초기 상태 (포트 열기: 미사용이면 OFF)
	m_togglePortOpen1.SetCheck(BST_UNCHECKED);
	m_togglePortOpen2.SetCheck(BST_UNCHECKED);
	m_toggleMultipad1.SetCheck(BST_UNCHECKED);
	m_toggleMultipad2.SetCheck(BST_UNCHECKED);

	// Set underlay colors to match card backgrounds
	COLORREF cardBgEnabled  = RGB(255, 255, 255);
	COLORREF cardBgDisabled = RGB(251, 252, 253);
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
	m_search_date.SetUnderlayColor(RGB(248, 249, 251));
	m_btnSearch.SetUnderlayColor(RGB(248, 249, 251));
	m_btnOk.SetUnderlayColor(RGB(255, 255, 255));
	m_btnCancel.SetUnderlayColor(RGB(255, 255, 255));

	m_bUIReady = TRUE;
	ModifyStyle(0, WS_CLIPCHILDREN);

	FitWindowToLayout();
	LayoutControls();

	// 초기 상태 반영(콤보는 항상 활성, 버튼은 미사용이면 비활성)
	UpdateReaderEnableState(1);
	UpdateReaderEnableState(2);

	GetDlgItem(IDOK)->SetFocus();   // 확인 버튼으로 포커스 이동
	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

CSize CReaderSetupDlg::CalcMinClientSize() const
{
	const int margin = SX(20);
	const int innerW = SX(720);
	const int titleArea = SX(92);
	const int sectionPad = SX(24);
	const int sectionTitleTop = SX(10);
	const int sectionTitleH = SX(28);
	const int sectionTitleGap = SX(12);
	const int infoTitleGap = SX(4);
	const int cardH = SX(126);
	const int cardGap = SX(14);
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

	memDC.FillSolidRect(rc, RGB(245, 247, 250));
	memDC.SetBkMode(TRANSPARENT);

	const int margin = SX(20);
	CRect mainCard(rc.left + margin, rc.top + margin, rc.right - margin, rc.bottom - margin);
	CRect shadow = mainCard; shadow.OffsetRect(SX(2), SX(3));
	FillRoundRect(&memDC, shadow, SX(18), RGB(234, 238, 243), RGB(234, 238, 243), 1);
	FillRoundRect(&memDC, mainCard, SX(18), RGB(255, 255, 255), RGB(224, 229, 235), 1);

	const int iconSize = SX(38);
	const int iconX = mainCard.left + SX(20);
	const int iconY = mainCard.top + SX(18);
	CRect rcIcon(iconX, iconY, iconX + iconSize, iconY + iconSize);
	// GDI+ anti-aliased icon: card reader (IC card with chip + stripe)
	{
		HDC hIco = memDC.GetSafeHdc();
		Gdiplus::Graphics gIco(hIco);
		gIco.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		const float bx = (float)iconX, by = (float)iconY, bsz = (float)iconSize;
		const float cx = bx + bsz * 0.5f, cy = by + bsz * 0.5f;

		auto MRR = [](Gdiplus::GraphicsPath& p,
			float x, float y, float w, float h, float r) {
			float d = r * 2.f;
			Gdiplus::RectF a(x, y, d, d);
			p.AddArc(a, 180.f, 90.f); a.X = x + w - d;
			p.AddArc(a, 270.f, 90.f); a.Y = y + h - d;
			p.AddArc(a,   0.f, 90.f); a.X = x;
			p.AddArc(a,  90.f, 90.f); p.CloseFigure();
		};

		// gradient blue background
		{
			Gdiplus::GraphicsPath bp;
			MRR(bp, bx, by, bsz, bsz, 8.f);
			Gdiplus::LinearGradientBrush grad(
				Gdiplus::PointF(bx, by), Gdiplus::PointF(bx, by + bsz),
				Gdiplus::Color(255, 60, 130, 245),
				Gdiplus::Color(255, 28,  76, 210));
			gIco.FillPath(&grad, &bp);
		}

		// card body (white, landscape, rounded corners)
		float cW = bsz * 0.72f, cH = bsz * 0.50f;
		float cX = cx - cW * 0.5f, cY = cy - cH * 0.5f;
		{
			Gdiplus::GraphicsPath cp;
			MRR(cp, cX, cY, cW, cH, 2.5f);
			Gdiplus::SolidBrush wb(Gdiplus::Color(255, 255, 255, 255));
			gIco.FillPath(&wb, &cp);
		}

		// magnetic stripe (semi-transparent dark band across top of card)
		float sH = cH * 0.28f, sY = cY + cH * 0.22f;
		Gdiplus::SolidBrush sb(Gdiplus::Color(80, 20, 60, 150));
		gIco.FillRectangle(&sb, Gdiplus::RectF(cX, sY, cW, sH));

		// IC chip (gold, bottom-left area of card)
		float chW = cW * 0.22f, chH = cH * 0.36f;
		float chX = cX + cW * 0.13f, chY = sY + sH + cH * 0.07f;
		{
			Gdiplus::GraphicsPath chp;
			MRR(chp, chX, chY, chW, chH, 1.5f);
			Gdiplus::SolidBrush gb(Gdiplus::Color(200, 215, 175, 55));
			gIco.FillPath(&gb, &chp);
		}
	}

	const int titleTextX = rcIcon.right + SX(14);
	memDC.SelectObject(&m_fontTitle);
	memDC.SetTextColor(RGB(33, 37, 41));
	memDC.TextOut(titleTextX, mainCard.top + SX(18), _T("리더기 설정"));

	memDC.SelectObject(&m_fontSub);
	memDC.SetTextColor(RGB(106, 119, 133));
	memDC.TextOut(titleTextX, mainCard.top + SX(46), _T("리더기 연결 및 제어를 관리합니다"));
	memDC.FillSolidRect(mainCard.left + SX(22), mainCard.top + SX(72), mainCard.Width() - SX(44), 1, RGB(230, 234, 239));

	CRect inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc;
	CPoint sec1Pt, sec2Pt;
	CalcLayoutRects(inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc, sec1Pt, sec2Pt);
	CRect portSection = CalcPortSectionBox(card1, card2);
	CRect integritySection = CalcIntegritySectionBox(queryBox, listRc);

	auto drawSectionTitle = [&](const CPoint& pt, LPCTSTR text)
	{
		memDC.FillSolidRect(pt.x - SX(10), pt.y + SX(2), SX(3), SX(16), RGB(0, 102, 221));
		memDC.SelectObject(&m_fontSection);
		memDC.SetTextColor(RGB(37, 47, 63));
		memDC.TextOut(pt.x, pt.y, text);
	};

	FillRoundRect(&memDC, portSection, SX(10), RGB(248, 249, 251), RGB(233, 236, 240), 1);
	FillRoundRect(&memDC, integritySection, SX(10), RGB(248, 249, 251), RGB(233, 236, 240), 1);
	drawSectionTitle(sec1Pt, _T("포트 설정"));
	drawSectionTitle(sec2Pt, _T("무결성 체크 정보"));

	auto drawReaderCard = [&](const CRect& r, BOOL enabled, int num)
	{
		COLORREF bg = enabled ? RGB(255, 255, 255) : RGB(251, 252, 253);
		COLORREF br = enabled ? RGB(0, 102, 221) : RGB(220, 226, 232);
		FillRoundRect(&memDC, r, SX(8), bg, br, enabled ? 2 : 1);

		const int badgeSize = SX(34);
		CRect badge(r.left + SX(16), r.top + SX(14), r.left + SX(16) + badgeSize, r.top + SX(14) + badgeSize);
		COLORREF badgeBg = enabled ? RGB(0, 102, 221) : RGB(190, 199, 209);
		FillRoundRect(&memDC, badge, SX(6), badgeBg, badgeBg, 1);
		CString numText; numText.Format(_T("%d"), num);
		memDC.SelectObject(&m_fontNormal);
		memDC.SetTextColor(RGB(255, 255, 255));
		memDC.DrawText(numText, badge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		CString label; label.Format(_T("리더기%d - COM 포트"), num);
		memDC.SelectObject(&m_fontLabel);
		memDC.SetTextColor(enabled ? RGB(107, 114, 128) : RGB(156, 163, 175));
		memDC.TextOut(r.left + SX(64), r.top + SX(14), label);

		const int comboW = SX(178);
		const int btnH = SX(36);
		const int x0 = r.left + SX(64);
		const int yCombo = r.top + SX(34);
		const int toggleW = SX(52);
		const int padR = SX(22);
		const int textGap = SX(8);
		const int openLabelW = SX(56);
		const int multiLabelW = SX(74);
		const int togglePadX = r.right - padR - toggleW;
		const int togglePadLabelX = togglePadX - textGap - multiLabelW;
		int toggleOpenX = togglePadLabelX - SX(18) - toggleW;
		if (toggleOpenX < x0 + comboW + SX(70))
			toggleOpenX = x0 + comboW + SX(70);
		const int xToggleOpenLabel = toggleOpenX - textGap - openLabelW;
		memDC.TextOut(xToggleOpenLabel, yCombo + (btnH - SX(14)) / 2, _T("포트 열기"));
		memDC.TextOut(togglePadLabelX, yCombo + (btnH - SX(14)) / 2, _T("멀티패드 여부"));
	};

	drawReaderCard(card1, m_bReader1Enabled, 1);
	drawReaderCard(card2, m_bReader2Enabled, 2);

	// 기본 ListCtrl은 숨기고, 무결성 체크 표는 여기서 직접 그린다.
	{
		CRect tableOuter = listRc;
		tableOuter.DeflateRect(1, 1);
		tableOuter.OffsetRect(0, -SX(14));
		const COLORREF tableBorder = RGB(184, 196, 212);
		const COLORREF headerBg = RGB(244, 247, 250);
		const COLORREF headerLine = RGB(227, 233, 240);
		const COLORREF bodyLine = RGB(238, 242, 247);
		const COLORREF bodyBg = RGB(255, 255, 255);
		const COLORREF altRowBg = RGB(249, 251, 254);
		const COLORREF headerText = RGB(42, 58, 84);
		const COLORREF bodyText = RGB(86, 98, 115);
		const COLORREF emptyText = RGB(160, 168, 180);
		const COLORREF scrollTrack = RGB(243, 246, 250);
		const COLORREF scrollThumb = RGB(196, 205, 216);

		FillRoundRect(&memDC, tableOuter, SX(8), bodyBg, tableBorder, 0);

		const int ratios[] = { 20, 13, 7, 14, 23, 23 };
		const int colCount = sizeof(ratios) / sizeof(ratios[0]);
		const int headerH = SX(36);
		const int rowH = SX(38);
		const int textPad = SX(10);
		const int visibleRows = GetIntegrityVisibleRows();

		CRect headerRc = tableOuter;
		headerRc.DeflateRect(1, 1);
		headerRc.bottom = headerRc.top + headerH;
		FillTopRoundRect(&memDC, headerRc, SX(8), headerBg);
		memDC.FillSolidRect(headerRc.left + SX(10), headerRc.bottom - 1, headerRc.Width() - SX(20), 1, RGB(220, 227, 236));

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

		if (itemCount <= 0)
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
					memDC.SelectObject(&m_fontSmall);
					memDC.SetTextColor(bodyText);
					memDC.DrawText(cellText, drawRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
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
				FillRoundRect(&memDC, m_rcIntegrityScrollBar, SX(4), scrollTrack, scrollTrack, 1);
				m_rcIntegrityScrollThumb = CRect(m_rcIntegrityScrollBar.left, thumbTop, m_rcIntegrityScrollBar.right, thumbTop + thumbH);
				FillRoundRect(&memDC, m_rcIntegrityScrollThumb, SX(4), scrollThumb, scrollThumb, 1);
			}
		}

		// 마지막에 라운드 외곽선을 다시 그려 모서리/상단 프레임이 배경에 묻히지 않도록 한다.
		DrawRoundRectBorder(&memDC, tableOuter, SX(8), tableBorder, 1);
		DrawRoundRectBorder(&memDC, CRect(tableOuter.left + 1, tableOuter.top + 1, tableOuter.right - 1, tableOuter.bottom - 1), SX(7), RGB(232, 237, 244), 1);
	}	
	dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);
	memDC.SelectObject(oldBmp);
}



BOOL CReaderSetupDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT nID  = LOWORD(wParam);
	int  nCode = (int)(short)HIWORD(wParam);

	// CSkinnedComboBox ON_CONTROL_REFLECT(CBN_SELCHANGE) causes MFC to eat
	// the notification before the parent message map runs.
	// Intercept here before default reflection so our handlers fire.
	if (nCode == CBN_SELCHANGE)
	{
		if (nID == IDC_COMPORT1) { OnSelchangeComport1(); return TRUE; }
		if (nID == IDC_COMPORT2) { OnSelchangeComport2(); return TRUE; }
	}

	return CDialog::OnCommand(wParam, lParam);
}

void CReaderSetupDlg::OnSelchangeComport1() 
{
	CString value;
	m_comport1.GetWindowText(value);
	AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, COMPORT1_FIELD, value);
	UpdateReaderEnableState(1);
}

void CReaderSetupDlg::OnSelchangeComport2() 
{
	CString value;
	m_comport2.GetWindowText(value);
	AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, COMPORT2_FIELD, value);
	UpdateReaderEnableState(2);
}



BOOL CReaderSetupDlg::DestroyWindow() 
{

	return CDialog::DestroyWindow();
}

