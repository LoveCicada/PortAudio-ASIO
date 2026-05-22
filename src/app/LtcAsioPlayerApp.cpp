#include "app/LtcAsioPlayerApp.h"

#include "ui/MainDlg.h"

CLtcAsioPlayerApp theApp;

CLtcAsioPlayerApp::CLtcAsioPlayerApp()
{
}

BOOL CLtcAsioPlayerApp::InitInstance()
{
    CWinApp::InitInstance();

    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&icc);

    CMainDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}
