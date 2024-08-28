/**
* @file CAcount.h
*/

#ifndef _INC_CACCOUNT_H
#define _INC_CACCOUNT_H

#include "../../network/CSocket.h"
#include "../../common/sphere_library/CSString.h"
#include "../../common/sphereproto.h"
#include "../../common/CScriptObj.h"
#include "../chars/CCharRefArray.h"
#include "../CServerConfig.h"
#include "../game_enums.h"

#define PRIV_UNUSED0		0x0001
#define PRIV_GM				0x0002	// Acts as a GM (dif from having GM level)
#define PRIV_UNUSED1		0x0004
#define PRIV_GM_PAGE		0x0008	// Listen to GM pages or not.
#define PRIV_HEARALL		0x0010	// I can hear everything said by people of lower plevel
#define PRIV_ALLMOVE		0x0020	// I can move all things. (GM only)
#define PRIV_DETAIL			0x0040	// Show combat detail messages
#define PRIV_DEBUG			0x0080	// Show all objects as boxes and chars as humans.
#define PRIV_UNUSED2		0x0100
#define PRIV_PRIV_NOSHOW	0x0200	// Show the GM title and Invul flags.
#define PRIV_TELNET_SHORT	0x0400	// Disable broadcasts to be accepted by client
#define PRIV_JAILED			0x0800	// Must be /PARDONed from jail.
#define PRIV_UNUSED3		0x1000
#define PRIV_BLOCKED		0x2000	// The account is blocked.
#define PRIV_ALLSHOW		0x4000	// Show even the offline chars.
#define PRIV_UNUSED4		0x8000
#define PRIV_UNUSED (PRIV_UNUSED0|PRIV_UNUSED1|PRIV_UNUSED2|PRIV_UNUSED3|PRIV_UNUSED4)


class CClient;

/**
* @brief Single account information.
*/
class CAccount : public CScriptObj
{
	// RES_ACCOUNT
	static lpctstr const sm_szVerbKeys[]; // Action list.
	static lpctstr const sm_szLoadKeys[]; // Script fields.
private:
	PLEVEL_TYPE m_PrivLevel; // Privileges level of the CAccount.
	CSString m_sName; // Name = no spaces. case independant.
	CSString m_sCurPassword; // Accounts auto-generated but never used should not last long !
	CSString m_sNewPassword; // The new password will be transfered when they use it.


	word m_PrivFlags; // optional privileges for char (bit-mapped)

	byte m_ResDisp; // current CAccount resdisp.
	byte m_MaxChars; // Max chars allowed for this CAccount.

	typedef struct { llong m_First; llong m_Last; llong m_vcDelay; } TimeTriesStruct_t;
	typedef std::pair<TimeTriesStruct_t, int> BlockLocalTimePair_t;
	typedef std::map<dword,BlockLocalTimePair_t> BlockLocalTime_t;
	BlockLocalTime_t m_BlockIP; // Password tries.

public:
	static const char *m_sClassName;

	CLanguageID m_lang;			// UNICODE language preference (ENU=english).
	CSString m_sChatName;		// Chat System Name

	CSocketAddressIP m_First_IP;// First ip logged in from.
	CSocketAddressIP m_Last_IP;	// last ip logged in from.

	int64 _iTimeConnectedTotal;	// Previous total amount of time in game (minutes). "TOTALCONNECTTIME"

	CSTime _dateConnectedFirst;	// First date logged in (use localtime()).
	CSTime _dateConnectedLast;	// Last logged in date (use localtime()).
	int64  _iTimeConnectedLast;	// Amount of time spent online last time (in minutes).

	CUID m_uidLastChar;			// Last CChar logged with this CAccount.
	CCharRefArray m_Chars;		// CChars attached to this CAccount.
	CVarDefMap m_TagDefs;		// Tags storage system.
	CVarDefMap m_BaseDefs;		// New Variable storage system.
    uint8 _iMaxHouses;          // g_Cfg._iMaxHousesAccount
    uint8 _iMaxShips;           // g_Cfg._iMaxShipsAccount

public:
	/**
	* @brief Creates a new CAccount.
	* Also sanitizes name and register de CAccount.
	* @param pszName CAccount name.
	* @param fGuest flag for guest accounts.
	*/
	CAccount(lpctstr pszName, bool fGuest = false);

