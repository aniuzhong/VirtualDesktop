// Virtual DesktopDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "CommonDef.h"

// CVirtualDesktopDlg dialog
class CVirtualDesktopDlg : public CDialog
{
// Construction
public:
	CVirtualDesktopDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_VIRTUALDESKTOP_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()

public:
	LRESULT OnTrayMessage(WPARAM wParam, LPARAM lParam);

	afx_msg void OnBnClickedSwitchToDesktop();
	afx_msg void OnDestroy();
	afx_msg void OnLbnSelchangeDesktopList();
	afx_msg void OnBnClickedAddNewDesktop();

private:
	CListBox m_DesktopListControl;
	CEdit m_DesktopNameControl;

	CButton m_AddNewDesktop;
	CButton m_SwitchToDesktop;
	CButton m_ChkVerifyDesktopSwitch;

	void ShowManageDesktopsDialog(void);
	void SwitchDesktopTo(TCHAR * szDesktopName);

	bool RegisterApplicationHotKeys(void);
	bool UnRegisterApplicationHotKeys(void);
	bool UpdateHotKeys(void);

	afx_msg void OnBnClickedVerifyCheck();
	afx_msg void OnBnClickedLaunchApplication();

	LRESULT OnHotKey(WPARAM wParam, LPARAM lParam);
};
