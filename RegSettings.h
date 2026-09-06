// RegSettings.h: interface for the CRegSettings class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGSETTINGS_H__5EFB76B3_4780_40CB_B12D_2FF95AE235CA__INCLUDED_)
#define AFX_REGSETTINGS_H__5EFB76B3_4780_40CB_B12D_2FF95AE235CA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



class CRegSettings  
{
public:
	//static CString m_strSubKey;
	CRegSettings();

	virtual ~CRegSettings();

	BOOL SetProfileString(LPCTSTR, LPCTSTR, LPCTSTR);
	BOOL SetProfileInt(LPCTSTR, LPCTSTR, int);

	CString ReadProfileString(LPCTSTR, LPCTSTR, LPCTSTR);

	UINT ReadProfileInt(LPCTSTR, LPCTSTR, int);
};

#endif // !defined(AFX_REGSETTINGS_H__5EFB76B3_4780_40CB_B12D_2FF95AE235CA__INCLUDED_)


