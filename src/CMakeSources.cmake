##############################
# Precompiled Header Files
##############################

# Precompiled header: it's parsed only once and stored in memory as an intermediate form. This speeds up compilation drastically.
# Be wise in choosing those, since every time we change one of them, we'll have to recompile every one on the list.

set(pch_list
    PRIVATE src/common/common.h
    #PRIVATE src/common/sphere_library/CSRand.h
    PRIVATE src/common/sphere_library/CSString.h
    PRIVATE src/common/sphere_library/sstring.h
    PRIVATE src/common/sphere_library/sstringobjs.h
    PRIVATE src/common/CException.h
    PRIVATE src/common/CExpression.h
    #PRIVATE src/common/CLog.h
    #PRIVATE src/common/CServerMap.h
    PRIVATE src/game/CObjBase.h
    #PRIVATE src/game/CSector.h
    #PRIVATE src/game/CWorld.h
    PRIVATE src/sphere/threads.h
    #PRIVATE src/network/send.h
    #PRIVATE src/network/CSocket.h
    #PRIVATE src/network/receive.h
    PRIVATE src/game/CEntity.h
    PRIVATE src/game/CEntityProps.h
    PRIVATE src/game/CServerConfig.h
    #PRIVATE src/game/clients/CClient.h
    #PRIVATE src/game/chars/CChar.h
    #PRIVATE src/game/items/CItem.h
)

##############################
# Sphere Main
##############################
set(sphere_S
    src/sphere/asyncdb.cpp
    src/sphere/ConsoleInterface.cpp
    src/sphere/GlobalInitializer.cpp
    src/sphere/MainThread.cpp
    src/sphere/ProfileData.cpp
    src/sphere/ProfileTask.cpp
    src/sphere/StartupMonitorThread.cpp
    src/sphere/threads.cpp
    src/sphere/ntservice.cpp
    src/sphere/ntwindow.cpp
    src/sphere/UnixTerminal.cpp
)
set(sphere_H
    src/sphere/asyncdb.h
    src/sphere/containers.h
    src/sphere/ConsoleInterface.h
    src/sphere/GlobalInitializer.h
    src/sphere/MainThread.h
    src/sphere/ProfileData.h
    src/sphere/ProfileTask.h
    src/sphere/StartupMonitorAPI.h
    src/sphere/StartupMonitorThread.h
    src/sphere/threads.h
    src/sphere/ntservice.h
    src/sphere/ntwindow.h
    src/sphere/UnixTerminal.h
)
source_group(sphere FILES ${sphere_H} ${sphere_S})

##############################
# Network Management
##############################
set(network_S
    src/network/CClientIterator.cpp
    src/network/CIPHistoryManager.cpp
    src/network/CNetState.cpp
    src/network/CNetworkInput.cpp
    src/network/CNetworkManager.cpp
    src/network/CNetworkOutput.cpp
    src/network/CNetworkThread.cpp
    src/network/CPacketManager.cpp
    src/network/CSocket.cpp
    src/network/linuxev.cpp
    src/network/net_datatypes.cpp
    src/network/packet.cpp
    src/network/receive.cpp
    src/network/send.cpp
    src/network/PingServer.cpp
)
set(network_H
    src/network/CClientIterator.h
    src/network/CIPHistoryManager.h
    src/network/CNetState.h
    src/network/CNetworkInput.h
    src/network/CNetworkManager.h
    src/network/CNetworkOutput.h
    src/network/CNetworkThread.h
    src/network/CPacketManager.h
    src/network/CSocket.h
    src/network/linuxev.h
    src/network/net_datatypes.h
    src/network/packet.h
    src/network/receive.h
    src/network/send.h
    src/network/PingServer.h
)
source_group(network FILES ${network_H} ${network_S})

