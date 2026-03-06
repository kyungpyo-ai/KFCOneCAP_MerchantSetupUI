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
	{"체크결과"			, LVCFMT_CENTER	, 8 },
	{"모듈ID"			, LVCFMT_CENTER	, 13 },
	{"리더기식별번호"	, LVCFMT_CENTER	, 19 },
	{"POS식별번호"		, LVCFMT_CENTER	, 17 },
	{NULL			, NULL			, 0	}
} ;

static void FillRoundRect(CDC* pDC, const CRect& rc, int radius, COLORREF fill, COLORREF border, int borderW = 1)
{
	// 간단한 라운드 렌더링(GDI) - 그림자/알파 없이도 충분히 모던 느낌
	CRgn rgn;
	rgn.CreateRoundRectRgn(rc.left, rc.top, rc.right + 1, rc.bottom + 1, radius, radius);

	CBrush brFill(fill);
	pDC->FillRgn(&rgn, &brFill);

	CPen pen(borderW, PS_SOLID, border);
	CPen* oldPen = pDC->SelectObject(&pen);
	pDC->FrameRgn(&rgn, &CBrush(border), borderW, borderW);
	pDC->SelectObject(oldPen);
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

	// Normal
	lf.lfHeight = -SX(13);
	lf.lfWeight = FW_NORMAL;
	m_fontNormal.CreateFontIndirect(&lf);

	// Small
	lf.lfHeight = -SX(11);
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

	const int margin = SX(18);
	inner = CRect(rc.left + margin, rc.top + margin, rc.right - margin, rc.bottom - margin);

	// 상단 타이틀 블럭(리더기 설정 + 서브 + 라인)
	const int titleBlock = SX(92);        // 스샷 느낌으로 약간 여유
	const int sectionTitleH = SX(28);

	int y = inner.top + titleBlock;

	// 섹션1 제목 위치(리더기 설정)
	sec1TitlePt = CPoint(inner.left + SX(24), y);
	y += sectionTitleH + SX(10);

	// 카드(리더기1/2)
	const int cardH = SX(88);
	const int cardGap = SX(14);

	card1 = CRect(inner.left, y, inner.right, y + cardH);
	y = card1.bottom + cardGap;

	card2 = CRect(inner.left, y, inner.right, y + cardH);
	y = card2.bottom + SX(22);

	// 섹션2 제목 위치(리더기 정보)
	sec2TitlePt = CPoint(inner.left + SX(24), y);
	y += sectionTitleH + SX(10);

	// 조회 박스
	const int queryH = SX(62);
	queryBox = CRect(inner.left, y, inner.right, y + queryH);
	y = queryBox.bottom + SX(12);

	// 리스트
	const int bottomBtnH = SX(42);
	const int bottomGap = SX(16);
	const int bottomArea = bottomBtnH + bottomGap + SX(8);

	int listH = inner.bottom - y - bottomArea;
	if (listH < SX(180)) listH = SX(180);
	listRc = CRect(inner.left, y, inner.right, y + listH);

	// 하단 버튼 (가운데 정렬)
	int bw = SX(108), bh = bottomBtnH, bgap = SX(14);
	int totalW = bw * 2 + bgap;
	int bx = inner.left + (inner.Width() - totalW) / 2;
	int by = inner.bottom - bh;

	okRc = CRect(bx, by, bx + bw, by + bh);
	cancelRc = CRect(bx + bw + bgap, by, bx + bw + bgap + bw, by + bh);

	// (infoTitleArea는 필요 시 확장용)
	infoTitleArea = CRect(sec2TitlePt.x, sec2TitlePt.y, inner.right, sec2TitlePt.y + sectionTitleH);
}

