/**
* @file CSWindow.cpp
* @brief Base window class for controls.
*/

#ifdef _WIN32

#include "CSWindow.h"


/* CSWindow */

CSWindow::operator HWND () const       // cast as a HWND
{
    return m_hWnd;
}
CSWindow::CSWindow() : m_pnid{}
{
    m_hWnd = nullptr;
}
CSWindow::~CSWindow()
{
    DestroyWindow();
}

// Standard message handlers.
BOOL CSWindow::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    UnreferencedParameter(lpCreateStruct);
    m_hWnd = hwnd;
    return true;
}

void CSWindow::OnDestroy()
{
    m_hWnd = nullptr;
}

void CSWindow::OnDestroy( HWND hwnd )
{
    UnreferencedParameter(hwnd);
    m_hWnd = nullptr;
}

// Basic window functions.
BOOL CSWindow::IsWindow() const
{
    if (m_hWnd == nullptr)
        return false;
    return ::IsWindow(m_hWnd);
}

HWND CSWindow::GetParent() const
{
    ASSERT(m_hWnd);
    return ::GetParent(m_hWnd);
}

LRESULT CSWindow::SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) const
{
    ASSERT(m_hWnd);
    return ::SendMessage(m_hWnd, uMsg, wParam, lParam);
}

BOOL CSWindow::PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) const
{
    ASSERT(m_hWnd);
    return ::PostMessage(m_hWnd, uMsg, wParam, lParam);
}

HWND CSWindow::GetDlgItem(int id) const
{
    ASSERT(m_hWnd);
    return ::GetDlgItem(m_hWnd, id);
}

BOOL CSWindow::SetDlgItemText(int nIDDlgItem, LPCTSTR lpString)
{
    ASSERT(m_hWnd);
    return ::SetDlgItemText(m_hWnd, nIDDlgItem, lpString);
}

// Create/Destroy
void CSWindow::DestroyWindow()
{
    if (m_hWnd == nullptr)
        return;
    ::DestroyWindow(m_hWnd);
    ASSERT(m_hWnd == nullptr);
}

// Area and location
BOOL CSWindow::MoveWindow(int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
{
    ASSERT(m_hWnd);
    return ::MoveWindow(m_hWnd, X, Y, nWidth, nHeight, bRepaint);
}

BOOL CSWindow::SetForegroundWindow()
{
    ASSERT(m_hWnd);
    return ::SetForegroundWindow(m_hWnd);
}

HWND CSWindow::SetFocus()
{
    ASSERT(m_hWnd);
    return ::SetFocus(m_hWnd);
}

BOOL CSWindow::ShowWindow(int nCmdShow)
{
    // SW_SHOW
    ASSERT(m_hWnd);
    return ::ShowWindow(m_hWnd, nCmdShow);
}

// Standard windows props.
int CSWindow::GetWindowText(LPTSTR lpszText, int iLen)
{
    ASSERT(m_hWnd);
    return ::GetWindowText(m_hWnd, lpszText, iLen);
}

BOOL CSWindow::SetWindowText(LPCTSTR lpszText)
{
    ASSERT(m_hWnd);
    return ::SetWindowText(m_hWnd, lpszText);
}

void CSWindow::SetFont(HFONT hFont, BOOL fRedraw)
{
    SendMessage(WM_SETFONT, (WPARAM)hFont, MAKELPARAM(fRedraw, 0));
}

HICON CSWindow::SetIcon(HICON hIcon, BOOL fType)
{
    // ICON_BIG vs ICON_SMALL
    return (HICON)SendMessage(WM_SETICON, (WPARAM)fType, (LPARAM)hIcon);
}

UINT_PTR CSWindow::SetTimer(UINT_PTR uTimerID, UINT uWaitmSec)
{
    ASSERT(m_hWnd);
    return ::SetTimer(m_hWnd, uTimerID, uWaitmSec, nullptr);
}

BOOL CSWindow::KillTimer(UINT_PTR uTimerID)
{
    ASSERT(m_hWnd);
    return ::KillTimer(m_hWnd, uTimerID);
}

int CSWindow::MessageBox(lpctstr lpszText, lpctstr lpszTitle, UINT fuStyle) const
{
    // ASSERT( m_hWnd ); ok for this to be nullptr !
    return ::MessageBox(m_hWnd, lpszText, lpszTitle, fuStyle);
}

INT_PTR CSWindow::SetWindowLongPtr(int nIndex, INT_PTR dwNewLong)
{
    ASSERT(m_hWnd);
    return ::SetWindowLongPtr(m_hWnd, nIndex, dwNewLong);
}

INT_PTR CSWindow::GetWindowLongPtr(int nIndex) const
{
    ASSERT(m_hWnd);
    return ::GetWindowLongPtr(m_hWnd, nIndex);
}

int CSWindow::SetDlgItemText(int ID, lpctstr lpszText) const
{
    return ::SetDlgItemText(m_hWnd, ID, lpszText);
}


/* CDialogBase */

INT_PTR CALLBACK CDialogBase::DialogProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) // static
{
    CDialogBase * pDlg;
    if ( message == WM_INITDIALOG )
    {
        pDlg = reinterpret_cast<CDialogBase *>(lParam);
        ASSERT( pDlg );
        pDlg->m_hWnd = hWnd;	// OnCreate()
        pDlg->SetWindowLongPtr( GWLP_USERDATA, reinterpret_cast<INT_PTR>(pDlg) );
    }
    else
    {
        pDlg = static_cast<CDialogBase *>((LPVOID)::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }
    if ( pDlg )
    {
        return pDlg->DefDialogProc( message, wParam, lParam );
    }
    else
    {
        return FALSE;
    }
}

BOOL CDialogBase::DefDialogProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    UnreferencedParameter(message);
    UnreferencedParameter(wParam);
    UnreferencedParameter(lParam);
    return FALSE;
}