##############################
# UO Files
##############################
set(uofiles_S
    src/game/uo_files/CUOHuesRec.cpp
    src/game/uo_files/CUOIndexRec.cpp
    src/game/uo_files/CUOItemInfo.cpp
    src/game/uo_files/CUOMapList.cpp
    src/game/uo_files/CUOMapMeter.cpp
    src/game/uo_files/CUOMobtypes.cpp
    src/game/uo_files/CUOMultiItemRec.cpp
    src/game/uo_files/CUOStaticItemRec.cpp
    src/game/uo_files/CUOTerrainInfo.cpp
    src/game/uo_files/CUOTiledata.cpp
    src/game/uo_files/CUOVersionBlock.cpp
)
set(uofiles_H
    src/game/uo_files/CUOHuesRec.h
    src/game/uo_files/CUOIndexRec.h
    src/game/uo_files/CUOItemInfo.h
    src/game/uo_files/CUOItemTypeRec.h
    src/game/uo_files/CUOMapBlock.h
    src/game/uo_files/CUOMapList.h
    src/game/uo_files/CUOMapMeter.h
    src/game/uo_files/CUOMobtypes.h
    src/game/uo_files/CUOMultiItemRec.h
    src/game/uo_files/CUOStaticItemRec.h
    src/game/uo_files/CUOTerrainInfo.h
    src/game/uo_files/CUOTerrainTypeRec.h
    src/game/uo_files/CUOTiledata.h
    src/game/uo_files/CUOVersionBlock.h
    src/game/uo_files/uofiles_enums.h
    src/game/uo_files/uofiles_enums_itemid.h
    src/game/uo_files/uofiles_enums_creid.h
    src/game/uo_files/uofiles_macros.h
    src/game/uo_files/uofiles_types.h
)
source_group(game\\uo_files FILES ${uofiles_H} ${uofiles_S})

##############################
# Common
##############################
set(common_S
    src/common/sqlite/SQLite.cpp
    src/common/common.cpp
    src/common/CCacheableScriptFile.cpp
    src/common/CDataBase.cpp
    src/common/CException.cpp
    src/common/CExpression.cpp
    src/common/CFloatMath.cpp
    src/common/CLanguageID.cpp
    src/common/CLocalVarsExtra.cpp
    src/common/CLog.cpp
    src/common/CServerMap.cpp
    src/common/CUID.cpp
    src/common/CPointBase.cpp
    src/common/CRect.cpp
    src/common/CScript.cpp
    src/common/CScriptContexts.cpp
    src/common/CScriptObj.cpp
    src/common/CScriptParserBufs.cpp
    src/common/CScriptTriggerArgs.cpp
    src/common/CSFileObj.cpp
    src/common/CSFileObjContainer.cpp
    src/common/CSVFile.cpp
    src/common/CTextConsole.cpp
    src/common/CUOClientVersion.cpp
    src/common/CUOInstall.cpp
    src/common/CVarDefMap.cpp
    src/common/ListDefContMap.cpp
)
set(common_H
    src/common/sqlite/SQLite.h
    src/common/assertion.h
    src/common/basic_threading.h
    src/common/CCacheableScriptFile.h
    src/common/CDataBase.h
    src/common/CException.h
    src/common/CExpression.h
    src/common/CFloatMath.h
    src/common/CLanguageID.h
    src/common/CLocalVarsExtra.h
    src/common/CLog.h
    src/common/CServerMap.h
    src/common/CUID.h
    src/common/CPointBase.h
    src/common/CRect.h
    src/common/CScript.h
    src/common/CScriptContexts.h
    src/common/CScriptObj.h
    src/common/CScriptParserBufs.h
    src/common/CScriptTriggerArgs.h
    src/common/CSFileObj.h
    src/common/CSFileObjContainer.h
    src/common/CSVFile.h
    src/common/CTextConsole.h
    src/common/CUOClientVersion.h
    src/common/CUOInstall.h
    src/common/CVarDefMap.h
    src/common/datatypes.h
    src/common/ListDefContMap.h
    src/common/os_unix.h
    src/common/os_windows.h
    src/common/sphereproto.h
    src/common/sphereversion.h
    src/common/target_info.h
)
source_group(common FILES ${common_H} ${common_S})

##############################
# Windows CrashDump
##############################
set(crashdump_S
    src/common/crashdump/crashdump.cpp
)
set(crashdump_H
    src/common/crashdump/crashdump.h
    src/common/crashdump/mingwdbghelp.h
)
source_group(common\\crashdump FILES ${crashdump_H} ${crashdump_S})

