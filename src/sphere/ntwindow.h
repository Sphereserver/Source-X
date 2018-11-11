/**
* @file ntwindow.h
*
*/

#ifndef _INC_NTWINDOW_H
#define _INC_NTWINDOW_H
#ifdef _WINDOWS
#include "ConsoleInterface.h"
#include "../common/sphere_library/CSWindow.h"
#include "../common/CTextConsole.h"

extern class CNTWindow : public CSWindow, public ConsoleInterface
{
public:
    bool NTWindow_Init(HINSTANCE hInstance, LPTSTR lpCmdLinel, int nCmdShow);
    void NTWindow_Exit();
    void NTWindow_DeleteIcon();
    bool NTWindow_OnTick(int iWaitmSec);
    bool NTWindow_PostMsg(ConsoleOutput *output);
    bool NTWindow_PostMsgColor(COLORREF color);
    void NTWindow_SetWindowTitle(LPCTSTR pText = nullptr);

    static const char *m_sClassName;
    class CAboutDlg : public CDialogBase				//	CNTWindow::CAboutDlg
    {
    private:
        bool OnInitDialog();
        bool OnCommand(word wNotifyCode, INT_PTR wID, HWND hwndCtl);
    public:
        virtual BOOL DefDialogProc(UINT message, WPARAM wParam, LPARAM lParam);
    };

    class COptionsDlg : public CDialogBase				//	CNTWindow::COptionsDlg
    {
    private:
        bool OnInitDialog();
        bool OnCommand(word wNotifyCode, INT_PTR wID, HWND hwndCtl);
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
            if (pszMessage == nullptr || ISINTRESOURCE(pszMessage))
                return;

            TCHAR * ppMessages[255];
            size_t iQty = Str_ParseCmds(const_cast<TCHAR*>(pszMessage), ppMessages, CountOf(ppMessages), "\n");
            for (size_t i = 0; i < iQty; ++i)
            {
                if (*ppMessages[i])
                    m_wndList.AddString(ppMessages[i]);
            }
        }
    };

    class CStatusWnd : public CDialogBase				//	CNTWindow::CStatusWnd
    {
    public:
        CListbox m_wndListClients;
        CListbox m_wndListStats;
    private:
        bool OnInitDialog();
        bool OnCommand(word wNotifyCode, INT_PTR wID, HWND hwndCtl);
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
    void List_Add(COLORREF color, LPCTSTR pszText);

    CNTWindow();
    virtual ~CNTWindow();

    char	m_zCommands[5][256];
} g_NTWindow;   // extern class

class CNTApp : public CWinApp
{
public:
    static const char *m_sClassName;
    CNTWindow m_wndMain;
    CNTWindow::CStatusWnd	m_wndStatus;
    CNTWindow::COptionsDlg	m_dlgOptions;
};


#endif // _WINDOWS

#endif // _INC_NTWINDOW_H