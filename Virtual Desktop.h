// Virtual Desktop.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "Virtual DesktopDlg.h"
#include "CommonDef.h"

// CVirtualDesktopApp:
// See Virtual Desktop.cpp for the implementation of this class
//

class CVirtualDesktopApp : public CWinApp
{
public:
	CVirtualDesktopApp();

// Overrides
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	DECLARE_MESSAGE_MAP()
private:
	CVirtualDesktopDlg *m_pApplicationWnd;
};

extern CVirtualDesktopApp theApp;