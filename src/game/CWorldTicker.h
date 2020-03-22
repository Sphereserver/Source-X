/**
* @file CWorldTicker.h
*/

#ifndef _INC_CWORLDTICKER_H
#define _INC_CWORLDTICKER_H

#include "CTimedFunctionHandler.h"
#include "CTimedObject.h"
#include <map>
#include <unordered_map>
#include <unordered_set>


class CObjBase;
class CChar;
class CWorldClock;

class CWorldTicker
{
    friend class CWorldTickingList;

public:
    static const char* m_sClassName;
    CWorldTicker(CWorldClock* pClock);
    ~CWorldTicker() = default;

private:
    struct TimedObjectsContainer : public std::vector<CTimedObject*>
    {
        THREAD_CMUTEX_DEF;
    };
    struct WorldTickList : public std::map<int64, TimedObjectsContainer>
    {
        THREAD_CMUTEX_DEF;
    };
    struct TimedObjectLookupList : public std::unordered_map<CTimedObject*, int64>
    {
        THREAD_CMUTEX_DEF;
    };

    struct TimedCharsContainer : public std::vector<CChar*>
    {
        THREAD_CMUTEX_DEF;
    };
    struct CharTickList : public std::map<int64, TimedCharsContainer>
    {
        THREAD_CMUTEX_DEF;
    };
    struct CharTickLookupList : public std::unordered_map<CChar*, int64>
    {
        THREAD_CMUTEX_DEF;
    };

    struct StatusUpdatesList : public std::unordered_set<CObjBase*>
    {
        THREAD_CMUTEX_DEF;
    };

    WorldTickList _mWorldTickList;
    TimedObjectLookupList _mWorldTickLookup;
    CharTickList _mCharTickList;
    CharTickLookupList _mCharTickLookup;

    CWorldClock* _pWorldClock;
    int64        _iLastTickDone;

public:
    StatusUpdatesList _ObjStatusUpdates;   // objects that need OnTickStatusUpdate called
    CTimedFunctionHandler _TimedFunctions; // TimedFunction Container/Wrapper

public:
    void Tick();

    void AddTimedObject(int64 iTimeout, CTimedObject* pTimedObject);
    void DelTimedObject(CTimedObject* pTimedObject);
    void AddCharTicking(CChar* pChar, bool fIgnoreSleep, bool fOverwrite);
    void DelCharTicking(CChar* pChar);
private:
    void _InsertTimedObject(int64 iTimeout, CTimedObject* pTimedObject);
    void _RemoveTimedObject(const int64 iOldTimeout, const CTimedObject* pTimedObject);
    void _InsertCharTicking(int64 iTickNext, CChar* pChar);
    void _RemoveCharTicking(const int64 iOldTimeout, const CChar* pChar);
};

#endif // _INC_CWORLDTICKER_H
