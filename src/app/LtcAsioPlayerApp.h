#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#include <afxwin.h>

class CLtcAsioPlayerApp : public CWinApp
{
public:
    CLtcAsioPlayerApp();

    virtual BOOL InitInstance() override;
};

extern CLtcAsioPlayerApp theApp;
