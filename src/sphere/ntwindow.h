/**
* @file ntwindow.h
*
*/

#ifndef _INC_NTWINDOW_H
#define _INC_NTWINDOW_H

#ifdef _WIN32

#include "threads.h"
#include "ConsoleInterface.h"
#include "../common/sphere_library/CSWindow.h"
#include "../common/CTextConsole.h"

extern struct CNTWindow : public AbstractSphereThread, public CSWindow, public ConsoleInterface
{
    static const char *m_sClassName;
    struct
    {
        HINSTANCE hInstance;
        LPTSTR lpCmdLine;
        int nCmdShow;
    } _NTWInitParams;

    virtual void onStart();
    virtual void terminate(bool ended);
    virtual bool shouldExit();
    virtual void tick();

    bool NTWindow_Init(HINSTANCE hInstance, LPTSTR lpCmdLinel, int nCmdShow);
    void NTWindow_ExitServer();
    void NTWindow_DeleteIcon();
    bool NTWindow_OnTick(int iWaitmSec);
    void NTWindow_CheckUpdateWindowTitle();
    bool NTWindow_CanScroll();

private:
    std::string _strWindowTitle;
    bool _fNewWindowTitle;
    std::shared_mutex _mutexWindowTitle;

    void exitActions();
    
public:
    class CAboutDlg : public CDialogBase				//	CNTWindow::CAboutDlg
    {
    private:
        bool OnInitDialog();
        bool OnCommand(WORD wNotifyCode, INT_PTR wID, HWND hwndCtl);
    public:
        virtual BOOL DefDialogProc(UINT message, WPARAM wParam, LPARAM lParam);
    };

    class CListTextConsole : public CTextConsole		//	CNTWindow::CListTextConsole
    {
        CListbox m_wndList;
    public:
        CListTextConsole(HWND hWndList)
        {
            m_wndList.m_hWnd = hWndList;
        }
        ~CListTextConsole()
        {
            m_wndList.OnDestroy();
        }
        virtual PLEVEL_TYPE GetPrivLevel() const
        {
            return PLEVEL_Admin;
        }
        virtual LPCTSTR GetName() const
        {
            return "Stats";
        }
        virtual void SysMessage(LPCTSTR pszMessage) const
        {
            if (pszMessage == nullptr)
                return;

            TCHAR * ppMessages[255];
            int iQty = Str_ParseCmds(const_cast<TCHAR*>(pszMessage), ppMessages, CountOf(ppMessages), "\n");
            for (int i = 0; i < iQty; ++i)
            {
                if (*ppMessages[i])
                    m_wndList.AddString(ppMessages[i]);
            }
        }
    };

    class CStatusDlg : public CDialogBase				//	CNTWindow::CStatusWnd
    {
    public:
        CListbox m_wndListClients;
        CListbox m_wndListStats;
    private:
        bool OnInitDialog();
        bool OnCommand(WORD wNotifyCode, INT_PTR wID, HWND hwndCtl);
    public:
        void FillClients();
        void FillStats();
        virtual BOOL DefDialogProc(UINT message, WPARAM wParam, LPARAM lParam);
    };

    COLORREF		m_dwColorNew;	// set the color for the next block written.
    COLORREF		m_dwColorPrv;
    CRichEditCtrl	m_wndLog;
    int				m_iLogTextLen;

    CEdit			m_wndInput;		// the text input portion at the bottom.
    int				m_iHeightInput;

    HFONT			m_hLogFont;

    bool m_fLogScrollLock;	// lock with the rolling text ?

private:
    int OnCreate(HWND hWnd, LPCREATESTRUCT lParam);
    bool OnSysCommand(WPARAM uCmdType, int xPos, int yPos);
    void OnSize(WPARAM nType, int cx, int cy);
    void OnDestroy();
    void OnSetFocus(HWND hWndLoss);
    bool OnClose();
    void OnUserPostMessage(COLORREF color, CSString * psMsg);
    LRESULT OnUserTrayNotify(WPARAM wID, LPARAM lEvent);
    LRESULT OnNotify(int idCtrl, NMHDR * pnmh);
    void	SetLogFont(const char * pszFont);

public:
    bool OnCommand(word wNotifyCode, INT_PTR wID, HWND hwndCtl);

    static bool RegisterClass(char *className);
    static LRESULT WINAPI WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    void List_Clear();
    void List_AddSingle(COLORREF color, LPCTSTR pszText);
    void List_AddGroup(std::deque<std::unique_ptr<ConsoleOutput>>&& msgs);
    void SetWindowTitle(LPCTSTR pText = nullptr);

    CNTWindow();
    virtual ~CNTWindow();

    char	m_zCommands[5][256];
} g_NTWindow;   // extern class

class CNTApp : public CWinApp
{
public:
    static const char *m_sClassName;
    CNTWindow m_wndMain;
    CNTWindow::CStatusDlg	m_wndStatus;
    CNTWindow::CAboutDlg    m_wndAbout;
};


#endif // _WINDOWS

#endif // _INC_NTWINDOW_H
