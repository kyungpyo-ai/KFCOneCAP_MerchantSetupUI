#pragma once
// StartupSelectDlg.h : 시작 다이얼로그(가맹점 설정/리더기 설정 선택)
//
// [v2.0 추가]
// 프로그램 실행 시 바로 ShopSetupDlg를 띄우지 않고,
// 사용자가 어떤 설정 화면을 열지 선택하도록 한다.
//
// 주의: 소스는 EUC-KR(CP949) 인코딩으로 저장해야 한글이 깨지지 않습니다.
//

class CStartupSelectDlg : public CDialog
{
public:
    enum { IDD = IDD_STARTUP_SELECT_DLG };

    enum START_CHOICE
    {
        CHOICE_SHOP_SETUP   = 0,
        CHOICE_READER_SETUP = 1
    };

public:
    CStartupSelectDlg(CWnd* pParent = NULL);

    START_CHOICE GetChoice() const { return m_choice; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void OnBtnShopSetup();
    afx_msg void OnBtnReaderSetup();

    DECLARE_MESSAGE_MAP()

private:
    START_CHOICE m_choice;
};
