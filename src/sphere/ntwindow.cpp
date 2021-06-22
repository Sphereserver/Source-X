// Put up a window for data (other than the console)
#ifdef _WIN32

#include "../common/sphere_library/CSWindow.h"
#include "../common/sphere_library/CSString.h"
#include "../common/CException.h"
#include "../common/CTextConsole.h"
#include "../common/CLog.h"
#include "../common/sphereversion.h"	// sphere version
#include "../game/CObjBase.h"
#include "../game/CServerConfig.h"
#include "../game/CServer.h"
#include "../game/spheresvr.h"
#include "ProfileTask.h"
#include "resource.h"
#include "ntwindow.h"
#include <commctrl.h>	// NM_RCLICK

#define WM_USER_POST_MSG		(WM_USER+10)
#define WM_USER_TRAY_NOTIFY		(WM_USER+12)
#define IDC_M_LOG	10
#define IDC_M_INPUT 11
#define IDT_ONTICK	1


CNTApp theApp;

//************************************
// -CAboutDlg

bool CNTWindow::CAboutDlg::OnInitDialog()
{
	char *z = Str_GetTemp();
	sprintf(z, "%s %s", SPHERE_TITLE, SPHERE_VERSION);
	#ifdef __GITREVISION__
	 sprintf(z, "%s (build %d / GIT hash %s)", z, __GITREVISION__, __GITHASH__);
	#endif
	SetDlgItemText(IDC_ABOUT_VERSION, z);

	sprintf(z, "Compiled at %s (%s)", __DATE__, __TIME__);
	SetDlgItemText(IDC_ABOUT_COMPILER, z);
	return false;
}

bool CNTWindow::CAboutDlg::OnCommand( WORD wNotifyCode, INT_PTR wID, HWND hwndCtl )
{
	UNREFERENCED_PARAMETER(wNotifyCode);
	UNREFERENCED_PARAMETER(hwndCtl);

	// WM_COMMAND
	switch ( wID )
	{
		case IDOK:
		case IDCANCEL:
			EndDialog( m_hWnd, wID );
			break;
	}
	return true;
}

BOOL CNTWindow::CAboutDlg::DefDialogProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message )
	{
	case WM_INITDIALOG:
		return( OnInitDialog());
	case WM_COMMAND:
		return( OnCommand(  HIWORD(wParam), LOWORD(wParam), (HWND) lParam ));
	case WM_DESTROY:
		OnDestroy();
		return true;
	}
	return false;
}

//************************************
// -CStatusWnd

void CNTWindow::CStatusDlg::FillClients()
{
	if ( m_wndListClients.m_hWnd == nullptr )
		return;
	m_wndListClients.ResetContent();
	CNTWindow::CListTextConsole capture( m_wndListClients.m_hWnd );
	g_Serv.ListClients( &capture );
	int iCount = m_wndListClients.GetCount();
	++iCount;
}

void CNTWindow::CStatusDlg::FillStats()
{
	if ( m_wndListStats.m_hWnd == nullptr )
		return;

	m_wndListStats.ResetContent();

	CNTWindow::CListTextConsole capture( m_wndListStats.m_hWnd );

	size_t iThreadCount = ThreadHolder::getActiveThreads();
	for ( size_t iThreads = 0; iThreads < iThreadCount; ++iThreads)
	{
		IThread* thrCurrent = ThreadHolder::getThreadAt(iThreads);
		if (thrCurrent == nullptr)
			continue;

		const ProfileData& profile = static_cast<AbstractSphereThread*>(thrCurrent)->m_profile;
		if (profile.IsEnabled() == false)
			continue;

		capture.SysMessagef("Thread %lu - '%s'\n", thrCurrent->getId(), thrCurrent->getName());

		for (int i = 0; i < PROFILE_QTY; ++i)
		{
			if (profile.IsEnabled( (PROFILE_TYPE)i ) == false)
				continue;

			capture.SysMessagef("'%-14s' = %s\n", profile.GetName((PROFILE_TYPE)i), profile.GetDescription((PROFILE_TYPE)i));
		}
	}
}

bool CNTWindow::CStatusDlg::OnInitDialog()
{
	m_wndListClients.m_hWnd = GetDlgItem(IDC_STAT_CLIENTS);
	FillClients();
	m_wndListStats.m_hWnd = GetDlgItem(IDC_STAT_STATS);
	FillStats();
	return false;
}

bool CNTWindow::CStatusDlg::OnCommand( word wNotifyCode, INT_PTR wID, HWND hwndCtl )
{
	UNREFERENCED_PARAMETER(wNotifyCode);
	UNREFERENCED_PARAMETER(hwndCtl);

	// WM_COMMAND
	switch ( wID )
	{
		case IDOK:
		case IDCANCEL:
			DestroyWindow();
			break;
	}
	return false;
}

BOOL CNTWindow::CStatusDlg::DefDialogProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	// IDM_STATUS
	switch ( message )
	{
	case WM_INITDIALOG:
		return OnInitDialog();
	case WM_COMMAND:
		return OnCommand( HIWORD(wParam), LOWORD(wParam), (HWND) lParam );
	case WM_DESTROY:
		m_wndListClients.OnDestroy();
		m_wndListStats.OnDestroy();
		OnDestroy();
		return true;
	}
	return false;
}