void CReaderSetupDlg::ApplyEnableStateToButtons(int readerIndex, BOOL bEnable)
{
	if (readerIndex == 1)
	{
		m_reader_init1.EnableWindow(bEnable);
		m_status_check1.EnableWindow(bEnable);
		m_keydown1.EnableWindow(bEnable);
		m_integrity_check1.EnableWindow(bEnable);
		m_bReader1Enabled = bEnable;
	}
	else
	{
		m_reader_init2.EnableWindow(bEnable);
		m_status_check2.EnableWindow(bEnable);
		m_keydown2.EnableWindow(bEnable);
		m_integrity_check2.EnableWindow(bEnable);
		m_bReader2Enabled = bEnable;
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

	const int padX = SX(24);
	const int badge = SX(54);
	const int gap = SX(10);
	const int rowH = SX(34);

	int comboW = SX(250);
	int btnW = SX(84);
	int btnH = SX(32);

	// 카드 내부 기준 좌측 시작점
	auto placeReaderRow = [&](const CRect & card, CComboBox & cb,
		CButton & b1, CButton & b2, CButton & b3, CButton & b4)
	{
		int x = card.left + padX;
		int y = card.top + (card.Height() - rowH) / 2;

		// 카드 폭이 좁으면 콤보 폭 자동 축소(밀림 방지)
		int availW = card.Width() - padX * 2 - badge - gap;
		int needW = comboW + gap + (btnW * 4) + (gap * 3);
		int comboW2 = comboW;
		if (needW > availW)
		{
			// 버튼 4개 영역을 먼저 확보하고 남는 폭을 콤보로
			int btnArea = (btnW * 4) + (gap * 3);
			comboW2 = max(SX(170), availW - gap - btnArea);
		}

		cb.SetWindowPos(NULL, x + badge + gap, y, comboW2, rowH, SWP_NOZORDER | SWP_NOACTIVATE);

		int bx = x + badge + gap + comboW2 + gap;
		b1.SetWindowPos(NULL, bx + (btnW + gap) * 0, y, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
		b2.SetWindowPos(NULL, bx + (btnW + gap) * 1, y, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
		b3.SetWindowPos(NULL, bx + (btnW + gap) * 2, y, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
		b4.SetWindowPos(NULL, bx + (btnW + gap) * 3, y, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
	};

	placeReaderRow(card1, m_comport1, m_reader_init1, m_status_check1, m_keydown1, m_integrity_check1);
	placeReaderRow(card2, m_comport2, m_reader_init2, m_status_check2, m_keydown2, m_integrity_check2);

	// 조회 영역
	int qx = queryBox.left + SX(24);
	int qy = queryBox.top + (queryBox.Height() - rowH) / 2;
	int labelW = SX(88);
	int qComboW = SX(190);

	m_search_date.SetWindowPos(NULL, qx + labelW, qy, qComboW, rowH, SWP_NOZORDER | SWP_NOACTIVATE);

	if (CWnd * pSearch = GetDlgItem(IDC_SEARCH))
		pSearch->SetWindowPos(NULL, qx + labelW + qComboW + SX(14), queryBox.top + (queryBox.Height() - SX(34)) / 2,
			SX(84), SX(34), SWP_NOZORDER | SWP_NOACTIVATE);

	// 리스트
	m_integrity_list.SetWindowPos(NULL, listRc.left, listRc.top, listRc.Width(), listRc.Height(),
		SWP_NOZORDER | SWP_NOACTIVATE);

	// 하단 버튼
	if (CWnd * pOK = GetDlgItem(IDOK))
		pOK->SetWindowPos(NULL, okRc.left, okRc.top, okRc.Width(), okRc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);

	if (CWnd * pCancel = GetDlgItem(IDCANCEL))
		pCancel->SetWindowPos(NULL, cancelRc.left, cancelRc.top, cancelRc.Width(), cancelRc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

/////////////////////////////////////////////////////////////////////////////
// CReaderSetupDlg dialog


CReaderSetupDlg::CReaderSetupDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CReaderSetupDlg::IDD, pParent)
{
	m_bUIReady = FALSE;
	m_bFitDone = FALSE;
	m_dpi = 96;
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

	ON_CBN_SELCHANGE(IDC_COMPORT1, OnCbnSelchangeComport1)
	ON_CBN_SELCHANGE(IDC_COMPORT2, OnCbnSelchangeComport2)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
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

	CString com_port1;
	CString com_port2;


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

	ListView_SetExtendedListViewStyle(m_integrity_list.GetSafeHwnd(), LVS_EX_GRIDLINES);
	for(i = 0 ; col_info[i].column_name != NULL ; ++i)
		m_integrity_list.InsertColumn(i, col_info[i].column_name, col_info[i].format, col_info[i].width * 8);



	// ... (기존 포트 목록/레지스트리/초기 Enable 로직 그대로 유지)

	// DPI/폰트/레이아웃/구 static 숨김
	HDC hdc = ::GetDC(m_hWnd);
	m_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
	::ReleaseDC(m_hWnd, hdc);

	EnsureFonts();
	HideLegacyStatics();
	m_bUIReady = TRUE;
	ModifyStyle(0, WS_CLIPCHILDREN);

	FitWindowToLayout();
	LayoutControls();

	// 초기 상태 반영(콤보는 항상 활성, 버튼은 미사용이면 비활성)
	UpdateReaderEnableState(1);
	UpdateReaderEnableState(2);

	// 리스트: 기존 GRIDLINES는 좀 올드해 보이니 제거(원하면 유지 가능)
	// ListView_SetExtendedListViewStyle(m_integrity_list.GetSafeHwnd(), LVS_EX_FULLROWSELECT);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

CSize CReaderSetupDlg::CalcMinClientSize() const
{
	// LayoutControls에서 쓰는 수치와 맞춰야 함
	const int margin = SX(18);
	const int titleBlk = SX(86);
	const int secTitleH = SX(34);

	const int cardPadX = SX(24);
	const int cardH = SX(82);
	const int cardGap = SX(12);

	const int badge = SX(54);
	const int rowH = SX(34);

	const int comboW = SX(250);
	const int btnW = SX(84);
	const int btnH = SX(32);
	const int gap = SX(10);

	const int secGapAfterCards = SX(18) + SX(22);

	const int queryBoxH = SX(56);
	const int queryGap = SX(14);

	const int listMinH = SX(220);   // 표는 최소 이 정도는 보여주자(원하면 조정)

	const int bottomBtnH = SX(40);
	const int bottomGap = SX(16);
	const int bottomArea = bottomBtnH + bottomGap + SX(10);

	// ---- 가로 최소: 카드 내부에 한 줄로 들어가게 ----
	int innerW_need = 0;
	{
		int needRowW = badge + gap + comboW + gap + (btnW * 4) + (gap * 3);
		// 카드 안쪽 패딩 고려
		innerW_need = (cardPadX * 2) + needRowW;
	}

	// 바깥 margin 고려 (클라이언트 전체 폭)
	int clientW = (margin * 2) + innerW_need;

	// ---- 세로 최소 ----
	int clientH = 0;
	clientH += margin;                  // top
	clientH += SX(18) + titleBlk;        // 타이틀 영역(여유 포함)
	clientH += secTitleH + SX(10);       // 섹션 타이틀 + 간격

	clientH += cardH;                   // 카드1
	clientH += cardGap;
	clientH += cardH;                   // 카드2

	clientH += secGapAfterCards;        // 섹션 간격

	clientH += queryBoxH;               // 조회 박스
	clientH += queryGap;

	clientH += listMinH;                // 리스트 최소 높이

	clientH += bottomArea;              // 하단 버튼 영역
	clientH += margin;                  // bottom

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

void CReaderSetupDlg::OnCbnSelchangeComport1()
{
	UpdateReaderEnableState(1);
}

void CReaderSetupDlg::OnCbnSelchangeComport2()
{
	UpdateReaderEnableState(2);
}
void CReaderSetupDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (!m_bUIReady)
		return;

	if (GetSafeHwnd())
		LayoutControls();
}
BOOL CReaderSetupDlg::OnEraseBkgnd(CDC* pDC)
{
	return TRUE; // 전체를 OnPaint에서 칠함
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

	// 메인 카드
	const int margin = SX(18);
	CRect mainCard(rc.left + margin, rc.top + margin, rc.right - margin, rc.bottom - margin);
	CRect shadow = mainCard; shadow.OffsetRect(SX(2), SX(3));
	FillRoundRect(&memDC, shadow, SX(18), RGB(230, 235, 240), RGB(230, 235, 240), 1);
	FillRoundRect(&memDC, mainCard, SX(18), RGB(255, 255, 255), RGB(220, 226, 232), 1);

	// 상단 타이틀
	memDC.SetBkMode(TRANSPARENT);
	memDC.SelectObject(&m_fontTitle);
	memDC.SetTextColor(RGB(0, 90, 156));
	memDC.TextOut(mainCard.left + SX(24), mainCard.top + SX(18), _T("리더기 설정"));

	memDC.SelectObject(&m_fontSub);
	memDC.SetTextColor(RGB(120, 130, 140));
	memDC.TextOut(mainCard.left + SX(24), mainCard.top + SX(44), _T("리더기 연결 및 제어를 관리합니다"));
	memDC.FillSolidRect(mainCard.left + SX(24), mainCard.top + SX(72), mainCard.Width() - SX(48), 1, RGB(235, 240, 245));

	// 동일 Rect 기반으로 섹션/카드 영역 얻기
	CRect inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc;
	CPoint sec1Pt, sec2Pt;
	CalcLayoutRects(inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc, sec1Pt, sec2Pt);

	// 섹션 제목 2개(원하는대로 보여주기)
	memDC.SelectObject(&m_fontNormal);
	memDC.SetTextColor(RGB(45, 55, 72));
	memDC.TextOut(sec1Pt.x, sec1Pt.y, _T("리더기 설정"));
	memDC.TextOut(sec2Pt.x, sec2Pt.y, _T("리더기 정보"));

	// 카드 2개
	auto drawCard = [&](const CRect & r, BOOL enabled, int num)
	{
		COLORREF bg = enabled ? RGB(240, 248, 255) : RGB(245, 246, 248);
		COLORREF brd = enabled ? RGB(0, 118, 190) : RGB(220, 226, 232);
		int bw = enabled ? 2 : 1;
		FillRoundRect(&memDC, r, SX(14), bg, brd, bw);

		// 배지(1/2)
		CRect badge(r.left + SX(14), r.top + (r.Height() - SX(46)) / 2,
			r.left + SX(14) + SX(46), r.top + (r.Height() - SX(46)) / 2 + SX(46));

		FillRoundRect(&memDC, badge, SX(12),
			enabled ? RGB(0, 90, 156) : RGB(160, 168, 176),
			enabled ? RGB(0, 90, 156) : RGB(160, 168, 176), 1);

		memDC.SelectObject(&m_fontNormal);
		memDC.SetTextColor(RGB(255, 255, 255));
		CString s; s.Format(_T("%d"), num);
		memDC.DrawText(s, badge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	};

	drawCard(card1, m_bReader1Enabled, 1);
	drawCard(card2, m_bReader2Enabled, 2);

	// 조회 박스(연한 카드)
	FillRoundRect(&memDC, queryBox, SX(14), RGB(248, 249, 251), RGB(230, 235, 240), 1);

	dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);
	memDC.SelectObject(oldBmp);
}


void CReaderSetupDlg::OnSelchangeComport1() 
{
	// TODO: Add your control notification handler code here
	CString com_port1;
	m_comport1.GetWindowText(com_port1);

	if (com_port1.Compare("미사용") == 0) {
		m_reader_init1.EnableWindow(FALSE);
		m_status_check1.EnableWindow(FALSE);
		m_keydown1.EnableWindow(FALSE);
		m_integrity_check1.EnableWindow(FALSE);
	} else {
		m_reader_init1.EnableWindow(TRUE);
		m_status_check1.EnableWindow(TRUE);
		m_keydown1.EnableWindow(TRUE);
		m_integrity_check1.EnableWindow(TRUE);
	}
}

void CReaderSetupDlg::OnSelchangeComport2() 
{
	// TODO: Add your control notification handler code here
	CString com_port2;
	m_comport2.GetWindowText(com_port2);

	if (com_port2.Compare("미사용") == 0) {
		m_reader_init2.EnableWindow(FALSE);
		m_status_check2.EnableWindow(FALSE);
		m_keydown2.EnableWindow(FALSE);
		m_integrity_check2.EnableWindow(FALSE);
	} else {
		m_reader_init2.EnableWindow(TRUE);
		m_status_check2.EnableWindow(TRUE);
		m_keydown2.EnableWindow(TRUE);
		m_integrity_check2.EnableWindow(TRUE);
	}
}



BOOL CReaderSetupDlg::DestroyWindow() 
{

	return CDialog::DestroyWindow();
}

