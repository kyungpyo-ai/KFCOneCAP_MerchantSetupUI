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
	{"Ã¼Å©ÀÏ½Ã"			, LVCFMT_CENTER	, 14 },
	{"COM Æ÷Æ®"			, LVCFMT_CENTER	, 11 },
	{"Ã¼Å©°á°ú"			, LVCFMT_CENTER	, 8 },
	{"¸ðµâID"			, LVCFMT_CENTER	, 13 },
	{"¸®´õ±â½Äº°¹øÈ£"	, LVCFMT_CENTER	, 19 },
	{"POS½Äº°¹øÈ£"		, LVCFMT_CENTER	, 17 },
	{NULL			, NULL			, 0	}
} ;

static void FillRoundRect(CDC* pDC, const CRect& rc, int radius, COLORREF fill, COLORREF border, int borderW = 1)
{
	// °£´ÜÇÑ ¶ó¿îµå ·»´õ¸µ(GDI) - ±×¸²ÀÚ/¾ËÆÄ ¾øÀÌµµ ÃæºÐÈ÷ ¸ð´ø ´À³¦
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
	// DPI ½ºÄÉÀÏ (96 ±âÁØ)
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
	lstrcpy(lf.lfFaceName, _T("¸¼Àº °íµñ"));
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

			// ÀÌ°Í¸¸ ¼û±è (Á¦¸ñÀº ¼û±âÁö ¾ÊÀ½)
			if (txt == _T("¸®´õ±â1") || txt == _T("¸®´õ±â2") || txt == _T("Á¶È¸ ¹üÀ§"))
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

	// »ó´Ü Å¸ÀÌÆ² ºí·°(¸®´õ±â ¼³Á¤ + ¼­ºê + ¶óÀÎ)
	const int titleBlock = SX(92);        // ½º¼¦ ´À³¦À¸·Î ¾à°£ ¿©À¯
	const int sectionTitleH = SX(28);

	int y = inner.top + titleBlock;

	// ¼½¼Ç1 Á¦¸ñ À§Ä¡(¸®´õ±â ¼³Á¤)
	sec1TitlePt = CPoint(inner.left + SX(24), y);
	y += sectionTitleH + SX(10);

	// Ä«µå(¸®´õ±â1/2)
	const int cardH = SX(118);	// 2Çà ·¹ÀÌ¾Æ¿ô(¶óº§+ÄÞº¸Çà, ¹öÆ°Çà)
	const int cardGap = SX(14);

	card1 = CRect(inner.left, y, inner.right, y + cardH);
	y = card1.bottom + cardGap;

	card2 = CRect(inner.left, y, inner.right, y + cardH);
	y = card2.bottom + SX(22);

	// ¼½¼Ç2 Á¦¸ñ À§Ä¡(¸®´õ±â Á¤º¸)
	sec2TitlePt = CPoint(inner.left + SX(24), y);
	y += sectionTitleH + SX(10);

	// Á¶È¸ ¹Ú½º
	const int queryH = SX(62);
	queryBox = CRect(inner.left, y, inner.right, y + queryH);
	y = queryBox.bottom + SX(12);

	// ¸®½ºÆ®
	const int bottomBtnH = SX(42);
	const int bottomGap = SX(16);
	const int bottomArea = bottomBtnH + bottomGap + SX(8);

	int listH = inner.bottom - y - bottomArea;
	if (listH < SX(180)) listH = SX(180);
	listRc = CRect(inner.left, y, inner.right, y + listH);

	// ÇÏ´Ü ¹öÆ° (°¡¿îµ¥ Á¤·Ä)
	int bw = SX(108), bh = bottomBtnH, bgap = SX(14);
	int totalW = bw * 2 + bgap;
	int bx = inner.left + (inner.Width() - totalW) / 2;
	int by = inner.bottom - bh;

	okRc = CRect(bx, by, bx + bw, by + bh);
	cancelRc = CRect(bx + bw + bgap, by, bx + bw + bgap + bw, by + bh);

	// (infoTitleArea´Â ÇÊ¿ä ½Ã È®Àå¿ë)
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
		m_update1.EnableWindow(bEnable);
		m_togglePortOpen1.EnableWindow(bEnable);
		m_toggleMultipad1.EnableWindow(bEnable);
		m_bReader1Enabled = bEnable;
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

	// ÄÞº¸¹Ú½º´Â "Ç×»ó" È°¼º À¯Áö(¿ä±¸»çÇ×)
	if (readerIndex == 1) m_comport1.EnableWindow(TRUE);
	else                  m_comport2.EnableWindow(TRUE);

	// ¹Ì»ç¿ëÀÌ¸é ºñÈ°¼º, ±× ¿Ü´Â È°¼º
	BOOL bEnable = (sel != _T("¹Ì»ç¿ë"));
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

	const int padL   = SX(74);   // ¹èÁö(54) + ÁÂ¿ìÆÐµù
	const int padR   = SX(16);
	const int gap    = SX(8);
	const int comboW = SX(200);
	const int btnW   = SX(84);
	const int btnH   = SX(32);
	const int toggleW = SX(50);
	const int toggleH = SX(28);
	const int labelH  = SX(18);
	const int rowComboY = SX(28);  // Ä«µå »ó´Ü ¶óº§ ¾Æ·¡ ÄÞº¸Çà ¿ÀÇÁ¼Â
	const int rowBtnY  = SX(64);   // ¹öÆ°Çà ¿ÀÇÁ¼Â

	// Ä«µå ³»ºÎ ÄÁÆ®·Ñ ¹èÄ¡ ¶÷´Ù
	auto placeReaderCard = [&](
		const CRect& card,
		CSkinnedComboBox& cb,
		CModernButton& bInit,  CModernButton& bStatus,
		CModernButton& bKey,   CModernButton& bInteg,
		CModernButton& bUpdate,
		CModernToggleSwitch& tgOpen, CModernToggleSwitch& tgPad)
	{
		// --- ÄÞº¸ Çà ---
		int x0 = card.left + padL;
		int yCombo = card.top + rowComboY;
		cb.SetWindowPos(NULL, x0, yCombo, comboW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);

		// Æ÷Æ® ¿­±â Åä±Û
		int xToggleOpen = x0 + comboW + SX(76); // offset includes room for label
		tgOpen.SetWindowPos(NULL, xToggleOpen, yCombo + (btnH - toggleH) / 2,
			toggleW, toggleH, SWP_NOZORDER | SWP_NOACTIVATE);

		// ¸ÖÆ¼ÆÐµå ¿©ºÎ Åä±Û
		int xTogglePad = xToggleOpen + toggleW + SX(94); // offset includes room for label
		tgPad.SetWindowPos(NULL, xTogglePad, yCombo + (btnH - toggleH) / 2,
			toggleW, toggleH, SWP_NOZORDER | SWP_NOACTIVATE);

		// --- ¹öÆ° Çà ---
		int yBtn = card.top + rowBtnY;
		int bx = x0;
		bInit.SetWindowPos(NULL,   bx,                     yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
		bStatus.SetWindowPos(NULL, bx + (btnW + gap),      yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
		bKey.SetWindowPos(NULL,    bx + (btnW + gap) * 2,  yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
		bInteg.SetWindowPos(NULL,  bx + (btnW + gap) * 3,  yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
		bUpdate.SetWindowPos(NULL, bx + (btnW + gap) * 4,  yBtn, btnW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
	};

	placeReaderCard(card1, m_comport1,
		m_reader_init1, m_status_check1, m_keydown1, m_integrity_check1, m_update1,
		m_togglePortOpen1, m_toggleMultipad1);

	placeReaderCard(card2, m_comport2,
		m_reader_init2, m_status_check2, m_keydown2, m_integrity_check2, m_update2,
		m_togglePortOpen2, m_toggleMultipad2);

	// Á¶È¸ ¿µ¿ª
	int qx = queryBox.left + SX(24);
	int qy = queryBox.top + (queryBox.Height() - SX(32)) / 2;
	int qComboW = SX(140);

	m_search_date.SetWindowPos(NULL, qx, qy, qComboW, SX(32), SWP_NOZORDER | SWP_NOACTIVATE);

	if (CWnd* pSearch = GetDlgItem(IDC_SEARCH))
		pSearch->SetWindowPos(NULL, qx + qComboW + SX(10), qy,
			SX(74), SX(32), SWP_NOZORDER | SWP_NOACTIVATE);

	// ¸®½ºÆ®
	m_integrity_list.SetWindowPos(NULL, listRc.left, listRc.top, listRc.Width(), listRc.Height(),
		SWP_NOZORDER | SWP_NOACTIVATE);

	// ÇÏ´Ü ¹öÆ°
	if (CWnd* pOK = GetDlgItem(IDOK))
		pOK->SetWindowPos(NULL, okRc.left, okRc.top, okRc.Width(), okRc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);

	if (CWnd* pCancel = GetDlgItem(IDCANCEL))
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

	ModernUIGfx::EnsureGdiplusStartup();

	ModifyStyle(0, WS_CLIPCHILDREN);
	


	vector<int> ports;
	int windows_platform = GetWindowsVersion();
	if (windows_platform == WINDOWS_VERSION_95 || windows_platform == WINDOWS_VERSION_98)
		GetWidowsComPort(ports);
	else
		GetNTComPort(ports);

	m_comport1.AddString("¹Ì»ç¿ë");
	m_comport2.AddString("¹Ì»ç¿ë");

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
		com_port1 = "¹Ì»ç¿ë";
		AfxGetApp()->WriteProfileString(SERIAL_PORT_SECTION, COMPORT1_FIELD, com_port1);
	}
	if (com_port2.GetLength() == 0) {
		com_port2 = "¹Ì»ç¿ë";
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

	if (m_comport1.GetCurSel() == 0 && com_port1.Compare("¹Ì»ç¿ë") != 0) {
		com_port1 += "(»ç¿ëºÒ°¡)";
		m_comport1.AddString(com_port1);
		m_comport1.SetCurSel(ports.size() + 1);
	}
	if (m_comport2.GetCurSel() == 0 && com_port2.Compare("¹Ì»ç¿ë") != 0) {
		com_port2 += "(»ç¿ëºÒ°¡)";
		m_comport2.AddString(com_port2);
		m_comport2.SetCurSel(ports.size() + 1);
	}

	if (com_port1.Compare("¹Ì»ç¿ë") == 0) {
		m_reader_init1.EnableWindow(FALSE);
		m_status_check1.EnableWindow(FALSE);
		m_keydown1.EnableWindow(FALSE);
		m_integrity_check1.EnableWindow(FALSE);
	}
	if (com_port2.Compare("¹Ì»ç¿ë") == 0) {
		m_reader_init2.EnableWindow(FALSE);
		m_status_check2.EnableWindow(FALSE);
		m_keydown2.EnableWindow(FALSE);
		m_integrity_check2.EnableWindow(FALSE);
	}

	m_search_date.AddString("¿À´Ã");
	m_search_date.AddString("7ÀÏ");
	m_search_date.AddString("30ÀÏ");
	m_search_date.AddString("100ÀÏ");

	m_search_date.SetCurSel(0);

	ListView_SetExtendedListViewStyle(m_integrity_list.GetSafeHwnd(), LVS_EX_GRIDLINES);
	for(i = 0 ; col_info[i].column_name != NULL ; ++i)
		m_integrity_list.InsertColumn(i, col_info[i].column_name, col_info[i].format, col_info[i].width * 8);



	// ... (±âÁ¸ Æ÷Æ® ¸ñ·Ï/·¹Áö½ºÆ®¸®/ÃÊ±â Enable ·ÎÁ÷ ±×´ë·Î À¯Áö)

	// DPI/ÆùÆ®/·¹ÀÌ¾Æ¿ô/±¸ static ¼û±è
	HDC hdc = ::GetDC(m_hWnd);
	m_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
	::ReleaseDC(m_hWnd, hdc);

	EnsureFonts();
	HideLegacyStatics();

	// === ModernUI ½ºÅ¸ÀÏ Àû¿ë ===
	// ÄÞº¸¹Ú½º ½ºÅ²

	// ¾×¼Ç ¹öÆ° ½ºÅ¸ÀÏ (Secondary: È¸»ö Å×µÎ¸®)
	m_reader_init1.SetButtonStyle(ButtonStyle::Default);
	m_status_check1.SetButtonStyle(ButtonStyle::Default);
	m_keydown1.SetButtonStyle(ButtonStyle::Default);
	m_integrity_check1.SetButtonStyle(ButtonStyle::Default);
	m_update1.SetButtonStyle(ButtonStyle::Default);

	m_reader_init2.SetButtonStyle(ButtonStyle::Default);
	m_status_check2.SetButtonStyle(ButtonStyle::Default);
	m_keydown2.SetButtonStyle(ButtonStyle::Default);
	m_integrity_check2.SetButtonStyle(ButtonStyle::Default);
	m_update2.SetButtonStyle(ButtonStyle::Default);

	// ÇÏ´Ü È®ÀÎ/Ãë¼Ò ¹öÆ° ½ºÅ¸ÀÏ
	if (CWnd* pOK = GetDlgItem(IDOK))
		m_btnOk.SubclassDlgItem(IDOK, this);
	if (CWnd* pCancel = GetDlgItem(IDCANCEL))
		m_btnCancel.SubclassDlgItem(IDCANCEL, this);
	m_btnOk.SetButtonStyle(ButtonStyle::Primary);
	m_btnCancel.SetButtonStyle(ButtonStyle::Default);

	// Á¶È¸ ¹öÆ°
	m_btnSearch.SubclassDlgItem(IDC_SEARCH, this);
	m_btnSearch.SetButtonStyle(ButtonStyle::Primary);

	// Åä±Û ÃÊ±â »óÅÂ (Æ÷Æ® ¿­±â: ¹Ì»ç¿ëÀÌ¸é OFF)
	m_togglePortOpen1.SetCheck(BST_UNCHECKED);
	m_togglePortOpen2.SetCheck(BST_UNCHECKED);
	m_toggleMultipad1.SetCheck(BST_UNCHECKED);
	m_toggleMultipad2.SetCheck(BST_UNCHECKED);

	// Set underlay colors to match card backgrounds
	COLORREF cardBgEnabled  = RGB(240, 248, 255);
	COLORREF cardBgDisabled = RGB(245, 246, 248);
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

	m_bUIReady = TRUE;
	ModifyStyle(0, WS_CLIPCHILDREN);

	FitWindowToLayout();
	LayoutControls();

	// ÃÊ±â »óÅÂ ¹Ý¿µ(ÄÞº¸´Â Ç×»ó È°¼º, ¹öÆ°Àº ¹Ì»ç¿ëÀÌ¸é ºñÈ°¼º)
	UpdateReaderEnableState(1);
	UpdateReaderEnableState(2);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

CSize CReaderSetupDlg::CalcMinClientSize() const
{
	// LayoutControls¿¡¼­ ¾²´Â ¼öÄ¡¿Í ¸ÂÃç¾ß ÇÔ
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

	const int listMinH = SX(220);   // Ç¥´Â ÃÖ¼Ò ÀÌ Á¤µµ´Â º¸¿©ÁÖÀÚ(¿øÇÏ¸é Á¶Á¤)

	const int bottomBtnH = SX(40);
	const int bottomGap = SX(16);
	const int bottomArea = bottomBtnH + bottomGap + SX(10);

	// ---- °¡·Î ÃÖ¼Ò: Ä«µå ³»ºÎ¿¡ ÇÑ ÁÙ·Î µé¾î°¡°Ô ----
	int innerW_need = 0;
	{
		int needRowW = badge + gap + comboW + gap + (btnW * 4) + (gap * 3);
		// Ä«µå ¾ÈÂÊ ÆÐµù °í·Á
		innerW_need = (cardPadX * 2) + needRowW;
	}

	// ¹Ù±ù margin °í·Á (Å¬¶óÀÌ¾ðÆ® ÀüÃ¼ Æø)
	int clientW = (margin * 2) + innerW_need;

	// ---- ¼¼·Î ÃÖ¼Ò ----
	int clientH = 0;
	clientH += margin;                  // top
	clientH += SX(18) + titleBlk;        // Å¸ÀÌÆ² ¿µ¿ª(¿©À¯ Æ÷ÇÔ)
	clientH += secTitleH + SX(10);       // ¼½¼Ç Å¸ÀÌÆ² + °£°Ý

	clientH += cardH;                   // Ä«µå1
	clientH += cardGap;
	clientH += cardH;                   // Ä«µå2

	clientH += secGapAfterCards;        // ¼½¼Ç °£°Ý

	clientH += queryBoxH;               // Á¶È¸ ¹Ú½º
	clientH += queryGap;

	clientH += listMinH;                // ¸®½ºÆ® ÃÖ¼Ò ³ôÀÌ

	clientH += bottomArea;              // ÇÏ´Ü ¹öÆ° ¿µ¿ª
	clientH += margin;                  // bottom

	return CSize(clientW, clientH);
}

void CReaderSetupDlg::FitWindowToLayout()
{
	if (m_bFitDone) return;
	m_bFitDone = TRUE;

	CSize needClient = CalcMinClientSize();

	// Å¬¶óÀÌ¾ðÆ® Å©±â¸¦ ±âÁØÀ¸·Î ½ÇÁ¦ À©µµ¿ì Å©±â °è»ê
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
	return TRUE; // ÀüÃ¼¸¦ OnPaint¿¡¼­ Ä¥ÇÔ
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

	// ¸ÞÀÎ Ä«µå
	const int margin = SX(18);
	CRect mainCard(rc.left + margin, rc.top + margin, rc.right - margin, rc.bottom - margin);
	CRect shadow = mainCard; shadow.OffsetRect(SX(2), SX(3));
	FillRoundRect(&memDC, shadow, SX(18), RGB(230, 235, 240), RGB(230, 235, 240), 1);
	FillRoundRect(&memDC, mainCard, SX(18), RGB(255, 255, 255), RGB(220, 226, 232), 1);

	// Header icon: blue rounded square with card-reader symbol
	memDC.SetBkMode(TRANSPARENT);
	{
		const int iconSize = SX(46);
		const int iconX    = mainCard.left + SX(18);
		const int iconY    = mainCard.top  + SX(16);
		CRect rcIcon(iconX, iconY, iconX + iconSize, iconY + iconSize);
		FillRoundRect(&memDC, rcIcon, SX(10), RGB(0, 100, 221), RGB(0, 100, 221), 1);
		// Card shape (white rectangle)
		memDC.FillSolidRect(iconX + SX(9),  iconY + SX(12), SX(28), SX(18), RGB(255, 255, 255));
		// Magnetic stripe (dark band)
		memDC.FillSolidRect(iconX + SX(9),  iconY + SX(16), SX(28), SX(4),  RGB(0, 60, 140));
		// Button dots
		memDC.FillSolidRect(iconX + SX(11), iconY + SX(34), SX(5), SX(5), RGB(255, 255, 255));
		memDC.FillSolidRect(iconX + SX(20), iconY + SX(34), SX(5), SX(5), RGB(255, 255, 255));
		memDC.FillSolidRect(iconX + SX(29), iconY + SX(34), SX(5), SX(5), RGB(255, 255, 255));
	}
	const int titleTextX = mainCard.left + SX(74);
	memDC.SelectObject(&m_fontTitle);
	memDC.SetTextColor(RGB(6, 52, 109));
	memDC.TextOut(titleTextX, mainCard.top + SX(18), _T("¸®´õ±â ¼³Á¤"));

	memDC.SelectObject(&m_fontSub);
	memDC.SetTextColor(RGB(0, 100, 180));
	memDC.TextOut(titleTextX, mainCard.top + SX(44), _T("¸®´õ±â ¿¬°á ¹× Á¦¾î¸¦ °ü¸®ÇÕ´Ï´Ù"));
	memDC.FillSolidRect(mainCard.left + SX(24), mainCard.top + SX(72), mainCard.Width() - SX(48), 1, RGB(235, 240, 245));

	// µ¿ÀÏ Rect ±â¹ÝÀ¸·Î ¼½¼Ç/Ä«µå ¿µ¿ª ¾ò±â
	CRect inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc;
	CPoint sec1Pt, sec2Pt;
	CalcLayoutRects(inner, card1, card2, infoTitleArea, queryBox, listRc, okRc, cancelRc, sec1Pt, sec2Pt);

	// ¼½¼Ç Á¦¸ñ 2°³(¿øÇÏ´Â´ë·Î º¸¿©ÁÖ±â)
	// Section titles with left blue bar indicator
	memDC.SelectObject(&m_fontNormal);
	memDC.SetTextColor(RGB(45, 55, 72));
	memDC.FillSolidRect(sec1Pt.x - SX(10), sec1Pt.y + SX(2), SX(3), SX(18), RGB(0, 100, 221));
	memDC.TextOut(sec1Pt.x, sec1Pt.y, _T("Æ÷Æ® ¼³Á¤"));
	memDC.FillSolidRect(sec2Pt.x - SX(10), sec2Pt.y + SX(2), SX(3), SX(18), RGB(0, 100, 221));
	memDC.TextOut(sec2Pt.x, sec2Pt.y, _T("¹«°á¼º Ã¼Å© Á¤º¸"));

	// Ä«µå 2°³ (2Çà ·¹ÀÌ¾Æ¿ô: ¶óº§+ÄÞº¸+Åä±Û / ¹öÆ°5°³)
	auto drawCard = [&](const CRect& r, BOOL enabled, int num)
	{
		COLORREF bg  = enabled ? RGB(240, 248, 255) : RGB(245, 246, 248);
		COLORREF brd = enabled ? RGB(0, 118, 190)   : RGB(220, 226, 232);
		int bw = enabled ? 2 : 1;
		FillRoundRect(&memDC, r, SX(14), bg, brd, bw);

		// ¹èÁö (¹øÈ£)
		const int badgeSize = SX(38);
		CRect badge(
			r.left + SX(16),
			r.top  + (r.Height() - badgeSize) / 2,
			r.left + SX(16) + badgeSize,
			r.top  + (r.Height() - badgeSize) / 2 + badgeSize);
		FillRoundRect(&memDC, badge, SX(10),
			enabled ? RGB(0, 90, 156) : RGB(160, 168, 176),
			enabled ? RGB(0, 90, 156) : RGB(160, 168, 176), 1);
		memDC.SelectObject(&m_fontNormal);
		memDC.SetTextColor(RGB(255, 255, 255));
		CString s; s.Format(_T("%d"), num);
		memDC.DrawText(s, badge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		// »ó´Ü ¶óº§ "¸®´õ±âN - COM Æ÷Æ®"
		const int padL = SX(74);
		memDC.SelectObject(&m_fontSmall);
		memDC.SetTextColor(enabled ? RGB(60, 80, 100) : RGB(160, 168, 176));
		CString label;
		label.Format(_T("¸®´õ±â%d - COM Æ÷Æ®"), num);
		memDC.TextOut(r.left + padL, r.top + SX(10), label);

		// "Æ÷Æ® ¿­±â" / "¸ÖÆ¼ÆÐµå ¿©ºÎ" ¶óº§ (Åä±Û ¿ÞÂÊ)
		const int comboW  = SX(200);
		const int btnH    = SX(32);
		const int toggleW = SX(50);
		const int yComboRow = r.top + SX(28);

		int xToggleOpenLabel = r.left + padL + comboW + SX(20);
		memDC.SetTextColor(enabled ? RGB(60, 80, 100) : RGB(180, 185, 190));
		memDC.TextOut(xToggleOpenLabel, yComboRow + (btnH - SX(14)) / 2, _T("Æ÷Æ® ¿­±â"));

		int xTogglePadLabel = r.left + padL + comboW + SX(76) + toggleW + SX(20);
		memDC.TextOut(xTogglePadLabel, yComboRow + (btnH - SX(14)) / 2, _T("¸ÖÆ¼ÆÐµå ¿©ºÎ"));
	};

	drawCard(card1, m_bReader1Enabled, 1);
	drawCard(card2, m_bReader2Enabled, 2);

	// Á¶È¸ ¹Ú½º - "Á¶È¸ ¹üÀ§" ¶óº§ Æ÷ÇÔ
	FillRoundRect(&memDC, queryBox, SX(14), RGB(248, 249, 251), RGB(230, 235, 240), 1);
	memDC.SelectObject(&m_fontSmall);
	memDC.SetTextColor(RGB(60, 80, 100));
	memDC.TextOut(queryBox.left + SX(24), queryBox.top + SX(8), _T("Á¶È¸ ¹üÀ§"));

	// Draw empty-state text when list has no items
	if (m_bUIReady && m_integrity_list.GetSafeHwnd() && m_integrity_list.GetItemCount() == 0) {
		CWindowDC listDC(&m_integrity_list);
		CRect listWinRc;
		m_integrity_list.GetClientRect(&listWinRc);
		CHeaderCtrl* pHdr = m_integrity_list.GetHeaderCtrl();
		int hdrH = 0;
		if (pHdr && pHdr->GetSafeHwnd()) {
			RECT rcH; pHdr->GetWindowRect(&rcH);
			hdrH = rcH.bottom - rcH.top;
		}
		CRect bodyRc(listWinRc.left, listWinRc.top + hdrH, listWinRc.right, listWinRc.bottom);
		listDC.FillSolidRect(bodyRc, RGB(255, 255, 255));
		CFont* pOld = listDC.SelectObject(&m_fontSmall);
		listDC.SetBkMode(TRANSPARENT);
		listDC.SetTextColor(RGB(160, 168, 180));
		listDC.DrawText(_T("ì¡°í???ë¬´ê²°??ì²´í? ??³´ê°€ ??????."), bodyRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		listDC.SelectObject(pOld);
	}
	
	dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);
	memDC.SelectObject(oldBmp);
}


void CReaderSetupDlg::OnSelchangeComport1() 
{
	UpdateReaderEnableState(1);
}

void CReaderSetupDlg::OnSelchangeComport2() 
{
	UpdateReaderEnableState(2);
}



BOOL CReaderSetupDlg::DestroyWindow() 
{

	return CDialog::DestroyWindow();
}