CNTWindow::CNTWindow() : AbstractSphereThread("T_ConsoleWindow", IThread::Highest),
    _NTWInitParams{}, m_zCommands {{}}
{
	m_iLogTextLen		= 0;
	m_fLogScrollLock	= false;
	m_dwColorNew		= RGB( 0xaf,0xaf,0xaf );
	m_dwColorPrv		= RGB( 0xaf,0xaf,0xaf );
	m_iHeightInput		= 0;
   	m_hLogFont			= nullptr;

    _fNewWindowTitle = false;
}

CNTWindow::~CNTWindow()
{
    DestroyWindow();
    exitActions();
}

void CNTWindow::exitActions()
{
    g_Serv.SetExitFlag(5);
    NTWindow_DeleteIcon();
    _thread_selfTerminateAfterThisTick = true;
}

void CNTWindow::onStart()
{
    AbstractThread::onStart();
    NTWindow_Init(_NTWInitParams.hInstance, _NTWInitParams.lpCmdLine, _NTWInitParams.nCmdShow);
}

void CNTWindow::terminate(bool ended)
{
    AbstractSphereThread::terminate(ended);
}

bool CNTWindow::shouldExit()
{
    return AbstractThread::shouldExit();
}

void CNTWindow::tick()
{
    if (!_qOutput.empty())
    {
		std::deque<std::unique_ptr<ConsoleOutput>> outMessages;
		{
			// No idea of the reason, but it seems that if we have some mutex locked while doing List_Add, sometimes we'll have a deadlock.
			//  In any case, it's best to keep the mutex locked for the least time possible.
			std::unique_lock<std::mutex> lock(this->ConsoleInterface::_ciQueueMutex);
			outMessages.swap(this->ConsoleInterface::_qOutput);
		}

		theApp.m_wndMain.List_AddGroup(std::move(outMessages));
    }

    NTWindow_CheckUpdateWindowTitle();

    if (!NTWindow_OnTick(0))
    {
        exitActions();
    }
}

bool CNTWindow::NTWindow_CanScroll()
{
    // If the select is on screen then keep scrolling.
    if ( ! m_fLogScrollLock /*&& (GetActiveWindow() == m_hWnd)*/ && ! GetCapture() )
    {
        if ( Sphere_GetOSInfo()->dwPlatformId == VER_PLATFORM_WIN32_NT )
        {
            return true;
        }
    }
    return false;
}

void CNTWindow::List_Clear()
{
	m_wndLog.SetWindowText( "");
	m_wndLog.SetSel( 0, 0 );
	m_iLogTextLen = 0;
}

void CNTWindow::List_AddSingle(COLORREF color, LPCTSTR ptcText)
{
	const int iMaxTextLen = (64 * 1024);

	const int iTextLen = (int)strlen(ptcText);
	const int iNewLen = m_iLogTextLen + iTextLen;

	if ( iNewLen > iMaxTextLen )
	{
		const int iCut = iNewLen - iMaxTextLen; 

		m_wndLog.SetSel( 0, iCut );

		// These SetRedraw FALSE/TRUE calls will make the log panel scroll much faster when spamming text, but
		//  it will generate some drawing artifact
		//m_wndLog.SetRedraw(FALSE);
		m_wndLog.ReplaceSel( "" );	
	}
    else if (NTWindow_CanScroll())
        theApp.m_wndMain.m_wndLog.ScrollLine();

	m_wndLog.SetSel(m_iLogTextLen, m_iLogTextLen + iTextLen);

	// set the blocks color.
	CHARFORMAT cf{};
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = color;
	m_wndLog.SetSelectionCharFormat( cf );

    //m_wndLog.SetRedraw(TRUE);
	m_wndLog.ReplaceSel(ptcText);

	m_iLogTextLen += iTextLen;
	m_wndLog.SetSel( m_iLogTextLen, m_iLogTextLen );

	int iSelBegin;
	int iSelEnd;
	m_wndLog.GetSel( iSelBegin, iSelEnd );
	m_iLogTextLen = iSelBegin;	// make sure it's correct.
}

