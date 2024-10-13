
#include "../common/CLog.h"
#include "CServer.h"
#include "triggers.h"


static constexpr lpctstr kOrderedTrigsNames[] = {
    #define ADD(_a_) \
    "@" #_a_ ,

    #include "../tables/triggers.tbl"
    #undef ADD
};

struct TRIGGER_T_ID
{
    char	m_name[TRIGGER_NAME_MAX_LEN];
    int		m_used;
};
//struct TRIGGER_T_str
//{
//    int		m_used;
//};
static std::vector<TRIGGER_T_ID> sm_vTriggersId;


bool IsTrigUsed(E_TRIGGERS id)
{
    if ( g_Serv.IsLoading() == true)
        return false;

    return (( (uint)id < sm_vTriggersId.size() ) && sm_vTriggersId[id].m_used );
}

bool IsTrigUsed(const char *name)
{
    if ( g_Serv.IsLoading() == true)
        return false;

    const int index = FindTableSorted(name, kOrderedTrigsNames, ARRAY_COUNT(kOrderedTrigsNames));
    if (index >= 0)
        return IsTrigUsed((E_TRIGGERS)index);

    return true; //Must return true for custom triggers
}

void TriglistInit()
{
    TRIGGER_T_ID trig{};
    sm_vTriggersId.clear();

#define ADD(_a_) \
    snprintf(trig.m_name, TRIGGER_NAME_MAX_LEN, "@%s", #_a_); \
    trig.m_used = 0; \
    sm_vTriggersId.push_back(trig);

#include "../tables/triggers.tbl"
#undef ADD
}

void TriglistClear()
{
    for ( auto it = sm_vTriggersId.begin(), end = sm_vTriggersId.end(); it != end; ++it )
    {
        it->m_used = 0;
    }
}

void TriglistAdd(E_TRIGGERS id)
{
    if (sm_vTriggersId.size() )
        ++ sm_vTriggersId[id].m_used;
}

void TriglistAdd(const char *name)
{
    for ( auto it = sm_vTriggersId.begin(), end = sm_vTriggersId.end(); it != end; ++it )
    {
        if ( !strcmpi(it->m_name, name) )
        {
            ++ it->m_used;
            break;
        }
    }
}

void Triglist(int &total, int &used)
{
    total = used = 0;
    for ( auto it = sm_vTriggersId.cbegin(), end = sm_vTriggersId.cend(); it != end; ++it )
    {
        ++total;
        if ( it->m_used )
            ++used;
    }
}

void TriglistPrint()
{
    for ( auto it = sm_vTriggersId.cbegin(), end = sm_vTriggersId.cend(); it != end; ++it )
    {
        if (it->m_used)
        {
            g_Log.Event(LOGM_INIT, "Trigger %s : used %d time%s.\n", it->m_name, it->m_used, (it->m_used > 1) ? "s" : "");
        }
        else
        {
            g_Log.Event(LOGM_INIT, "Trigger %s : NOT used.\n", it->m_name);
        }
    }
}

//Trigger function end
