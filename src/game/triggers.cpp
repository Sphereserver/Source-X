
#include "../common/CLog.h"
#include "CServer.h"
#include "triggers.h"

//Trigger function start

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
std::vector<TRIGGER_T_ID> g_triggers_id;


bool IsTrigUsed(E_TRIGGERS id)
{
    if ( g_Serv.IsLoading() == true)
        return false;

    return (( (uint)id < g_triggers_id.size() ) && g_triggers_id[id].m_used );
}

bool IsTrigUsed(const char *name)
{
    if ( g_Serv.IsLoading() == true)
        return false;

    int index = FindTableSorted(name, kOrderedTrigsNames, ARRAY_COUNT(kOrderedTrigsNames));
    if (index >= 0)
        return IsTrigUsed((E_TRIGGERS)index);
    return false;
}

void TriglistInit()
{
    TRIGGER_T_ID trig{};
    g_triggers_id.clear();

#define ADD(_a_) \
    snprintf(trig.m_name, TRIGGER_NAME_MAX_LEN, "@%s", #_a_); \
    trig.m_used = 0; \
    g_triggers_id.push_back(trig);

#include "../tables/triggers.tbl"
#undef ADD
}

void TriglistClear()
{
    for ( auto it = g_triggers_id.begin(), end = g_triggers_id.end(); it != end; ++it )
    {
        it->m_used = 0;
    }
}

void TriglistAdd(E_TRIGGERS id)
{
    if (g_triggers_id.size() )
        ++ g_triggers_id[id].m_used;
}

void TriglistAdd(const char *name)
{
    for ( auto it = g_triggers_id.begin(), end = g_triggers_id.end(); it != end; ++it )
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
    for ( auto it = g_triggers_id.cbegin(), end = g_triggers_id.cend(); it != end; ++it )
    {
        ++total;
        if ( it->m_used )
            ++used;
    }
}

void TriglistPrint()
{
    for ( auto it = g_triggers_id.cbegin(), end = g_triggers_id.cend(); it != end; ++it )
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
