# Main program files: threads, console...
SET (sphere_SRCS
sphere/asyncdb.cpp
sphere/asyncdb.h
sphere/containers.h
sphere/ConsoleInterface.cpp
sphere/ConsoleInterface.h
sphere/ProfileData.cpp
sphere/ProfileData.h
sphere/ProfileTask.cpp
sphere/ProfileTask.h
sphere/threads.cpp
sphere/threads.h
sphere/ntservice.cpp
sphere/ntservice.h
sphere/ntwindow.cpp
sphere/ntwindow.h
sphere/UnixTerminal.cpp
sphere/UnixTerminal.h
sphere/SphereSvr.rc
)
SOURCE_GROUP (sphere FILES ${sphere_SRCS})

# Network management files
SET (network_SRCS
network/CClientIterator.cpp
network/CClientIterator.h
network/CIPHistoryManager.cpp
network/CIPHistoryManager.h
network/CNetState.cpp
network/CNetState.h
network/CNetworkInput.cpp
network/CNetworkInput.h
network/CNetworkManager.cpp
network/CNetworkManager.h
network/CNetworkOutput.cpp
network/CNetworkOutput.h
network/CNetworkThread.cpp
network/CNetworkThread.h
network/CPacketManager.cpp
network/CPacketManager.h
network/CSocket.cpp
network/CSocket.h
network/linuxev.cpp
network/linuxev.h
network/net_datatypes.h
network/net_datatypes.cpp
network/packet.cpp
network/packet.h
network/receive.cpp
network/receive.h
network/send.cpp
network/send.h
network/PingServer.cpp
network/PingServer.h
)
SOURCE_GROUP (network FILES ${network_SRCS})

# Login encryption handling
SET (crypto_SRCS
common/crypto/CBCrypt.cpp
common/crypto/CBCrypt.h
common/crypto/CCrypto.cpp
common/crypto/CCrypto.h
common/crypto/CCryptoBlowFish.cpp
common/crypto/CCryptoHuffman.cpp
common/crypto/CCryptoLogin.cpp
common/crypto/CCryptoMD5Interface.cpp
common/crypto/CCryptoTwoFishInterface.cpp
common/crypto/CMD5.cpp
common/crypto/CMD5.h
common/crypto/bcrypt/crypt_blowfish.c
common/crypto/bcrypt/crypt_blowfish.h
common/crypto/bcrypt/crypt_gensalt.c
common/crypto/bcrypt/crypt_gensalt.h
common/crypto/bcrypt/ow-crypt.h
common/crypto/bcrypt/wrapper.c
common/crypto/twofish/twofish.cpp
common/crypto/twofish/twofish.h
common/crypto/twofish/twofish_aux.h
)
SOURCE_GROUP (common\\crypto FILES ${crypto_SRCS})

# Handle UO Client files
SET (uofiles_SRCS
game/uo_files/CUOHuesRec.h
game/uo_files/CUOHuesRec.cpp
game/uo_files/CUOIndexRec.h
game/uo_files/CUOIndexRec.cpp
game/uo_files/CUOItemInfo.h
game/uo_files/CUOItemInfo.cpp
game/uo_files/CUOItemTypeRec.h
game/uo_files/CUOMapBlock.h
game/uo_files/CUOMapList.h
game/uo_files/CUOMapList.cpp
game/uo_files/CUOMapMeter.h
game/uo_files/CUOMapMeter.cpp
game/uo_files/CUOMultiItemRec.h
game/uo_files/CUOMultiItemRec.cpp
game/uo_files/CUOStaticItemRec.h
game/uo_files/CUOStaticItemRec.cpp
game/uo_files/CUOTerrainInfo.h
game/uo_files/CUOTerrainInfo.cpp
game/uo_files/CUOTerrainTypeRec.h
game/uo_files/CUOTiledata.h
game/uo_files/CUOTiledata.cpp
game/uo_files/CUOVersionBlock.h
game/uo_files/CUOVersionBlock.cpp
game/uo_files/uofiles_enums.h
game/uo_files/uofiles_enums_itemid.h
game/uo_files/uofiles_enums_creid.h
game/uo_files/uofiles_macros.h
game/uo_files/uofiles_types.h
)
SOURCE_GROUP (game\\uo_files FILES ${uofiles_SRCS})