	CAccount(const CAccount& copy) = delete;
	CAccount& operator=(const CAccount& other) = delete;

public:
	lpctstr GetDefStr( lpctstr ptcKey, bool fZero = false ) const;
	int64 GetDefNum( lpctstr ptcKey ) const;
	void SetDefNum(lpctstr ptcKey, int64 iVal, bool fZero = true);
	void SetDefStr(lpctstr ptcKey, lpctstr pszVal, bool fQuoted = false, bool fZero = true);
	void DeleteDef(lpctstr ptcKey);


	/**
	* @brief Remove a CAccount.
	* We should go track down and delete all the chars and clients that use this account !
	*/
	// virtual not required at the moment but might be if subclassed
	virtual ~CAccount() override;

	/************************************************************************
	* SCP related section.
	************************************************************************/

	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual bool r_Verb( CScript &s, CTextConsole * pSrc ) override;
	virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
	void r_Write(CScript & s);

	/************************************************************************
	* Name and password related section.
	************************************************************************/

	/**
	* @brief Check and sanitizes name.
	* Basically only accepts [a-zA-Z0-9_]+ string. Also check if name is obscene.
	* @param pszNameOut output string.
	* @param pszNameInp input string.
	* @return true if name is a valid account name and not repeated, false otherwise.
	*/
	static bool NameStrip(tchar * pszNameOut, lpctstr pszNameInp);
	/**
	* @brief Get the CAccount name.
	* @return the CAccount name.
	*/
	virtual lpctstr GetName() const override {
	    return m_sName;
    }
	/**
	* @brief Get the CAccount password.
	* @return the CAccount password.
	*/
	lpctstr GetPassword() const { return( m_sCurPassword ); }
	/**
	* @brief Set a new password.
	* fires the server trigger f_onserver_pwchange. If true, return false.
	* If password is valid, set it and return true.
	* @return true if the password is set, false otherwise.
	*/
	bool SetPassword( lpctstr pszPassword, bool isMD5Hash = false );
	/**
	* @brief Removes the current password.
	* The password can be set on next login.
	*/
	void ClearPassword() { m_sCurPassword.Clear(); }
	/**
	* @brief Check password agains CAccount password.
	* If CAccount has no password and password length is 0, check fails.
	* If CAccount has no password and password lenght is greater than 0, try to set password check success.
	* Then server f_onaccount_connect trigger is fired. If it returns true, check fails.
	* Last the password is checked.
	* @param pszPassword pass to check.
	* @return true if password is ok and f_onaccount_connect not returns true. False otherwise.
	*/
	bool CheckPassword( lpctstr pszPassword );
	/**
	* @brief Get new password.
	* @return new password.
	*/
	lpctstr GetNewPassword() const { return( m_sNewPassword ); }
	void SetNewPassword( lpctstr pszPassword );
	/**
	* @brief Wrong pw given, check the failed tries to take actions again'st this ip.
	* If the peer is localhost or 127.0.0.1, the password try do not count.
	* Register the password try and, if there is too many password tries (>100) clear the last tries.
	* @param csaPeerName Client net information.
	* @return TODOC.
	*/
	bool CheckPasswordTries(CSocketAddress csaPeerName);
	/**
	* @brief Removes saved data related to passwordtries for this CAccount.
	* We can remove the last 3 minutes or all the data.
	* @param bAll true if we want to remove all passwordtries data.
	*/
	void ClearPasswordTries(bool bAll = false);

	/************************************************************************
	* Resdisp related section.
	************************************************************************/
	/**
	* @brief Sets the resdisp value to the CAccount.
	* @param what resdisp to set.
	* @return true on success, false otherwise.
	*/
	bool SetResDisp(byte what);
	/**
	* @brief Gets the current resdisp on this CAccount.
	* @return The current resdisp.
	*/
	byte GetResDisp() const { return m_ResDisp; }
	/**
	* @brief Updates the resdisp if the current is lesser.
	* @param what the resdisp to update.
	* @return true if success, false otherwise.
	*/
	bool SetGreaterResDisp(byte what);
	/**
	* @brief Sets the resdisp on this CAccount based on pClient version.
	* @return true if success, false otherwise.
	*/
	bool SetAutoResDisp(CClient *pClient);
	/**
	* @brief Check the current resdisp.
	* @param what the resdisp to check.
	* @return true if the current resdisp is equal to what, false otherwise.
	*/
	bool IsResDisp(byte what) const { return ( m_ResDisp == what ); }

