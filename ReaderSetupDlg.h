#if !defined(AFX_READERSETUPDLG_H__183B0FB7_82FE_468C_855D_25D47BF512A5__INCLUDED_)
#define AFX_READERSETUPDLG_H__183B0FB7_82FE_468C_855D_25D47BF512A5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ReaderSetupDlg.h : header file
//

#include <vector>
#include "ModernUI.h"

#define WM_READER_DONE    (WM_APP + 100)
#define WM_PORT_OPEN_DONE (WM_APP + 101)


using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CReaderSetupDlg dialog

class CReaderSetupDlg : public CDialog
{

protected:
	CSize CalcMinClientSize() const;
	void  FitWindowToLayout();   // 계산값으로 창 크기 자동 조절
	BOOL  m_bFitDone;            // 무한루프 방지(1회만)


	void CalcLayoutRects(
		CRect& inner,
		CRect& card1, CRect& card2,
		CRect& infoTitleArea,
		CRect& queryBox,
		CRect& listRc,
		CRect& okRc, CRect& cancelRc,
		CPoint& sec1TitlePt,
		CPoint& sec2TitlePt
	) const;

	CRect CalcPortSectionBox(const CRect& card1, const CRect& card2) const;
	CRect CalcIntegritySectionBox(const CRect& queryBox, const CRect& listRc) const;

	virtual void DoDataExchange(CDataExchange* pDX);
	BOOL m_bUiInitialized;

	// === 추가: UI 상태/폰트/레이아웃 ===
	BOOL  m_bReader1Enabled;
	BOOL  m_bReader2Enabled;

	CBrush m_brushBg;    // dialog background RGB(249,250,252)
	CFont m_fontTitle;
	CFont m_fontSub;
	CFont m_fontSection;
	CFont m_fontNormal;
	CFont m_fontLabel;
	CFont m_fontSmall;

	// GDI+ section title font (cached, matches ShopSetupDlg DrawMinCard style)
	Gdiplus::FontFamily* m_pGdipSecFamily;
	Gdiplus::Font*       m_pGdipSecFont;
	Gdiplus::Font*       m_pGdipHdrTitleFont;
	Gdiplus::Font*       m_pGdipHdrSubFont;

	// Popover info icon buttons
	CInfoIconButton m_btnPortOpenInfo;
	CInfoIconButton m_btnMultipad1Info;
	CInfoIconButton m_btnMultipad2Info;
	CModernPopover  m_popover;

	int   SX(int v) const; // scale x/y 공용
	void  DrawSectionTitle(CDC& dc, CPoint pt, LPCTSTR text);
	void  EnsureFonts();
	void  LayoutControls();

	void  UpdateReaderEnableState(int readerIndex); // 1 or 2
	void  ApplyEnableStateToButtons(int readerIndex, BOOL bEnable);
    void  ApplyAopRestrictions();             // Disable reader2 + reader1 init when INTERLOCK=AOP

	void  HideLegacyStatics(); // "리더기1/2/조회 범위" 같은 기존 static 숨김
	void  RecalcIntegrityColumns(); // 무결성 리스트 컬럼 너비를 화면 폭에 맞게 재계산
	int   m_nIntegrityScrollPos;
	CRect m_rcIntegrityScrollBar;
	CRect m_rcIntegrityScrollThumb;
	BOOL  m_bDraggingThumb;
	int   m_nDragStartY;
	int   m_nDragStartScrollPos;
	void  NormalizeIntegrityScrollPos();
	int   GetIntegrityVisibleRows() const;
	void  StartLoadingOperation(UINT nButtonID);
	void  FinishLoadingOperation(BOOL bRefresh = TRUE);
	BOOL  IsReaderCardButton(UINT nID) const;
	BOOL  IsSearchLoading() const { return m_nLoadingButtonID == IDC_SEARCH; }
	void  SetReaderCardBusy(int readerIndex, BOOL bBusy);
	void  SetSearchBusy(BOOL bBusy);
	CString MakeLoadingText(UINT nButtonID) const;