##############################
# Login / Cryptography
##############################
set(crypto_S
    src/common/crypto/CBCrypt.cpp
    src/common/crypto/CCrypto.cpp
    src/common/crypto/CCryptoBlowFish.cpp
    src/common/crypto/CCryptoLogin.cpp
    src/common/crypto/CCryptoKeyCalc.cpp
    src/common/crypto/CCryptoMD5Interface.cpp
    src/common/crypto/CCryptoTwoFishInterface.cpp
    src/common/crypto/CHuffman.cpp
    src/common/crypto/CMD5.cpp
)
set(crypto_H
    src/common/crypto/CBCrypt.h
    src/common/crypto/CCrypto.h
    src/common/crypto/CCryptoKeyCalc.h
    src/common/crypto/CHuffman.h
    src/common/crypto/CMD5.h
    src/common/crypto/crypto_common.h
)
source_group(common\\crypto FILES ${crypto_H} ${crypto_S})

##############################
# Sphere Resources
##############################
set(resource_S
    src/common/resource/CResourceDef.cpp
    src/common/resource/CResourceHolder.cpp
    src/common/resource/CResourceID.cpp
    src/common/resource/CResourceHash.cpp
    src/common/resource/CResourceLink.cpp
    src/common/resource/CResourceLock.cpp
    src/common/resource/CResourceRef.cpp
    src/common/resource/CResourceScript.cpp
    src/common/resource/CResourceSortedArrays.cpp
    src/common/resource/CResourceQty.cpp
    src/common/resource/CValueDefs.cpp
)
set(resource_H
    src/common/resource/CResourceDef.h
    src/common/resource/CResourceHolder.h
    src/common/resource/CResourceID.h
    src/common/resource/CResourceHash.h
    src/common/resource/CResourceLink.h
    src/common/resource/CResourceLock.h
    src/common/resource/CResourceRef.h
    src/common/resource/CResourceScript.h
    src/common/resource/CResourceSortedArrays.h
    src/common/resource/CResourceQty.h
    src/common/resource/CValueDefs.h
)
source_group(common\\resource FILES ${resource_H} ${resource_S})

##############################
# Scripted resource sections
##############################
set(resourcesections_S
    src/common/resource/sections/CDialogDef.cpp
    src/common/resource/sections/CItemTypeDef.cpp
    src/common/resource/sections/CRandGroupDef.cpp
    src/common/resource/sections/CRegionResourceDef.cpp
    src/common/resource/sections/CResourceNamedDef.cpp
    src/common/resource/sections/CSkillClassDef.cpp
    src/common/resource/sections/CSkillDef.cpp
    src/common/resource/sections/CSpellDef.cpp
    src/common/resource/sections/CWebPageDef.cpp
)
set(resourcesections_H
    src/common/resource/sections/CDialogDef.h
    src/common/resource/sections/CItemTypeDef.h
    src/common/resource/sections/CRandGroupDef.h
    src/common/resource/sections/CRegionResourceDef.h
    src/common/resource/sections/CResourceNamedDef.h
    src/common/resource/sections/CSkillClassDef.h
    src/common/resource/sections/CSkillDef.h
    src/common/resource/sections/CSpellDef.h
    src/common/resource/sections/CWebPageDef.h
)
source_group(common\\resource\\sections FILES ${resourcesections_H} ${resourcesections_S})