/* CSWindowBase */

ATOM CSWindowBase::RegisterClass( WNDCLASS & wc )	// static
{
    wc.lpfnWndProc = WndProc;
    return ::RegisterClass( &wc );
}

LRESULT WINAPI CSWindowBase::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) // static
{
    // NOTE: It is important to call OnDestroy() for asserts to work.
    CSWindowBase * pWnd;
    if ( message == WM_NCCREATE || message == WM_CREATE )
    {
        LPCREATESTRUCT lpCreateStruct = (LPCREATESTRUCT)lParam;
        ASSERT(lpCreateStruct);
        pWnd = static_cast<CSWindowBase *>(lpCreateStruct->lpCreateParams);
        ASSERT( pWnd );
        pWnd->m_hWnd = hWnd;	// OnCreate()
        pWnd->SetWindowLongPtr(GWLP_USERDATA, reinterpret_cast<INT_PTR>(pWnd));
    }
    pWnd = static_cast<CSWindowBase *>((LPVOID)::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    return ( pWnd ? pWnd->DefWindowProc(message, wParam, lParam) : ::DefWindowProc(hWnd, message, wParam, lParam) );
}


/* CWinApp */

CWinApp::CWinApp()
{
    m_pszAppName = "";
    m_hInstance = nullptr;
    m_lpCmdLine = nullptr;
    m_pMainWnd = nullptr;
}

void CWinApp::InitInstance(LPCTSTR pszAppName, HINSTANCE hInstance, LPTSTR lpszCmdLine)
{
    m_pszAppName = pszAppName;	// assume this is a static data pointer valid forever.
    m_hInstance	= hInstance;
    m_lpCmdLine	= lpszCmdLine;

    char szFileName[SPHERE_MAX_PATH];
    if (! GetModuleFileName(m_hInstance, szFileName, sizeof(szFileName)))
        return;
    m_pszExeName = szFileName;

    LPTSTR pszTmp = const_cast<LPTSTR>(strrchr(m_pszExeName, '\\'));	// Get title
    lstrcpy(szFileName, (pszTmp == nullptr) ? m_pszExeName : (pszTmp + 1));
    pszTmp = strrchr(szFileName, '.');	// Get extension.
    if (pszTmp != nullptr)
        pszTmp[0] = '\0';
    lstrcat(szFileName, ".INI");

    OFSTRUCT ofs = { };
    if (OpenFile(szFileName, &ofs, OF_EXIST) != HFILE_ERROR)
    {
        m_pszProfileName = ofs.szPathName;
    }
    else
    {
        m_pszProfileName = szFileName;
    }
}

HICON CWinApp::LoadIcon(int id) const
{
    return ::LoadIcon(m_hInstance, MAKEINTRESOURCE(id));
}
HMENU CWinApp::LoadMenu(int id) const
{
    return ::LoadMenu(m_hInstance, MAKEINTRESOURCE(id));
}


/* CScrollBar */

// Attributes
void CScrollBar::GetScrollRange(LPINT lpMinPos, LPINT lpMaxPos) const
{
    ASSERT(IsWindow());
    ::GetScrollRange(m_hWnd, SB_CTL, lpMinPos, lpMaxPos);
}
BOOL CScrollBar::GetScrollInfo(LPSCROLLINFO lpScrollInfo, UINT nMask)
{
    lpScrollInfo->cbSize = sizeof(*lpScrollInfo);
    lpScrollInfo->fMask = nMask;
    return ::GetScrollInfo(m_hWnd, SB_CTL, lpScrollInfo);
}


/* CEdit */

// Operations

void CEdit::SetSel(DWORD dwSelection, BOOL bNoScroll)
{
    UnreferencedParameter(bNoScroll);
    ASSERT(IsWindow());
    SendMessage(EM_SETSEL, (WPARAM)dwSelection, (LPARAM)dwSelection);
}
void CEdit::SetSel(size_t nStartChar, size_t nEndChar, BOOL bNoScroll)
{
    UnreferencedParameter(bNoScroll);
    ASSERT(IsWindow());
    SendMessage(EM_SETSEL, (WPARAM)nStartChar, (LPARAM)nEndChar);
}
size_t CEdit::GetSel() const
{
    ASSERT(IsWindow());
    return (size_t)(SendMessage(EM_GETSEL));
}
void CEdit::GetSel(size_t& nStartChar, size_t& nEndChar) const
{
    ASSERT(IsWindow());
    size_t nSelection = GetSel();
    nStartChar = LOWORD(nSelection);
    nEndChar = HIWORD(nSelection);
}

void CEdit::ReplaceSel(lpctstr lpszNewText, BOOL bCanUndo)
{
    ASSERT(IsWindow());
    SendMessage(EM_REPLACESEL, (WPARAM)bCanUndo, (LPARAM)lpszNewText);
}


/* CRichEditCtrl */

COLORREF CRichEditCtrl::SetBackgroundColor(BOOL bSysColor, COLORREF cr)
{
    return ((COLORREF)(DWORD)SendMessage(EM_SETBKGNDCOLOR, (WPARAM)bSysColor, (LPARAM)cr));
}

void CRichEditCtrl::SetSel(int nStartChar, int nEndChar)
{
    ASSERT(IsWindow());
    CHARRANGE range;
    range.cpMin = nStartChar;
    range.cpMax = nEndChar;
    SendMessage(EM_EXSETSEL, 0, (LPARAM)&range);
}
void CRichEditCtrl::GetSel(int& nStartChar, int& nEndChar) const
{
    ASSERT(IsWindow());
    CHARRANGE range;
    SendMessage(EM_EXGETSEL, 0, (LPARAM)&range);
    nStartChar = range.cpMin;
    nEndChar = range.cpMax;
}

void CRichEditCtrl::SetRedraw(BOOL val)
{
    ASSERT(IsWindow());
    SendMessage(WM_SETREDRAW, (WPARAM)val, 0);
}

void CRichEditCtrl::SetCaretHide(BOOL val)
{
    ASSERT(IsWindow());
    if (val)
        HideCaret(m_hWnd);
    else
        ShowCaret(m_hWnd);
}

DWORD CRichEditCtrl::ScrollLine()
{
    return (DWORD)PostMessage(EM_SCROLL, (WPARAM)SB_LINEDOWN);
}

DWORD CRichEditCtrl::ScrollPageDown()
{
    return (DWORD)PostMessage(EM_SCROLL, (WPARAM)SB_PAGEDOWN);
}

DWORD CRichEditCtrl::ScrollBottomRight()
{
    return (DWORD)PostMessage(WM_VSCROLL, (WPARAM)SB_BOTTOM);
}

// Formatting.
BOOL CRichEditCtrl::SetDefaultCharFormat(CHARFORMAT& cf)
{
    return (BOOL)(DWORD)SendMessage(EM_SETCHARFORMAT, (WPARAM)SCF_DEFAULT, (LPARAM)&cf);
}

BOOL CRichEditCtrl::SetSelectionCharFormat(CHARFORMAT& cf)
{
    return (BOOL)(DWORD)SendMessage(EM_SETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf);
}

// Events.
int CRichEditCtrl::GetEventMask() const
{
    return (DWORD)SendMessage(EM_GETEVENTMASK);
}

DWORD CRichEditCtrl::SetEventMask(DWORD dwEventMask)
{
    // ENM_NONE = default.
    return (DWORD)SendMessage(EM_SETEVENTMASK, 0, (LPARAM)dwEventMask);
}


/* CListbox */

void CListbox::ResetContent()
{
    ASSERT(IsWindow());
    SendMessage(LB_RESETCONTENT);
}
int CListbox::GetCount() const
{
    return (int)(DWORD)SendMessage(LB_GETCOUNT);
}
int CListbox::AddString(LPCTSTR lpsz) const
{
    return (int)(DWORD)SendMessage(LB_ADDSTRING, 0L, (LPARAM)(lpsz));
}


#endif	// _WIN32