# Files containing 'background work'
SET (common_SRCS
common/assertion.h
common/basic_threading.h
common/CCacheableScriptFile.cpp
common/CCacheableScriptFile.h
common/CDataBase.cpp
common/CDataBase.h
common/CException.cpp
common/CException.h
common/CExpression.cpp
common/CExpression.h
common/CFloatMath.cpp
common/CFloatMath.h
common/CLocalVarsExtra.cpp
common/CLocalVarsExtra.h
common/CLog.cpp
common/CLog.h
common/CServerMap.cpp
common/CServerMap.h
common/CUID.cpp
common/CUID.h
common/common.cpp
common/common.h
common/CPointBase.cpp
common/CPointBase.h
common/CRect.cpp
common/CRect.h
common/CScript.cpp
common/CScript.h
common/CScriptContexts.cpp
common/CScriptContexts.h
common/CScriptObj.cpp
common/CScriptObj.h
common/CScriptTriggerArgs.cpp
common/CScriptTriggerArgs.h
common/CSFileObj.cpp
common/CSFileObj.h
common/CSFileObjContainer.cpp
common/CSFileObjContainer.h
common/CSVFile.cpp
common/CSVFile.h
common/CTextConsole.cpp
common/CTextConsole.h
common/CUOInstall.cpp
common/CUOInstall.h
common/CVarDefMap.cpp
common/CVarDefMap.h
common/datatypes.h
common/sphereproto.h
common/sphereversion.h
common/ListDefContMap.cpp
common/ListDefContMap.h
common/os_unix.h
common/os_windows.h
common/regex/deelx.h
)
SOURCE_GROUP (common FILES ${common_SRCS})

SET (resource_SRCS
common/resource/CResourceBase.cpp
common/resource/CResourceBase.h
common/resource/CResourceDef.cpp
common/resource/CResourceDef.h
common/resource/CResourceID.cpp
common/resource/CResourceID.h
common/resource/CResourceHash.cpp
common/resource/CResourceHash.h
common/resource/CResourceLink.cpp
common/resource/CResourceLink.h
common/resource/CResourceLock.cpp
common/resource/CResourceLock.h
common/resource/CResourceRef.cpp
common/resource/CResourceRef.h
common/resource/CResourceScript.cpp
common/resource/CResourceScript.h
common/resource/CResourceSortedArrays.cpp
common/resource/CResourceSortedArrays.h
common/resource/CResourceQty.cpp
common/resource/CResourceQty.h
common/resource/CValueDefs.cpp
common/resource/CValueDefs.h
)
SOURCE_GROUP (common\\resource FILES ${resource_SRCS})

SET (resourcesections_SRCS
common/resource/sections/CDialogDef.cpp
common/resource/sections/CDialogDef.h
common/resource/sections/CItemTypeDef.cpp
common/resource/sections/CItemTypeDef.h
common/resource/sections/CRandGroupDef.cpp
common/resource/sections/CRandGroupDef.h
common/resource/sections/CRegionResourceDef.cpp
common/resource/sections/CRegionResourceDef.h
common/resource/sections/CResourceNamedDef.cpp
common/resource/sections/CResourceNamedDef.h
common/resource/sections/CSkillClassDef.cpp
common/resource/sections/CSkillClassDef.h
common/resource/sections/CSkillDef.cpp
common/resource/sections/CSkillDef.h
common/resource/sections/CSpellDef.cpp
common/resource/sections/CSpellDef.h
common/resource/sections/CWebPageDef.cpp
common/resource/sections/CWebPageDef.h
)
SOURCE_GROUP (common\\resource\\sections FILES ${resourcesections_SRCS})