	/************************************************************************
	* Privileges related section.
	************************************************************************/
	/**
	* @brief Sets the PLEVEL of the CAccount.
	* @param plevel PLEVEL to set.
	*/
	void SetPrivLevel( PLEVEL_TYPE plevel );
	/**
	* @brief Gets the CAccount PLEVEL description.
	* @param pszFlags TODOC.
	* @return TODOC.
	*/
	static PLEVEL_TYPE GetPrivLevelText( lpctstr pszFlags );
	/**
	* @brief Gets the CAccount PLEVEL.
	* @param pszFlags TODOC.
	* @return TODOC.
	*/
	PLEVEL_TYPE GetPrivLevel() const { return( m_PrivLevel ); }
	/**
	* @brief Check CAccount for a specified privilege flags.
	* @param wPrivFlags Privilege flags to test.
	* @return true if all the flags are set, false otherwise.
	*/
	bool IsPriv( word wPrivFlags ) const { return (m_PrivFlags & wPrivFlags) != 0; }
	/**
	* @brief Set the privileges flags specified.
	* @param wPrivFlags flags to set.
	*/
	void SetPrivFlags( word wPrivFlags ) { m_PrivFlags |= wPrivFlags; }
	/**
	* @brief Unset the privileges flags specified.
	* @param wPrivFlags flags to unset.
	*/
	void ClearPrivFlags( word wPrivFlags ) { m_PrivFlags &= ~wPrivFlags; }
	/**
	* @brief Operate with privilege flags.
	* If pszArgs is empty, only intersection privileges with wPrivFlags are set.
	* If pszArgs is true, set wPrivFlags.
	* If pszArgs is false, unset wPrivFlags.
	* @param wPrivFlags flags to intersect, set or unset.
	* @param pszArgs the operation to do.
	*/
	void TogPrivFlags( word wPrivFlags, lpctstr pszArgs );

	/************************************************************************
	* Log in / Log out related section.
	************************************************************************/

	/**
	* @brief Updates context information on login.
	* Updates last time connected, flags (if GM) and first time connected if needed.
	* @param pClient client logged in with this CAccount.
	*/
	void OnLogin( CClient * pClient );
	/**
	* @brief Updates context information on logout.
	* Updates total time connected.
	* @param pClient client logging out from this CAccount.
	* @param fWasChar true if is logged with a CChar.
	*/
	void OnLogout(CClient *pClient, bool fWasChar = false);
	/**
	* @brief Kick / Ban a player.
	* Only if plevel of CAccount is higher than SRC plevel, do not kick or ban.
	* @param pSrc SRC of the action.
	* @param fBlock if true, kick and ban.
	*/
	bool Kick(CTextConsole * pSrc, bool fBlock);

	/************************************************************************
	* CChars related section.
	************************************************************************/

	/**
	* @brief Get the max chars count for this CAccount.
	* @return the max chars for this CAccount.
	*/
	byte GetMaxChars() const;
	/**
	* @brief Set the max chars for this acc.
	* The max is set only if the current number of chars is lesser than the new value.
	* @param chars New value for max chars.
	*/
	void SetMaxChars(byte chars);
	/**
	* @brief Check if a CChar is owned by this CAccount.
	* @param pChar CChar to check.
	* @return true if this CAccount owns the CChar. False otherwise.
	*/
	bool IsMyAccountChar( const CChar * pChar ) const;
	/**
	* @brief Unlink the CChar from this CAccount.
	* @param CChar to detach.
	* @return TODOC.
	*/
	size_t DetachChar( CChar * pChar );
	/**
	* @brief Link the CChar to this CAccount.
	* @param pChar CChar to link.
	* @return TODOC.
	*/
	size_t AttachChar( CChar * pChar );
	/**
	* @brief Removes all chars from this CAccount.
	* If client is connected, kicks it.
	*/
	void DeleteChars();
	/**
	* @brief Get the client logged into a CAccount.
	* Can be used to check if the CAccount is logged in.
	* @param pExclude excluded client.
	* @return CClient logged into de CAccount, nullptr otherwise.
	*/
	CClient * FindClient( const CClient * pExclude = nullptr ) const;
    void SetBlockStatus(bool fNewStatus);
};