	// === 초기화/레이아웃/테이블 리팩토링 헬퍼 ===
	void  InitPortComboItems();              // COM 포트 목록 구성
	void  LoadSavedPortSelections();         // 저장된 포트 선택값 복원
	void  InitSearchDateCombo();             // 조회 기간 콤보 초기화
	void  InitIntegrityListColumns();        // 무결성 리스트 컬럼 생성
	void  ApplyDialogFonts();                // 대화상자 공통 폰트 적용
	void  InitModernButtonStyles();          // ModernUI 버튼 스타일/문구 적용
	void  InitSampleIntegrityRows();         // 커스텀 표 미리보기 샘플 데이터
	void  InitToggleAndUnderlayColors();     // 토글/버튼 배경색 초기화

	UINT  m_nLoadingButtonID;
	UINT  m_nLoadingTimerID;
	UINT  m_nLoadingAnimTimerID;
	UINT_PTR m_nPortOpenTimerID;  // 10-second timeout for port open operation
	int   m_nComport1PrevSel;
	int   m_nComport2PrevSel;
	int   m_nBusyReaderIndex;
	BOOL  m_bBusySearch;

	//{{AFX_MSG(CReaderSetupDlg)

	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT OnReaderDone(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPortOpenDone(WPARAM wParam, LPARAM lParam);

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	
// Construction
public:
	CReaderSetupDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CReaderSetupDlg)
	enum { IDD = IDD_READER_SETUP_DIALOG };
	CListCtrl			m_integrity_list;
	CSkinnedComboBox	m_search_date;
	CModernButton		m_reader_init2;
	CModernButton		m_reader_init1;
	CModernButton		m_status_check2;
	CModernButton		m_status_check1;
	CModernButton		m_keydown2;
	CModernButton		m_keydown1;
	CModernButton		m_integrity_check2;
	CModernButton		m_integrity_check1;
	CSkinnedComboBox	m_comport2;
	CSkinnedComboBox	m_comport1;
	// 업데이트 버튼 (신규)
	CModernButton		m_update1;
	CModernButton		m_update2;
	// 포트 열기 / 멀티패드 토글
	CModernToggleSwitch	m_togglePortOpen1;
	CModernToggleSwitch	m_togglePortOpen2;
	CModernToggleSwitch	m_toggleMultipad1;
	CModernToggleSwitch	m_toggleMultipad2;
	// 하단 확인/취소 버튼
	CModernButton		m_btnOk;
	CModernButton		m_btnCancel;
	// 조회 버튼
	CModernButton		m_btnSearch;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReaderSetupDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:

	//}}AFX_VIRTUAL

// Implementation
protected:
	void GetNTComPort(vector<int>& ports);

	// Generated message map functions
	//{{AFX_MSG(CReaderSetupDlg)
	virtual BOOL OnInitDialog();
	
	afx_msg void OnSelchangeComport1();
	afx_msg void OnSelchangeComport2();
	afx_msg void OnBnClickedPortOpenInfo();
	afx_msg void OnBnClickedMultipad1Info();
	afx_msg void OnBnClickedMultipad2Info();
	afx_msg void OnPortOpen1Clicked();


	//}}AFX_MSG
	void ShowInfoPopover(CInfoIconButton& btn, LPCTSTR szTitle, LPCTSTR szBody);


	struct SettingsSnapshot {
		int  cmbPort1, cmbPort2;
		BOOL tglPortOpen1, tglPortOpen2;
		BOOL tglMultipad1, tglMultipad2;
	};
	SettingsSnapshot m_snap;
	void TakeSnapshot();
	BOOL HasChanges() const;

	virtual void OnOK();
	virtual void OnCancel();

	// Button action handlers (to be implemented)
	void OnReaderInit(int readerIndex);
	void OnStatusCheck(int readerIndex);
	void OnKeyDown(int readerIndex);
	void OnIntegrityCheck(int readerIndex);
	void OnUpdate(int readerIndex);
	void OnSearch(BOOL isLoading);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_READERSETUPDLG_H__183B0FB7_82FE_468C_855D_25D47BF512A5__INCLUDED_)