# Sphere library files
SET (spherelibrary_SRCS
common/sphere_library/CSAssoc.cpp
common/sphere_library/CSAssoc.h
common/sphere_library/CSFile.cpp
common/sphere_library/CSFile.h
common/sphere_library/CSFileList.cpp
common/sphere_library/CSFileList.h
common/sphere_library/CSFileText.cpp
common/sphere_library/CSFileText.h
common/sphere_library/CSMemBlock.cpp
common/sphere_library/CSMemBlock.h
common/sphere_library/CSObjArray.h
common/sphere_library/CSObjCont.cpp
common/sphere_library/CSObjCont.h
common/sphere_library/CSObjContRec.h
common/sphere_library/CSObjList.cpp
common/sphere_library/CSObjList.h
common/sphere_library/CSObjListRec.h
common/sphere_library/CSObjSortArray.h
common/sphere_library/CSPtrTypeArray.h
common/sphere_library/CSSortedVector.h
common/sphere_library/CSTypedArray.h
common/sphere_library/CSQueue.cpp
common/sphere_library/CSQueue.h
common/sphere_library/CSRand.cpp
common/sphere_library/CSRand.h
common/sphere_library/CSString.cpp
common/sphere_library/CSString.h
common/sphere_library/CSTime.cpp
common/sphere_library/CSTime.h
common/sphere_library/CSWindow.cpp
common/sphere_library/CSWindow.h
common/sphere_library/smap.h
common/sphere_library/smutex.h
common/sphere_library/smutex.cpp
common/sphere_library/squeues.h
common/sphere_library/sresetevents.cpp
common/sphere_library/sresetevents.h
common/sphere_library/sstacks.h
common/sphere_library/sstring.cpp
common/sphere_library/sstring.h
common/sphere_library/sstringobjs.cpp
common/sphere_library/sstringobjs.h
)
SOURCE_GROUP (common\\sphere_library FILES ${spherelibrary_SRCS})

# Main game files.
SET (game_SRCS
game/CBase.cpp
game/CBase.h
game/CContainer.cpp
game/CContainer.h
game/CComponent.cpp
game/CComponent.h
game/CComponentProps.cpp
game/CComponentProps.h
game/CEntity.cpp
game/CEntity.h
game/CEntityProps.cpp
game/CEntityProps.h
game/CObjBase.cpp
game/CObjBase.h
game/CObjBaseTemplate.cpp
game/CObjBaseTemplate.h
game/CPathFinder.cpp
game/CPathFinder.h
game/CRegion.cpp
game/CRegion.h
game/CRegionBase.cpp
game/CRegionBase.h
game/CResourceCalc.cpp
game/CScriptProfiler.h
game/CSector.cpp
game/CSector.h
game/CSectorEnviron.h
game/CSectorEnviron.cpp
game/CSectorTemplate.cpp
game/CSectorTemplate.h
game/CSectorList.cpp
game/CSectorList.h
game/CServer.cpp
game/CServer.h
game/CServerConfig.cpp
game/CServerConfig.h
game/CServerDef.cpp
game/CServerDef.h
game/CServerTime.cpp
game/CServerTime.h
game/CTimedFunctionHandler.cpp
game/CTimedFunctionHandler.h
game/CTimedFunctions.cpp
game/CTimedFunctions.h
game/CTimedObject.cpp
game/CTimedObject.h
game/CWorld.cpp
game/CWorld.h
game/CWorldCache.cpp
game/CWorldCache.h
game/CWorldClock.cpp
game/CWorldClock.h
game/CWorldComm.cpp
game/CWorldComm.h
game/CWorldGameTime.cpp
game/CWorldGameTime.h
game/CWorldImport.cpp
game/CWorldMap.cpp
game/CWorldMap.h
game/CWorldTicker.cpp
game/CWorldTicker.h
game/CWorldTickingList.cpp
game/CWorldTickingList.h
game/game_enums.h
game/game_macros.h
game/spheresvr.cpp
game/spheresvr.h
game/triggers.h
game/triggers.cpp
)
SOURCE_GROUP (game FILES ${game_SRCS})