/**
* @brief The full accounts database.
* This class has methods to manage accounts by scripts and by command interface.
* Stuff saved in *ACCT.SCP file.
*/
class CAccounts
{
protected:
	static const char *m_sClassName; // TODOC.
	static lpctstr const sm_szVerbKeys[]; // ACCOUNT action list.
	CObjNameSortArray m_Accounts; // Sorted CAccount list.
public:
    CAccounts() noexcept
        : m_fLoading(false)
    {
    }
	/**
	* CAccount needs CAccounts methods.
	* @see CAccount
	*/
	friend class CAccount;
	/**
	* Used to control if we are loading account files.
	*/
	bool m_fLoading;
private:
	/**
	* @brief Add a new CAccount, command interface.
	* Check if extist a CAccount with the same name, and validate the name. If not exists and name is valid, create the CAccount.
	* @param pSrc command shell interface.
	* @param pszName new CAccount name.
	* @param ptcArg new CAccount password.
	* @param md5 true if we need md5 to store the password.
	* @return true if CAccount creation is success, false otherwise.
	*/
	bool Cmd_AddNew( CTextConsole * pSrc, lpctstr pszName, lpctstr ptcArg, bool md5=false );
	/**
	* @brief Do something to all the unused accounts.
	* First check for accounts with an inactivity of greater or equal to pszDays. Then perform the action to that accounts.
	* If action is DELETE, the accounts with privileges will not be removed.
	* @param pSrc command shell interface.
	* @param pszDays number of days of inactivity to consider a CAccount unused.
	* @param pszVerb action to perform.
	* @param pszArgs args to action.
	* @param dwMask discard any CAccount with this mask.
	* @return Always true.
	*/
	bool Cmd_ListUnused( CTextConsole * pSrc, lpctstr pszDays, lpctstr pszVerb, lpctstr pszArgs, dword dwMask = 0);
public:
	/**
	* @brief Save the accounts file.
	* @return true if successfully saved, false otherwise.
	*/
	bool Account_SaveAll();
	/**
	* @brief Load a single account.
	* @see Account_LoadAll()
	* @param pszNameRaw header of ACCOUNT section.
	* @param s Arguments for account.
	* @param fChanges false = trap duplicates.
	* @return true if account is successfully loaded, false otherwise.
	*/
	bool Account_Load( lpctstr pszNameRaw, CScript & s, bool fChanges );
	/**
	* @brief Load account file.
	* If fChanges is true, will read acct file.
	* @param fChanges true if is an update.
	* @param fClearChanges true to clear the acct file.
	* @return true if succesfully load new accounts, false otherwise.
	*/
	bool Account_LoadAll( bool fChanges = true, bool fClearChanges = false );
	/**
	* @brief Perform actions to accounts via command shell interface (Command ACCOUNT).
	* @param pszArgs args provided to command ACCOUNT.
	* @param pSrc command shell interface.
	* @return the status of the command executed.
	*/
	bool Account_OnCmd( tchar * pszArgs, CTextConsole * pSrc );
	/**
	* @brief Get the CAccount count.
	* @return The count of CAccounts.
	*/
	size_t Account_GetCount() const;
	/**
	* @brief Get a CAccount * of an CAccount by his index.
	* @param index array index of the CAccount.
	* @return CAccount * of the CAccount if index is valid, nullptr otherwise.
	*/
	CAccount * Account_Get( size_t index );
	/**
	* @brief Get a CAccount * from a valid name.
	* If the name is not valid nullptr is returned.
	* @param pszName the name of the CAccount we are looking for.
	* @return CAccount * if pszName si a valid account name and exists an CAccount with that name, Null otherwise.
	*/
	CAccount * Account_Find( lpctstr pszName );
	/**
	* @brief Get or create an CAccount in some circumstances.
	* If there is an CAccount with the provided name, a CAccount * of the account is returned.
	* If there is not an CAccount with the providded name, AutoAccount is enabled in sphere.ini and the name is a valid account name, a CAcount is created and a CAccount * of the returned.
	* Otherwise, nullptr is returned.
	* @param pszName name of the account.
	* @param fAutoCreate try to create the account if not exists.
	* @return CAccount * if account exists or created, nullptr otherwise.
	*/
	CAccount * Account_FindCreate( lpctstr pszName, bool fCreate = false );
	/**
	* @brief Check if a chat name is already used.
	* @param pszChatName string containing the name.
	* @return CAccount * if the name is already used, nullptr otherwise.
	*/
	CAccount * Account_FindChat( lpctstr pszName );
	/**
	* @brief Remove an CAccount.
	* First try to call the f_onaccount_delete server trigger. If trigger returns true, do not remove the account and return false. If trigger returns false, remove the account and return true.
	* @param pAccount CAccount to remove.
	* @return true if CAccount was successfully removed, true otherwise.
	*/
	bool Account_Delete( CAccount * pAccount );
	/**
	* @brief Add a new CAccount.
	* First call f_onaccount_create trigger. If trigger returns true, cancel creation. If triggers return false, create the CAccount.
	* @param pAccount Account to create.
	*/
	void Account_Add( CAccount * pAccount );
};

/**
* All the player accounts. Name sorted CAccount.
*/
extern CAccounts g_Accounts;

#endif // _INC_CACCOUNT_H