void CNTWindow::List_AddGroup(std::deque<std::unique_ptr<ConsoleOutput>>&& msgs)
{
	const int iMaxTextLen = (64 * 1024);

	// Erase the old text to make space for all the message queue at once
	int iTotalTextLen = 0;
	for (std::unique_ptr<ConsoleOutput> const& co : msgs)
	{
		iTotalTextLen += co->GetTextString().GetLength();
	}
	
	const int iNewLen = m_iLogTextLen + iTotalTextLen;

	if (iNewLen > iMaxTextLen)
	{
		int iCut = iNewLen - iMaxTextLen;
		iCut = minimum(iCut, iMaxTextLen);

		m_wndLog.SetSel(0, iCut);
		m_wndLog.ReplaceSel("");
	}

	// Append all the messages at once
	for (std::unique_ptr<ConsoleOutput> const& co : msgs)
	{
		const COLORREF color = (COLORREF)CTColToRGB(co->GetTextColor());
		const lpctstr ptcText = co->GetTextString().GetBuffer();
		const int iTextLen = co->GetTextString().GetLength();

		m_wndLog.SetSel(m_iLogTextLen, m_iLogTextLen + iTextLen);

		// set the blocks color.
		CHARFORMAT cf{};
		cf.cbSize = sizeof(cf);
		cf.dwMask = CFM_COLOR;
		cf.crTextColor = color;
		m_wndLog.SetSelectionCharFormat(cf);

		m_wndLog.ReplaceSel(ptcText);

		m_iLogTextLen += iTextLen;
		m_wndLog.SetSel(m_iLogTextLen, m_iLogTextLen);

		int iSelBegin;
		int iSelEnd;
		m_wndLog.GetSel(iSelBegin, iSelEnd);
		m_iLogTextLen = iSelBegin;	// make sure it's correct.
	}

	// Scroll just one time, after all the new messages are added
	if (NTWindow_CanScroll())
		theApp.m_wndMain.m_wndLog.ScrollBottomRight();
}

void CNTWindow::SetWindowTitle(LPCTSTR pText)
{
    std::unique_lock<std::shared_mutex> lock(_mutexWindowTitle);
    if (pText)
        _strWindowTitle = pText;
    else
        _strWindowTitle.clear();
    _fNewWindowTitle = true;
}

bool CNTWindow::RegisterClass(char *className)	// static
{
	WNDCLASS wc;
	memset( &wc, 0, sizeof(wc));

	wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = theApp.m_hInstance;
	wc.hIcon = theApp.LoadIcon( IDR_MAINFRAME );
	wc.hCursor = LoadCursor( nullptr, IDC_ARROW );
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = className;

	ATOM frc = ::RegisterClass( &wc );
	if ( !frc )
	{
		return false;
	}

    LoadLibrary("Riched20.dll"); // Load the RichEdit DLL to activate the class
	return true;
}