##############################
# Sphere Library files
##############################
set(spherelibrary_S
    src/common/sphere_library/CSAssoc.cpp
    src/common/sphere_library/CSFile.cpp
    src/common/sphere_library/CSFileList.cpp
    src/common/sphere_library/CSFileText.cpp
    src/common/sphere_library/CSMemBlock.cpp
    src/common/sphere_library/CSObjCont.cpp
    src/common/sphere_library/CSObjList.cpp
    src/common/sphere_library/CSQueue.cpp
    src/common/sphere_library/CSRand.cpp
    src/common/sphere_library/CSString.cpp
    src/common/sphere_library/CSTime.cpp
    src/common/sphere_library/CSWindow.cpp
    src/common/sphere_library/smutex.cpp
    src/common/sphere_library/sresetevents.cpp
    src/common/sphere_library/sstring.cpp
    src/common/sphere_library/sstringobjs.cpp
    src/common/sphere_library/stypecast.cpp
)
set(spherelibrary_H
    src/common/sphere_library/CSAssoc.h
    src/common/sphere_library/CSFile.h
    src/common/sphere_library/CSFileList.h
    src/common/sphere_library/CSFileText.h
    src/common/sphere_library/CSMemBlock.h
    src/common/sphere_library/CSObjArray.h
    src/common/sphere_library/CSObjCont.h
    src/common/sphere_library/CSObjContRec.h
    src/common/sphere_library/CSObjList.h
    src/common/sphere_library/CSObjListRec.h
    src/common/sphere_library/CSObjSortArray.h
    src/common/sphere_library/CSPtrTypeArray.h
    src/common/sphere_library/CSReferenceCount.h
    src/common/sphere_library/CSTypedArray.h
    src/common/sphere_library/CSQueue.h
    src/common/sphere_library/CSRand.h
    src/common/sphere_library/CSString.h
    src/common/sphere_library/CSTime.h
    src/common/sphere_library/CSWindow.h
    src/common/sphere_library/scontainer_ops.h
    src/common/sphere_library/sfastmath.h
    src/common/sphere_library/smap.h
    src/common/sphere_library/smutex.h
    src/common/sphere_library/sobjpool.h
    src/common/sphere_library/squeues.h
    src/common/sphere_library/sresetevents.h
    src/common/sphere_library/ssorted_vector.h
    src/common/sphere_library/sptr.h
    src/common/sphere_library/sptr_containers.h
    src/common/sphere_library/sstacks.h
    src/common/sphere_library/sstring.h
    src/common/sphere_library/sstringobjs.h
    src/common/sphere_library/stypecast.h
)
source_group(common\\sphere_library FILES ${spherelibrary_H} ${spherelibrary_S})

##############################
# Main game-related classes
##############################
set(game_S
    src/game/CBase.cpp
    src/game/CContainer.cpp
    src/game/CComponent.cpp
    src/game/CComponentProps.cpp
    src/game/CEntity.cpp
    src/game/CEntityProps.cpp
    src/game/CObjBase.cpp
    src/game/CObjBaseTemplate.cpp
    src/game/CPathFinder.cpp
    src/game/CRegion.cpp
    src/game/CRegionBase.cpp
    src/game/CResourceCalc.cpp
    src/game/CSector.cpp
    src/game/CSectorEnviron.cpp
    src/game/CSectorTemplate.cpp
    src/game/CSectorList.cpp
    src/game/CServer.cpp
    src/game/CServerConfig.cpp
    src/game/CServerDef.cpp
    src/game/CServerTime.cpp
    src/game/CTeleport.cpp
    src/game/CTimedFunction.cpp
    src/game/CTimedFunctionHandler.cpp
    src/game/CTimedObject.cpp
    src/game/CWorld.cpp
    src/game/CWorldCache.cpp
    src/game/CWorldClock.cpp
    src/game/CWorldComm.cpp
    src/game/CWorldGameTime.cpp
    src/game/CWorldImport.cpp
    src/game/CWorldMap.cpp
    src/game/CWorldSearch.cpp
    src/game/CWorldTicker.cpp
    src/game/CWorldTickingList.cpp
    src/game/CWorldTimedFunctions.cpp
    src/game/spheresvr.cpp
    src/game/triggers.cpp
    src/game/CSectorEnviron.cpp
)
set(game_H
    src/game/CBase.h
    src/game/CContainer.h
    src/game/CComponent.h
    src/game/CComponentProps.h
    src/game/CEntity.h
    src/game/CEntityProps.h
    src/game/CObjBase.h
    src/game/CObjBaseTemplate.h
    src/game/CPathFinder.h
    src/game/CRegion.h
    src/game/CRegionBase.h
    src/game/CScriptProfiler.h
    src/game/CSector.h
    src/game/CSectorEnviron.h
    src/game/CSectorTemplate.h
    src/game/CSectorList.h
    src/game/CServer.h
    src/game/CServerConfig.h
    src/game/CServerDef.h
    src/game/CServerTime.h
    src/game/CStartLoc.h
    src/game/CTeleport.h
    src/game/CTimedFunction.h
    src/game/CTimedFunctionHandler.h
    src/game/CTimedObject.h
    src/game/CWorld.h
    src/game/CWorldCache.h
    src/game/CWorldClock.h
    src/game/CWorldComm.h
    src/game/CWorldGameTime.h
    src/game/CWorldMap.h
    src/game/CWorldSearch.h
    src/game/CWorldTicker.h
    src/game/CWorldTickingList.h
    src/game/CWorldTimedFunctions.h
    src/game/game_enums.h
    src/game/game_macros.h
    src/game/spheresvr.h
    src/game/triggers.h
)
source_group(game FILES ${game_H} ${game_S})