SET (items_SRCS
game/items/CItem.cpp
game/items/CItem.h
game/items/CItemBase.cpp
game/items/CItemBase.h
game/items/CItemCommCrystal.cpp
game/items/CItemCommCrystal.h
game/items/CItemContainer.cpp
game/items/CItemContainer.h
game/items/CItemCorpse.cpp
game/items/CItemCorpse.h
game/items/CItemMap.cpp
game/items/CItemMap.h
game/items/CItemMemory.cpp
game/items/CItemMemory.h
game/items/CItemMessage.cpp
game/items/CItemMessage.h
game/items/CItemMulti.cpp
game/items/CItemMulti.h
game/items/CItemMultiCustom.cpp
game/items/CItemMultiCustom.h
game/items/CItemPlant.cpp
game/items/CItemScript.cpp
game/items/CItemScript.h
game/items/CItemShip.cpp
game/items/CItemShip.h
game/items/CItemScript.cpp
game/items/CItemScript.h
game/items/CItemStone.cpp
game/items/CItemStone.h
game/items/CItemVendable.cpp
game/items/CItemVendable.h
game/items/item_types.h
)
SOURCE_GROUP (game\\items FILES ${items_SRCS})

SET (chars_SRCS
game/chars/CCharAct.cpp
game/chars/CCharBase.cpp
game/chars/CChar.cpp
game/chars/CChar.h
game/chars/CCharAttacker.cpp
game/chars/CCharBase.h
game/chars/CCharFight.cpp
game/chars/CCharLOS.cpp
game/chars/CCharMemory.cpp
game/chars/CCharNotoriety.cpp
game/chars/CCharNPC.cpp
game/chars/CCharNPC.h
game/chars/CCharNPCAct.cpp
game/chars/CCharNPCAct_Fight.cpp
game/chars/CCharNPCAct_Magic.cpp
game/chars/CCharNPCAct_Vendor.cpp
game/chars/CCharNPCPet.cpp
game/chars/CCharNPCStatus.cpp
game/chars/CCharPlayer.cpp
game/chars/CCharPlayer.h
game/chars/CCharRefArray.h
game/chars/CCharRefArray.cpp
game/chars/CCharSkill.cpp
game/chars/CCharSpell.cpp
game/chars/CCharStat.cpp
game/chars/CCharStatus.cpp
game/chars/CCharUse.cpp
game/chars/CStoneMember.h
game/chars/CStoneMember.cpp
)
SOURCE_GROUP (game\\chars FILES ${chars_SRCS})

SET (clients_SRCS
game/clients/CAccount.cpp
game/clients/CAccount.h
game/clients/CChat.cpp
game/clients/CChat.h
game/clients/CChatChannel.cpp
game/clients/CChatChannel.h
game/clients/CChatChanMember.cpp
game/clients/CChatChanMember.h
game/clients/CClient.cpp
game/clients/CClientDialog.cpp
game/clients/CClientEvent.cpp
game/clients/CClient.h
game/clients/CClientLog.cpp
game/clients/CClientMsg.cpp
game/clients/CClientMsg_AOSTooltip.cpp
game/clients/CClientTarg.cpp
game/clients/CClientTooltip.h
game/clients/CClientTooltip.cpp
game/clients/CClientUse.cpp
game/clients/CGMPage.cpp
game/clients/CGMPage.h
game/clients/CParty.cpp
game/clients/CParty.h
)
SOURCE_GROUP (game\\clients FILES ${clients_SRCS})

