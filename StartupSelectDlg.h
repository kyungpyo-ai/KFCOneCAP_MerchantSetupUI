#pragma once

#include "resource.h"

class CStartupSelectDlg : public CDialog
{
public:
    enum { IDD = IDD_STARTUP_SELECT_DLG };

    enum START_CHOICE
    {
        CHOICE_SHOP_SETUP = 0,
        CHOICE_READER_SETUP = 1,
        CHOICE_KFTC_ONECAP = 2
    };

public:
    CStartupSelectDlg(CWnd* pParent = NULL);
    START_CHOICE GetChoice() const { return m_choice; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    afx_msg void OnBtnShopSetup();
    afx_msg void OnBtnReaderSetup();
    afx_msg void OnBtnKFTCOneCAP();

    DECLARE_MESSAGE_MAP()

private:
    START_CHOICE m_choice;
};