##############################
# Item entities
##############################
set(items_S
    src/game/items/CItem.cpp
    src/game/items/CItemBase.cpp
    src/game/items/CItemCommCrystal.cpp
    src/game/items/CItemContainer.cpp
    src/game/items/CItemCorpse.cpp
    src/game/items/CItemMap.cpp
    src/game/items/CItemMemory.cpp
    src/game/items/CItemMessage.cpp
    src/game/items/CItemMulti.cpp
    src/game/items/CItemMultiCustom.cpp
    src/game/items/CItemPlant.cpp
    src/game/items/CItemScript.cpp
    src/game/items/CItemShip.cpp
    src/game/items/CItemScript.cpp
    src/game/items/CItemStone.cpp
    src/game/items/CItemVendable.cpp
)
set(items_H
    src/game/items/CItem.h
    src/game/items/CItemBase.h
    src/game/items/CItemCommCrystal.h
    src/game/items/CItemContainer.h
    src/game/items/CItemCorpse.h
    src/game/items/CItemMap.h
    src/game/items/CItemMemory.h
    src/game/items/CItemMessage.h
    src/game/items/CItemMulti.h
    src/game/items/CItemMultiCustom.h
    src/game/items/CItemScript.h
    src/game/items/CItemShip.h
    src/game/items/CItemStone.h
    src/game/items/CItemVendable.h
    src/game/items/item_types.h
)
source_group(game\\items FILES ${items_H} ${items_S})

##############################
# Chars entities
##############################
set(chars_S
    src/game/chars/CCharAct.cpp
    src/game/chars/CCharBase.cpp
    src/game/chars/CChar.cpp
    src/game/chars/CCharAttacker.cpp
    src/game/chars/CCharFight.cpp
    src/game/chars/CCharLOS.cpp
    src/game/chars/CCharMemory.cpp
    src/game/chars/CCharNotoriety.cpp
    src/game/chars/CCharNPC.cpp
    src/game/chars/CCharNPCAct.cpp
    src/game/chars/CCharNPCAct_Fight.cpp
    src/game/chars/CCharNPCAct_Magic.cpp
    src/game/chars/CCharNPCAct_Vendor.cpp
    src/game/chars/CCharNPCPet.cpp
    src/game/chars/CCharNPCStatus.cpp
    src/game/chars/CCharPlayer.cpp
    src/game/chars/CCharRefArray.cpp
    src/game/chars/CCharSkill.cpp
    src/game/chars/CCharSpell.cpp
    src/game/chars/CCharStat.cpp
    src/game/chars/CCharStatus.cpp
    src/game/chars/CCharUse.cpp
    src/game/chars/CStoneMember.cpp
)
set(chars_H
    src/game/chars/CChar.h
    src/game/chars/CCharBase.h
    src/game/chars/CCharNPC.h
    src/game/chars/CCharPlayer.h
    src/game/chars/CCharRefArray.h
    src/game/chars/CStoneMember.h
)
source_group(game\\chars FILES ${chars_H} ${items_S})

##############################
# Clients
##############################
set(clients_S
    src/game/clients/CAccount.cpp
    src/game/clients/CChat.cpp
    src/game/clients/CChatChannel.cpp
    src/game/clients/CChatChanMember.cpp
    src/game/clients/CClient.cpp
    src/game/clients/CClientDialog.cpp
    src/game/clients/CClientEvent.cpp
    src/game/clients/CClientLog.cpp
    src/game/clients/CClientMsg.cpp
    src/game/clients/CClientMsg_AOSTooltip.cpp
    src/game/clients/CClientTarg.cpp
    src/game/clients/CClientTooltip.cpp
    src/game/clients/CClientUse.cpp
    src/game/clients/CGlobalChatChanMember.cpp
    src/game/clients/CGMPage.cpp
    src/game/clients/CParty.cpp
)
set(clients_H
    src/game/clients/CAccount.h
    src/game/clients/CChat.h
    src/game/clients/CChatChannel.h
    src/game/clients/CChatChanMember.h
    src/game/clients/CClient.h
    src/game/clients/CClientTooltip.h
    src/game/clients/CGlobalChatChanMember.h
    src/game/clients/CGMPage.h
    src/game/clients/CParty.h
)
source_group(game\\clients FILES ${clients_H} ${clients_S})

