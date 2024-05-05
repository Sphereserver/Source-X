/**
* @file CSWindow.h
* @brief Base window class for controls on MS Windows OS.
*/

#ifndef _INC_CSWINDOW_H
#define _INC_CSWINDOW_H

#ifdef _WIN32

#include "CSString.h"
#include <richedit.h>	// CRichEditCtrl
#include <shellapi.h>


struct CSWindow    // similar to Std MFC class CWnd
{
	static const char *m_sClassName;
	HWND m_hWnd;
	NOTIFYICONDATA m_pnid;

	operator HWND () const;       // cast as a HWND

	CSWindow();
	virtual ~CSWindow();
	CSWindow(const CSWindow& copy) = delete;
	CSWindow& operator=(const CSWindow& other) = delete;

	// Standard message handlers.
	BOOL OnCreate( HWND hwnd, LPCREATESTRUCT lpCreateStruct = nullptr );
	void OnDestroy();
	void OnDestroy( HWND hwnd );

	// Basic window functions.
	BOOL IsWindow() const;
	HWND GetParent() const;
	LRESULT SendMessage( UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0 ) const;
	BOOL PostMessage( UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0 ) const;
	HWND GetDlgItem( int id ) const;
	BOOL SetDlgItemText( int nIDDlgItem, LPCTSTR lpString );

	// Create/Destroy
	void DestroyWindow();

	// Area and location
	BOOL MoveWindow( int X, int Y, int nWidth, int nHeight, BOOL bRepaint = TRUE );
	BOOL SetForegroundWindow();
	HWND SetFocus();
	BOOL ShowWindow( int nCmdShow );

	// Standard windows props.
	int GetWindowText( LPTSTR lpszText, int iLen );
	BOOL SetWindowText( LPCTSTR lpszText );
	void SetFont( HFONT hFont, BOOL fRedraw = false );
	HICON SetIcon( HICON hIcon, BOOL fType = false );
	UINT_PTR SetTimer( UINT_PTR uTimerID, UINT uWaitmSec );
	BOOL KillTimer( UINT_PTR uTimerID );
	int MessageBox( lpctstr lpszText, lpctstr lpszTitle, UINT fuStyle = MB_OK ) const;
	INT_PTR SetWindowLongPtr( int nIndex, INT_PTR dwNewLong );
	INT_PTR GetWindowLongPtr( int nIndex ) const;
	int SetDlgItemText( int ID, lpctstr lpszText ) const;
};

struct CDialogBase : public CSWindow
{
	static const char *m_sClassName;
	static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

    CDialogBase() = default;
	virtual ~CDialogBase() = default;

	virtual BOOL DefDialogProc( UINT message, WPARAM wParam, LPARAM lParam );
};

struct CSWindowBase : public CSWindow
{
	static const char *m_sClassName;
	static LRESULT WINAPI WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	static ATOM RegisterClass( WNDCLASS & wc );
	virtual LRESULT DefWindowProc( UINT message, WPARAM wParam, LPARAM lParam );
};

struct CWinApp	// Similar to MFC type
{
    static const char *m_sClassName;
    lpctstr	 	m_pszAppName;	// Specifies the name of the application. (display freindly)
    HINSTANCE 	m_hInstance;	// Identifies the current instance of the application.
    lptstr 		m_lpCmdLine;	// Points to a null-terminated string that specifies the command line for the application.
    CSWindow *	m_pMainWnd;		// Holds a pointer to the application's main window. For an example of how to initialize m_pMainWnd, see InitInstance.
    CSString	m_pszExeName;	// The module name of the application.
    CSString	m_pszProfileName;	// the path to the profile.

    CWinApp();
    virtual ~CWinApp() = default;

    CWinApp(const CWinApp& copy) = delete;
    CWinApp& operator=(const CWinApp& other) = delete;

    void InitInstance( LPCTSTR pszAppName, HINSTANCE hInstance, LPTSTR lpszCmdLine );
    HICON LoadIcon( int id ) const;
    HMENU LoadMenu( int id ) const;
};


struct CScrollBar : public CSWindow
{
    // Constructors
	static const char *m_sClassName;
	CScrollBar() = default;
    virtual ~CScrollBar() = default;

    // Attributes
	void GetScrollRange(LPINT lpMinPos, LPINT lpMaxPos) const;
	BOOL GetScrollInfo(LPSCROLLINFO lpScrollInfo, UINT nMask);
};

struct CEdit : public CSWindow
{
    // Constructors
	static const char *m_sClassName;
	CEdit() = default;
    virtual ~CEdit() = default;

    // Operations
	void SetSel( DWORD dwSelection, BOOL bNoScroll = FALSE );
	void SetSel( size_t nStartChar, size_t nEndChar, BOOL bNoScroll = FALSE );
	size_t GetSel() const;
	void GetSel(size_t& nStartChar, size_t& nEndChar) const;
	void ReplaceSel( lpctstr lpszNewText, BOOL bCanUndo = FALSE );
};

struct CRichEditCtrl : public CEdit
{
	static const char *m_sClassName;
	COLORREF SetBackgroundColor( BOOL bSysColor, COLORREF cr );

	void SetSel( int nStartChar, int nEndChar );
	void GetSel( int& nStartChar, int& nEndChar ) const;

    void SetRedraw(BOOL val);
    void SetCaretHide(BOOL val);

	DWORD ScrollLine();
    DWORD ScrollPageDown();
    DWORD ScrollBottomRight();

	// Formatting.
	BOOL SetDefaultCharFormat( CHARFORMAT& cf );
	BOOL SetSelectionCharFormat( CHARFORMAT& cf );

	// Events.
	int GetEventMask() const;
	DWORD SetEventMask(DWORD dwEventMask = ENM_NONE );
};

struct CListbox : public CSWindow
{
    // Constructors
	static const char *m_sClassName;
	CListbox() = default;
    virtual ~CListbox() = default;

    // Operations
	void ResetContent();
	int GetCount() const;
	int AddString( LPCTSTR lpsz ) const;
};

#endif	// _WIN32

#endif	// _INC_CSWINDOW_H
