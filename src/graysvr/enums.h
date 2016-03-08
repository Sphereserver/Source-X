#ifndef _INC_ENUMS_H
#define _INC_ENUMS_H

enum CLIMODE_TYPE	// What mode is the client to server connection in ? (waiting for input ?)
{
    // Setup events ------------------------------------------------------------------
            CLIMODE_SETUP_CONNECTING,
    CLIMODE_SETUP_SERVERS,			// client has received the servers list
    CLIMODE_SETUP_RELAY,			// client has been relayed to the game server. wait for new login
    CLIMODE_SETUP_CHARLIST,			// client has the char list and may (select char, delete char, create new char)

    // Capture the user input for this mode  -----------------------------------------
            CLIMODE_NORMAL,					// No targeting going on, we are just walking around, etc

    // Asyc events enum here  --------------------------------------------------------
            CLIMODE_DRAG,					// I'm dragging something (not quite a targeting but similar)
    CLIMODE_DYE,					// the dye dialog is up and I'm targeting something to dye
    CLIMODE_INPVAL,					// special text input dialog (for setting item attrib)

    // Some sort of general gump dialog ----------------------------------------------
            CLIMODE_DIALOG,					// from RES_DIALOG

    // Hard-coded (internal) dialogs
            CLIMODE_DIALOG_VIRTUE = 0x1CD,

    // Making a selection from a menu  -----------------------------------------------
            CLIMODE_MENU,					// from RES_MENU

    // Hard-coded (internal) menus
            CLIMODE_MENU_SKILL,				// result of some skill (tracking, tinkering, blacksmith, etc)
    CLIMODE_MENU_SKILL_TRACK_SETUP,
    CLIMODE_MENU_SKILL_TRACK,
    CLIMODE_MENU_GM_PAGES,			// open gm pages list
    CLIMODE_MENU_EDIT,				// edit the contents of a container

    // Prompting for text input ------------------------------------------------------
            CLIMODE_PROMPT_NAME_RUNE,
    CLIMODE_PROMPT_NAME_KEY,		// naming a key
    CLIMODE_PROMPT_NAME_SIGN,		// naming a house sign
    CLIMODE_PROMPT_NAME_SHIP,
    CLIMODE_PROMPT_GM_PAGE_TEXT,	// allowed to enter text for GM page
    CLIMODE_PROMPT_VENDOR_PRICE,	// what would you like the price to be?
    CLIMODE_PROMPT_TARG_VERB,		// send message to another player
    CLIMODE_PROMPT_SCRIPT_VERB,		// script verb
    CLIMODE_PROMPT_STONE_NAME,		// prompt for text

    // Targeting mouse cursor  -------------------------------------------------------
            CLIMODE_MOUSE_TYPE,				// greater than this = mouse type targeting

    // GM targeting command stuff
            CLIMODE_TARG_OBJ_SET,			// set some attribute of the item I will show
    CLIMODE_TARG_OBJ_INFO,			// what item do I want props for?
    CLIMODE_TARG_OBJ_FUNC,

    CLIMODE_TARG_UNEXTRACT,			// break out multi items
    CLIMODE_TARG_ADDCHAR,			// "ADDNPC" command
    CLIMODE_TARG_ADDITEM,			// "ADDITEM" command
    CLIMODE_TARG_LINK,				// "LINK" command
    CLIMODE_TARG_TILE,				// "TILE" command

    // Normal user stuff  (mouse targeting)
            CLIMODE_TARG_SKILL,				// targeting a skill or spell
    CLIMODE_TARG_SKILL_MAGERY,
    CLIMODE_TARG_SKILL_HERD_DEST,
    CLIMODE_TARG_SKILL_POISON,
    CLIMODE_TARG_SKILL_PROVOKE,

    CLIMODE_TARG_USE_ITEM,			// target for using the selected item
    CLIMODE_TARG_PET_CMD,			// targeted pet command
    CLIMODE_TARG_PET_STABLE,		// pick a creature to stable
    CLIMODE_TARG_REPAIR,			// attempt to repair an item
    CLIMODE_TARG_STONE_RECRUIT,		// recruit members for a stone (mouse select)
    CLIMODE_TARG_STONE_RECRUITFULL,	// recruit/make a member and set abbrev show
    CLIMODE_TARG_PARTY_ADD,

    CLIMODE_TARG_QTY
};


#endif //_INC_ENUMS_H