##############################
# ECS: components
##############################
set(components_S
    src/game/components/subcomponents/CFactionDef.cpp
    src/game/components/CCChampion.cpp
    src/game/components/CCItemDamageable.cpp
    src/game/components/CCMultiMovable.cpp
    src/game/components/CCPropsChar.cpp
    src/game/components/CCPropsItem.cpp
    src/game/components/CCPropsItemChar.cpp
    src/game/components/CCPropsItemEquippable.cpp
    src/game/components/CCPropsItemWeapon.cpp
    src/game/components/CCPropsItemWeaponRanged.cpp
    src/game/components/CCSpawn.cpp
)
set(components_H
    src/game/components/subcomponents/CFactionDef.h
    src/game/components/CCChampion.h
    src/game/components/CCItemDamageable.h
    src/game/components/CCMultiMovable.h
    src/game/components/CCPropsChar.h
    src/game/components/CCPropsItem.h
    src/game/components/CCPropsItemChar.h
    src/game/components/CCPropsItemEquippable.h
    src/game/components/CCPropsItemWeapon.h
    src/game/components/CCPropsItemWeaponRanged.h
    src/game/components/CCSpawn.h
)
source_group(game\\components FILES ${components_H} ${components_S})

##############################
# Table Definitions
##############################
set(tables_S
    src/tables/CBaseBaseDef_props.tbl
    src/tables/CChar_functions.tbl
    src/tables/CChar_props.tbl
    src/tables/CCharBase_props.tbl
    src/tables/CCharNpc_props.tbl
    src/tables/CCharPlayer_functions.tbl
    src/tables/CCharPlayer_props.tbl
    src/tables/CClient_functions.tbl
    src/tables/CClient_props.tbl
    src/tables/CCPropsChar_props.tbl
    src/tables/CCPropsItem_props.tbl
    src/tables/CCPropsItemChar_props.tbl
    src/tables/CCPropsItemEquippable_props.tbl
    src/tables/CCPropsItemWeapon_props.tbl
    src/tables/CCPropsItemWeaponRanged_props.tbl
    src/tables/CSFileObj_functions.tbl
    src/tables/CSFileObj_props.tbl
    src/tables/CSFileObjContainer_functions.tbl
    src/tables/CSFileObjContainer_props.tbl
    src/tables/CItem_functions.tbl
    src/tables/CItem_props.tbl
    src/tables/CItemBase_props.tbl
    src/tables/CItemStone_functions.tbl
    src/tables/CItemStone_props.tbl
    src/tables/classnames.tbl
    src/tables/CObjBase_functions.tbl
    src/tables/CObjBase_props.tbl
    src/tables/CParty_functions.tbl
    src/tables/CParty_props.tbl
    src/tables/CScriptObj_functions.tbl
    src/tables/CSector_functions.tbl
    src/tables/CStoneMember_functions.tbl
    src/tables/CStoneMember_props.tbl
    src/tables/defmessages.tbl
    src/tables/triggers.tbl
)
source_group(tables FILES ${tables_S})

##############################
# App Resources (Windows .rc)
##############################
set(app_resources_S
    src/resources/SphereSvr.rc
)

##############################
# Docs and Configs
##############################
set(docs_TEXT
    Changelog.txt
    src/sphere.ini
    src/sphereCrypt.ini
)


set(SPHERE_HEADERS
    ${game_H}
    ${items_H}
    ${chars_H}
    ${clients_H}
    ${components_H}
    ${uofiles_H}
    ${common_H}
    ${resource_H}
    ${resourcesections_H}
    ${network_H}
    ${crypto_H}
    ${sphere_H}
    ${crashdump_H}
    ${spherelibrary_H}
)

set(SPHERE_SOURCES
    ${game_S}
    ${items_S}
    ${chars_S}
    ${clients_S}
    ${components_S}
    ${uofiles_S}
    ${common_S}
    ${resource_S}
    ${resourcesections_S}
    ${network_S}
    ${crypto_S}
    ${sphere_S}
    ${crashdump_S}
    ${spherelibrary_S}
    ${tables_S}
    ${app_resources_S}
)