int CNTWindow::OnCreate( HWND hWnd, LPCREATESTRUCT lParam )
{
	UNREFERENCED_PARAMETER(lParam);
	CSWindow::OnCreate(hWnd);

	m_wndLog.m_hWnd = ::CreateWindow( RICHEDIT_CLASS, nullptr,
		ES_LEFT | ES_MULTILINE | ES_READONLY | /* ES_OEMCONVERT | */
		WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		0, 0, 10, 10,
		m_hWnd,
		(HMENU)(UINT) IDC_M_LOG, theApp.m_hInstance, nullptr );
	ASSERT( m_wndLog.m_hWnd );
	m_wndLog.SetSel(0, 0);


	SetLogFont( "Courier" );

	// TEXTMODE
	m_wndLog.SetBackgroundColor( false, RGB(0,0,0) );
	CHARFORMAT cf;
	memset( &cf, 0, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = m_dwColorPrv;
	cf.bCharSet = ANSI_CHARSET;
	cf.bPitchAndFamily = FF_MODERN | FIXED_PITCH;
	m_wndLog.SetDefaultCharFormat( cf );
	m_wndLog.SetEventMask( ENM_LINK | ENM_MOUSEEVENTS | ENM_KEYEVENTS );

	m_wndInput.m_hWnd = ::CreateWindow("EDIT", nullptr,
		ES_LEFT | ES_AUTOHSCROLL | WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP,
		0, 0, 10, 10,
		m_hWnd,
		(HMENU)(UINT) IDC_M_INPUT, theApp.m_hInstance, nullptr );
	ASSERT( m_wndInput.m_hWnd );

	if ( Sphere_GetOSInfo()->dwPlatformId > VER_PLATFORM_WIN32s )
	{
		memset(&m_pnid,0,sizeof(m_pnid));
        m_pnid.cbSize = sizeof(NOTIFYICONDATA);
        m_pnid.hWnd   = m_hWnd;
        m_pnid.uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE;
        m_pnid.uCallbackMessage = WM_USER_TRAY_NOTIFY;
        m_pnid.hIcon  = theApp.LoadIcon( IDR_MAINFRAME );
        Str_CopyLimitNull(m_pnid.szTip, theApp.m_pszAppName, CountOf(m_pnid.szTip)-1);
		Shell_NotifyIcon(NIM_ADD, &m_pnid);
	}

	SetWindowTitle();

	return 0;
}

void CNTWindow::OnDestroy()
{
	m_wndLog.OnDestroy();	// these are automatic.
	m_wndInput.OnDestroy();
	CSWindow::OnDestroy();
    exitActions();
}

void CNTWindow::OnSetFocus( HWND hWndLoss )
{
	UNREFERENCED_PARAMETER(hWndLoss);
	m_wndInput.SetFocus();
}

LRESULT CNTWindow::OnUserTrayNotify( WPARAM wID, LPARAM lEvent )
{
	UNREFERENCED_PARAMETER(wID);

	// WM_USER_TRAY_NOTIFY
	switch ( lEvent )
	{
	case WM_RBUTTONDOWN:
		// Context menu ?
		{
			HMENU hMenu = theApp.LoadMenu( IDM_POP_TRAY );
			if ( hMenu == nullptr )
				break;
			HMENU hMenuPop = GetSubMenu(hMenu,0);
			if ( hMenuPop )
			{
				POINT point;
				if ( GetCursorPos( &point ))
				{
					TrackPopupMenu( hMenuPop, TPM_RIGHTBUTTON, point.x, point.y, 0, m_hWnd, nullptr );
				}
			}
			DestroyMenu( hMenu );
		}
		return 1;
	case WM_LBUTTONDBLCLK:
		{
			if ( IsWindowVisible(m_hWnd) )
				ShowWindow(SW_HIDE);
			else
			{
				ShowWindow(SW_NORMAL);
				SetForegroundWindow();
			}
			return 1;
		}
	}
	return 0;	// not handled.
}

void CNTWindow::OnUserPostMessage( COLORREF color, CSString * psMsg )
{
	// WM_USER_POST_MSG
	if ( psMsg )
	{
		List_AddSingle(color, *psMsg);
		delete psMsg;

        // Scroll down a line each time we print a message
        if (NTWindow_CanScroll())
            theApp.m_wndMain.m_wndLog.ScrollLine();
	}
}

void CNTWindow::OnSize( WPARAM nType, int cx, int cy )
{
	if ( nType != SIZE_MINIMIZED && nType != SIZE_MAXHIDE && m_wndLog.m_hWnd )
	{
		if ( ! m_iHeightInput )
		{
			HFONT hFont = (HFONT)SendMessage(WM_GETFONT);
			if ( !hFont )
				hFont = (HFONT)GetStockObject(SYSTEM_FONT);
			ASSERT(hFont);

			LOGFONT logfont;
			int iRet = ::GetObject(hFont, sizeof(logfont),&logfont );
			ASSERT(iRet==sizeof(logfont));
			UNREFERENCED_PARAMETER(iRet);

			m_iHeightInput = abs( logfont.lfHeight );
			ASSERT(m_iHeightInput);
		}

		m_wndLog.MoveWindow( 0, 0, cx, cy-m_iHeightInput, TRUE );
		m_wndInput.MoveWindow( 0, cy-m_iHeightInput, cx, m_iHeightInput, TRUE );
	}
}

bool CNTWindow::OnClose()
{
	// WM_CLOSE
	if ( g_Serv.GetExitFlag() == 0 )
	{
		int iRet = theApp.m_wndMain.MessageBox("Are you sure you want to close the server?",
			theApp.m_pszAppName, MB_YESNO|MB_ICONQUESTION );
		if ( iRet == IDNO )
			return false;
	}

	PostQuitMessage(0); // posts WM_QUIT
	return true;	// ok to close.
}

bool CNTWindow::OnCommand( WORD wNotifyCode, INT_PTR wID, HWND hwndCtl )
{
	// WM_COMMAND
	UNREFERENCED_PARAMETER(wNotifyCode);
	UNREFERENCED_PARAMETER(hwndCtl);

	switch ( wID )
	{
	case IDC_M_LOG:
		break;
	case IDM_STATUS:
		if ( theApp.m_wndStatus.m_hWnd == nullptr )
		{
			theApp.m_wndStatus.m_hWnd = ::CreateDialogParam(
				theApp.m_hInstance,
				MAKEINTRESOURCE(IDM_STATUS),
				HWND_DESKTOP,
				CDialogBase::DialogProc,
				reinterpret_cast<LPARAM>(static_cast <CDialogBase*>(&theApp.m_wndStatus)) );
		}
		theApp.m_wndStatus.ShowWindow(SW_NORMAL);
		theApp.m_wndStatus.SetForegroundWindow();
		break;
	case IDR_ABOUT_BOX:
        if ( theApp.m_wndAbout.m_hWnd == nullptr )
        {
            theApp.m_wndAbout.m_hWnd = ::CreateDialogParam(
                theApp.m_hInstance,
                MAKEINTRESOURCE(IDR_ABOUT_BOX),
                HWND_DESKTOP,
                CDialogBase::DialogProc,
                reinterpret_cast<LPARAM>(static_cast <CDialogBase*>(&theApp.m_wndAbout)) );
        }
        theApp.m_wndAbout.ShowWindow(SW_NORMAL);
        theApp.m_wndAbout.SetForegroundWindow();
        break;
	case IDM_MINIMIZE:
		// SC_MINIMIZE
	    ShowWindow(SW_HIDE);
		break;
	case IDM_RESTORE:
		// SC_RESTORE
	    ShowWindow(SW_NORMAL);
		SetForegroundWindow();
		break;
	case IDM_EXIT:
		PostMessage( WM_CLOSE );
		break;

	case IDM_RESYNC_PAUSE:
		if ( ! g_Serv.m_fConsoleTextReadyFlag )	// busy ?
		{
			g_Serv.m_sConsoleText = "R";
			g_Serv.m_fConsoleTextReadyFlag = true;
			return true;
		}
		return false;

	case IDM_EDIT_COPY:
		m_wndLog.SendMessage( WM_COPY );
		break;

	case IDOK:
		// We just entered the text.

		if ( ! g_Serv.m_fConsoleTextReadyFlag )	// busy ?
		{
			TCHAR szTmp[ MAX_TALK_BUFFER ];
			m_wndInput.GetWindowText( szTmp, sizeof(szTmp));
			strcpy(m_zCommands[4], m_zCommands[3]);
			strcpy(m_zCommands[3], m_zCommands[2]);
			strcpy(m_zCommands[2], m_zCommands[1]);
			strcpy(m_zCommands[1], m_zCommands[0]);
			strcpy(m_zCommands[0], szTmp);
			m_wndInput.SetWindowText("");
			g_Serv.m_sConsoleText = szTmp;
			g_Serv.m_fConsoleTextReadyFlag = true;
			return true;
		}
		return false;
	}
	return true;
}

bool CNTWindow::OnSysCommand( WPARAM uCmdType, int xPos, int yPos )
{
	// WM_SYSCOMMAND
	// return : 1 = i processed this.
	UNREFERENCED_PARAMETER(xPos);
	UNREFERENCED_PARAMETER(yPos);

	switch ( uCmdType )
	{
		case SC_MINIMIZE:
			if ( Sphere_GetOSInfo()->dwPlatformId > VER_PLATFORM_WIN32s )
			{
				ShowWindow(SW_HIDE);
				return true;
			}
			break;
	}
	return false;
}

void CNTWindow::SetLogFont( const char * pszFont )
{
	// use an even spaced font
	if ( pszFont == nullptr )
	{
		m_hLogFont	= (HFONT) GetStockObject(SYSTEM_FONT);
	}
	else
	{
		LOGFONT logfont;
   		memset( &logfont, 0, sizeof(logfont) );
   		strcpy( logfont.lfFaceName, pszFont );

		// calculate height for a 10pt font, some systems can produce an unreadable
		// font size if we let CreateFontIndirect pick a system default size
		HDC hdc = GetDC(nullptr);
		if (hdc != nullptr)
		{
			logfont.lfHeight = IMulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
			ReleaseDC(nullptr, hdc);
		}

		logfont.lfPitchAndFamily = FF_MODERN;
   		m_hLogFont = CreateFontIndirect( &logfont );
	}
   	m_wndLog.SetFont( m_hLogFont, true );
}


LRESULT CNTWindow::OnNotify( int idCtrl, NMHDR * pnmh )
{
	ASSERT(pnmh);
	if ( idCtrl != IDC_M_LOG )
		return 0;

	switch ( pnmh->code )
	{
	case EN_LINK:
		{
			ENLINK * pLink = (ENLINK *)(pnmh);
			if ( pLink->msg == WM_LBUTTONDOWN )
				return 1;
			break;
		}
	case EN_MSGFILTER:
		{
			MSGFILTER	*pMsg = (MSGFILTER *)pnmh;
			ASSERT(pMsg);

			switch ( pMsg->msg )
			{
			case WM_MOUSEMOVE:
				return 0;
			case WM_RBUTTONDOWN:
				{
					HMENU hMenu = theApp.LoadMenu( IDM_POP_LOG );
					if ( !hMenu )
						return 0;
					HMENU hMenuPop = GetSubMenu(hMenu,0);
					if ( hMenuPop )
					{
						POINT point;
						if ( GetCursorPos( &point ))
							TrackPopupMenu( hMenuPop, TPM_RIGHTBUTTON, point.x, point.y, 0, m_hWnd, nullptr );
					}
					DestroyMenu(hMenu);
					return 1;
				}
			case WM_LBUTTONDBLCLK:
				{
					TCHAR * zTemp = Str_GetTemp();
					POINT pt;
					pt.x = LOWORD(pMsg->lParam);
					pt.y = HIWORD(pMsg->lParam);

					// get selected line
					LRESULT line = m_wndLog.SendMessage(EM_LINEFROMCHAR, m_wndLog.SendMessage(EM_CHARFROMPOS, 0, reinterpret_cast<LPARAM>(&pt)), 0);

					// get the line text
					reinterpret_cast<word*>(zTemp)[0] = SCRIPT_MAX_LINE_LEN - 1; // first word is used to indicate the max buffer length
					zTemp[m_wndLog.SendMessage(EM_GETLINE, line, reinterpret_cast<LPARAM>(zTemp))] = '\0';
					if ( *zTemp == '\0' )
						break;

					//	use dclick to open the corresponding script file
					TCHAR * pos = strstr(zTemp, SPHERE_SCRIPT);
					if ( pos != nullptr )
					{
						//	use two formats of file names:
						//		Loading filepath/filename/name.scp
						//		ERROR:(filename.scp,line)
						LPCTSTR start = pos;
						TCHAR * end = pos + 4;

						while ( start > zTemp )
						{
							if (( *start == ' ' ) || ( *start == '(' ))
								break;
							--start;
						}
						++start;
						*end = '\0';

						if ( *start != '\0' )
						{
							LPCTSTR filePath = nullptr;

							// search script files for a matching name
							size_t i = 0;
							for (const CResourceScript * s = g_Cfg.GetResourceFile(i++); s != nullptr; s = g_Cfg.GetResourceFile(i++))
							{
								if ( strstr(s->GetFilePath(), start) == nullptr )
									continue;

								filePath = s->GetFilePath();
								break;
							}

							// since certain files aren't listed, handle these separately
							if (filePath == nullptr)
							{
								if ( strstr(SPHERE_FILE "tables" SPHERE_SCRIPT, start) )
								{
									TCHAR * z = Str_GetTemp();
									strcpy(z, g_Cfg.m_sSCPBaseDir);
									strcat(z, start);
									filePath = z;
								}
							}

							if (filePath != nullptr)
							{
								// ShellExecute fails when a relative path is passed to it that uses forward slashes as a path
								// separator.. to workaround this we can use GetFullPathName (which accepts forward slashes) to
								// resolve the relative path to an absolute path
								TCHAR * z = Str_GetTemp();
								if (GetFullPathName(filePath, THREAD_STRING_LENGTH, z, nullptr) > 0)
								{
									INT_PTR r = reinterpret_cast<INT_PTR>(ShellExecute(nullptr, nullptr, z, nullptr, nullptr, SW_SHOW));
									if (r > 32)
										return 1;
								}

								// failure occurred
								int errorCode = CSFile::GetLastError();
								if (CSError::GetSystemErrorMessage(errorCode, z, THREAD_STRING_LENGTH) > 0)
									g_Log.Event(LOGL_WARN, "Failed to open '%s' code=%d (%s).\n", filePath, errorCode, z);
								else
									g_Log.Event(LOGL_WARN, "Failed to open '%s' code=%d.\n", filePath, errorCode);
							}
						}
					}
					break;
				}
			case WM_CHAR:
				{
					// We normally have no business typing into this window.
					// Should we allow CTL C etc ?
					if ( pMsg->lParam & (1<<29))	// ALT
						return 0;
					SHORT sState = GetKeyState( VK_CONTROL );
					if ( sState & 0xff00 )
						return 0;
					m_wndInput.SetFocus();
					m_wndInput.PostMessage( WM_CHAR, pMsg->wParam, pMsg->lParam );
					return 1;	// mostly ignored.
				}
			} // pMsg
		} // MSGFILTER
	} // code
	return 0;
}

LRESULT WINAPI CNTWindow::WindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )	// static
{
	try
	{
		switch( message )
		{
		case WM_CREATE:
			return( theApp.m_wndMain.OnCreate( hWnd, (LPCREATESTRUCT) lParam ));
		case WM_SYSCOMMAND:
			if ( theApp.m_wndMain.OnSysCommand( wParam &~ 0x0f, LOWORD(lParam), HIWORD(lParam)))
				return 0;
			break;
		case WM_COMMAND:
			if ( theApp.m_wndMain.OnCommand( HIWORD(wParam), LOWORD(wParam), (HWND) lParam ))
				return 0;
			break;
		case WM_CLOSE:
			if ( ! theApp.m_wndMain.OnClose())  // this method doesn't appear to be called (something is terminating this process in the meanwhile)
				return false;
			break;
		case WM_ERASEBKGND:	// don't bother with this.
			return 1;
		case WM_SIZE:	// get the client rectangle
			theApp.m_wndMain.OnSize( wParam, LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_DESTROY:
			theApp.m_wndMain.OnDestroy();
			return 0;
		case WM_SETFOCUS:
			theApp.m_wndMain.OnSetFocus( (HWND) wParam );
			return 0;
		case WM_NOTIFY:
			theApp.m_wndMain.OnNotify( (int) wParam, (NMHDR *) lParam );
			return 0;
		case WM_USER_POST_MSG:
			theApp.m_wndMain.OnUserPostMessage( (COLORREF) wParam, reinterpret_cast<CSString*>(lParam) );
			return 1;
		case WM_USER_TRAY_NOTIFY:
			return theApp.m_wndMain.OnUserTrayNotify( wParam, lParam );
		}
	}
	catch (const CSError& e)
	{
		g_Log.CatchEvent(&e, "Window");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent(nullptr, "Window");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	return ::DefWindowProc(hWnd, message, wParam, lParam);
}

//************************************

bool CNTWindow::NTWindow_Init(HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow)
{
#define SPHERE_WINDOW_TITLE_BASE     SPHERE_TITLE " " SPHERE_VERSION_PREFIX SPHERE_VERSION
	theApp.InitInstance(SPHERE_WINDOW_TITLE_BASE, hInstance, lpCmdLine);

	//	read target window name from the arguments
	char	className[32] = SPHERE_TITLE;
	TCHAR	*argv[32];
	argv[0] = nullptr;
	int argc = Str_ParseCmds(lpCmdLine, &argv[1], CountOf(argv)-1, " \t") + 1;
	if (( argc > 1 ) && _IS_SWITCH(*argv[1]) )
	{
		if ( toupper(argv[1][1]) == 'C' )
		{
			if ( argv[1][2] )
				strcpy(className, &argv[1][2]);
		}
	}
	CNTWindow::RegisterClass(className);

	theApp.m_wndMain.m_hWnd = ::CreateWindow(
		className,
        SPHERE_WINDOW_TITLE_BASE, // window name
		WS_OVERLAPPEDWINDOW,   // window style
		CW_USEDEFAULT,  // horizontal position of window
		CW_USEDEFAULT,  // vertical position of window
		CW_USEDEFAULT,  // window width
		CW_USEDEFAULT,	// window height
		HWND_DESKTOP,      // handle to parent or owner window
		nullptr,          // menu handle or child identifier
		theApp.m_hInstance,  // handle to application instance
		nullptr        // window-creation data
		);

	theApp.m_wndMain.ShowWindow(nCmdShow);
	return true;
}

void CNTWindow::NTWindow_DeleteIcon()
{
	if ( Sphere_GetOSInfo()->dwPlatformId > VER_PLATFORM_WIN32s )
	{
		theApp.m_wndMain.m_pnid.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &theApp.m_wndMain.m_pnid);
	}
}

void CNTWindow::NTWindow_ExitServer()
{
	// Unattach the window.
	int iExitFlag = g_Serv.GetExitFlag();
	if ( iExitFlag < 0 )
	{
		TCHAR *pszMsg = Str_GetTemp();
		sprintf(pszMsg, "Server terminated by error %d!", iExitFlag);
		theApp.m_wndMain.MessageBox(pszMsg, theApp.m_pszAppName, MB_OK|MB_ICONEXCLAMATION );
	}
}

void CNTWindow::NTWindow_CheckUpdateWindowTitle()
{
	if ( theApp.m_wndMain.m_hWnd == nullptr )
		return;

    _mutexWindowTitle.lock_shared();
    if (!_fNewWindowTitle)
    {
        _mutexWindowTitle.unlock_shared();
        return;
    }
    std::string strNewTitle = _strWindowTitle;
    _strWindowTitle.clear();
    _fNewWindowTitle = false;
    _mutexWindowTitle.unlock_shared();

	// set the title to reflect mode.

	LPCTSTR pszMode;
	switch ( g_Serv.GetServerMode() )
	{
	case SERVMODE_RestockAll:	// Major event.
		pszMode = "Restocking";
		break;
	case SERVMODE_GarbageCollection:	// Major event.
		pszMode = "Collecting Garbage";
		break;
	case SERVMODE_Saving:		// Forced save freezes the system.
		pszMode = "Saving";
		break;
	case SERVMODE_Run:			// Game is up and running
		pszMode = "Running";
		break;
    case SERVMODE_PreLoadingINI:
	case SERVMODE_Loading:		// Initial load.
		pszMode = "Loading";
		break;
	case SERVMODE_ResyncPause:
		pszMode = "Resync Pause";
		break;
	case SERVMODE_ResyncLoad:	// Loading after resync
		pszMode = "Resync Load";
		break;
	case SERVMODE_Exiting:		// Closing down
		pszMode = "Exiting";
		break;
	default:
		pszMode = "Unknown";
		break;
	}

	// Number of connections ?

	char *psTitle = Str_GetTemp();
	snprintf(psTitle, STR_TEMPLENGTH, "%s - %s (%s) %s", theApp.m_pszAppName, g_Serv.GetName(), pszMode, strNewTitle.c_str() );
	theApp.m_wndMain.SetWindowText( psTitle );

	if ( Sphere_GetOSInfo()->dwPlatformId > VER_PLATFORM_WIN32s )
	{
		theApp.m_wndMain.m_pnid.uFlags = NIF_TIP;
        Str_CopyLimitNull(theApp.m_wndMain.m_pnid.szTip, psTitle, CountOf(theApp.m_wndMain.m_pnid.szTip)-1);
		Shell_NotifyIcon(NIM_MODIFY, &theApp.m_wndMain.m_pnid);
	}
}

bool CNTWindow::NTWindow_OnTick( int iWaitmSec )
{
	// RETURN: false = exit the app.

    if ( iWaitmSec )
    {
        if ( !theApp.m_wndMain.m_hWnd || !theApp.m_wndMain.SetTimer(IDT_ONTICK, iWaitmSec) )
        {
            iWaitmSec = 0;
        }
    }
	

	// Give the windows message loops a tick.
	for (;;)
	{
		EXC_TRY("Tick");

		MSG msg;

		// any windows messages?
		if ( iWaitmSec )
		{
			if ( ! GetMessage( &msg, nullptr, 0, 0 )) // (blocks until a message arrives)
				return false;

			if ( (msg.hwnd == theApp.m_wndMain.m_hWnd) && (msg.message == WM_TIMER) && (msg.wParam == IDT_ONTICK) )
			{
				theApp.m_wndMain.KillTimer( IDT_ONTICK );
				iWaitmSec = 0;	// empty the queue and bail out.
				continue;
			}
		}
		else
		{
			if (! PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE )) // not blocking
				return true;

			if ( msg.message == WM_QUIT )
				return false;
		}

		//	Got char in edit box
		if ( theApp.m_wndMain.m_wndInput.m_hWnd && (msg.hwnd == theApp.m_wndMain.m_wndInput.m_hWnd) )
		{
			if ( msg.message == WM_CHAR )	//	char to edit box
			{
				if ( msg.wParam == '\r' )	//	ENTER
				{
					if ( theApp.m_wndMain.OnCommand( 0, IDOK, msg.hwnd ))
					{
						return true;
					}
				}
			}
			else if ( msg.message == WM_KEYUP )	//	key released
			{
				if ( msg.wParam == VK_UP )			//	UP (commands history)
				{
					theApp.m_wndMain.m_wndInput.SetWindowText(theApp.m_wndMain.m_zCommands[0]);
					strcpy(theApp.m_wndMain.m_zCommands[0], theApp.m_wndMain.m_zCommands[1]);
					strcpy(theApp.m_wndMain.m_zCommands[1], theApp.m_wndMain.m_zCommands[2]);
					strcpy(theApp.m_wndMain.m_zCommands[2], theApp.m_wndMain.m_zCommands[3]);
					strcpy(theApp.m_wndMain.m_zCommands[3], theApp.m_wndMain.m_zCommands[4]);
					theApp.m_wndMain.m_wndInput.GetWindowText(theApp.m_wndMain.m_zCommands[4], sizeof(theApp.m_wndMain.m_zCommands[4]));
				}
				else if ( msg.wParam == VK_TAB )	// TAB (auto-complete)
				{
					size_t selStart, selEnd;
					TCHAR *pszTemp = Str_GetTemp();
					CEdit *inp = &theApp.m_wndMain.m_wndInput;

					//	get current selection (to be replaced), suppose it being our "completed" word
					inp->GetSel(selStart, selEnd);
					inp->GetWindowText(pszTemp, SCRIPT_MAX_LINE_LEN);

					// the code will act incorrectly if using tab in the middle of the text
					// since we are just unable to get our current position. really unable?
					if ( selEnd == strlen(pszTemp) )		// so proceed only if working on last char
					{
						TCHAR * pszCurSel = Str_GetTemp();
						size_t inputLen = 0;

						// there IS a selection, so extract it
						if ( selStart != selEnd )
						{
                            size_t iSizeSel = selEnd - selStart + 1; // +1 for the terminator
                            if (iSizeSel > STR_TEMPLENGTH)
                                iSizeSel = STR_TEMPLENGTH;
							Str_CopyLimit(pszCurSel, pszTemp + selStart, iSizeSel);
							pszCurSel[iSizeSel] = '\0';
						}
						else
						{
							*pszCurSel = '\0';
						}

						// detect part of the text we are entered so far
						TCHAR *p = &pszTemp[strlen(pszTemp) - 1];
						while (( p >= pszTemp ) && ( *p != '.' ) && ( *p != ' ' ) && ( *p != '/' ) && ( *p != '=' ))
						{
							--p;
						}
						++p;

						// remove the selected part of the message
						pszTemp[selStart] = '\0';
						inputLen = strlen(p);

						// search in the auto-complete list for starting on P, and save coords of 1st and Last matched
						CSStringListRec	*firstmatch = nullptr;
						CSStringListRec	*lastmatch = nullptr;
						CSStringListRec	*curmatch = nullptr;	// the one that should be set

						for ( curmatch = g_AutoComplete.GetHead(); curmatch != nullptr; curmatch = curmatch->GetNext() )
						{
							if ( !strnicmp(curmatch->GetBuffer(), p, inputLen) )	// matched
							{
								if ( firstmatch == nullptr )
								{
									firstmatch = lastmatch = curmatch;
								}
								else
								{
									lastmatch = curmatch;
								}
							}
							else if ( lastmatch )
							{
								break;	// if no longer matches - save time by instant quit
							}
						}

						if ( firstmatch != nullptr )	// there IS a match
						{
							bool bOnly(false);
							if ( firstmatch == lastmatch )					// and the match is the ONLY
							{
								bOnly = true;
								curmatch = firstmatch;
							}
							else if ( !*pszCurSel )							// or there is still no selection
							{
								curmatch = firstmatch;	
							}
							else											// need to find for the next record
							{
								size_t curselLen = strlen(pszCurSel);
								for ( curmatch = firstmatch; curmatch != lastmatch->GetNext(); curmatch = curmatch->GetNext() )
								{
									// found the first next one
									if ( strnicmp(curmatch->GetBuffer() + inputLen, pszCurSel, curselLen) > 0 )
									{
										break;
									}
								}
								if ( curmatch == lastmatch->GetNext() )
								{
									curmatch = firstmatch; // scrolled over
								}
							}

							LPCTSTR	tmp = curmatch->GetBuffer() + inputLen;
							inp->ReplaceSel(tmp);
							if ( !bOnly )
							{
								inp->SetSel(selStart, selStart + strlen(tmp));
							}
						}
					}
					else
					{
						inp->SendMessage(WM_CHAR, ' ');	// in this case just replace selection by a space
					}
				}
			}
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);

		EXC_CATCH;
	}
}

#endif // _WIN32