SET (components_SRCS
game/components/CCChampion.cpp
game/components/CCChampion.h
game/components/CCFaction.cpp
game/components/CCFaction.h
game/components/CCItemDamageable.cpp
game/components/CCItemDamageable.h
game/components/CCMultiMovable.cpp
game/components/CCMultiMovable.h
game/components/CCPropsChar.cpp
game/components/CCPropsChar.h
game/components/CCPropsItem.cpp
game/components/CCPropsItem.h
game/components/CCPropsItemChar.cpp
game/components/CCPropsItemChar.h
game/components/CCPropsItemEquippable.cpp
game/components/CCPropsItemEquippable.h
game/components/CCPropsItemWeapon.cpp
game/components/CCPropsItemWeapon.h
game/components/CCPropsItemWeaponRanged.cpp
game/components/CCPropsItemWeaponRanged.h
game/components/CCSpawn.cpp
game/components/CCSpawn.h
)
SOURCE_GROUP (game\\components FILES ${components_SRCS})

# CrashDump files
SET (crashdump_SRCS
common/crashdump/crashdump.cpp
common/crashdump/crashdump.h
common/crashdump/mingwdbghelp.h
)
SOURCE_GROUP (common\\crashdump FILES ${crashdump_SRCS})

# LibEv files
SET (libev_SRCS
common/libev/wrapper_ev.c
)
SOURCE_GROUP (common\\libev FILES ${libev_SRCS})

# SQLite files
SET (sqlite_SRCS
common/sqlite/sqlite3.c
common/sqlite/sqlite3.h
common/sqlite/SQLite.cpp
common/sqlite/SQLite.h
)
SOURCE_GROUP (common\\sqlite FILES ${sqlite_SRCS})

# ZLib files
SET (zlib_SRCS
common/zlib/adler32.c
common/zlib/compress.c
common/zlib/crc32.c
common/zlib/crc32.h
common/zlib/deflate.c
common/zlib/deflate.h
common/zlib/gzclose.c
common/zlib/gzguts.h
common/zlib/gzlib.c
common/zlib/gzread.c
common/zlib/gzwrite.c
common/zlib/infback.c
common/zlib/inffast.c
common/zlib/inffast.h
common/zlib/inffixed.h
common/zlib/inflate.c
common/zlib/inflate.h
common/zlib/inftrees.c
common/zlib/inftrees.h
common/zlib/trees.c
common/zlib/trees.h
common/zlib/uncompr.c
common/zlib/zconf.h
common/zlib/zlib.h
common/zlib/zutil.c
common/zlib/zutil.h
)
SOURCE_GROUP (common\\zlib FILES ${zlib_SRCS})

# Table definitions
SET (tables_SRCS
tables/CBaseBaseDef_props.tbl
tables/CChar_functions.tbl
tables/CChar_props.tbl
tables/CCharBase_props.tbl
tables/CCharNpc_props.tbl
tables/CCharPlayer_functions.tbl
tables/CCharPlayer_props.tbl
tables/CClient_functions.tbl
tables/CClient_props.tbl
tables/CCPropsChar_props.tbl
tables/CCPropsItem_props.tbl
tables/CCPropsItemChar_props.tbl
tables/CCPropsItemEquippable_props.tbl
tables/CCPropsItemWeapon_props.tbl
tables/CCPropsItemWeaponRanged_props.tbl
tables/CSFileObj_functions.tbl
tables/CSFileObj_props.tbl
tables/CSFileObjContainer_functions.tbl
tables/CSFileObjContainer_props.tbl
tables/CItem_functions.tbl
tables/CItem_props.tbl
tables/CItemBase_props.tbl
tables/CItemStone_functions.tbl
tables/CItemStone_props.tbl
tables/classnames.tbl
tables/CObjBase_functions.tbl
tables/CObjBase_props.tbl
tables/CParty_functions.tbl
tables/CParty_props.tbl
tables/CScriptObj_functions.tbl
tables/CSector_functions.tbl
tables/CStoneMember_functions.tbl
tables/CStoneMember_props.tbl
tables/defmessages.tbl
tables/triggers.tbl
)
SOURCE_GROUP (tables FILES ${tables_SRCS})

# Misc doc and *.ini files
SET (docs_TEXT
../Changelog-X1-Nightlies.txt
sphere.ini
sphereCrypt.ini
)

INCLUDE_DIRECTORIES (
common/mysql/
common/flat_containers/
common/parallel_hashmap/
)
