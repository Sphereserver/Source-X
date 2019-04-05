//
// Created by rot on 11/03/2016.
//

#include <vector>
#include "../common/sphereproto.h"
#include "../common/CLog.h"
#include "CServer.h"
#include "triggers.h"

//Trigger function start

struct T_TRIGGERS
{
    char	m_name[48];
    int		m_used;
};

std::vector<T_TRIGGERS> g_triggers;

bool IsTrigUsed(E_TRIGGERS id)
{
    if ( g_Serv.IsLoading() == true)
        return false;
    return (( (uint)id < g_triggers.size() ) && g_triggers[id].m_used );
}

bool IsTrigUsed(const char *name)
{
    if ( g_Serv.IsLoading() == true)
        return false;
    for ( auto it = g_triggers.begin(), end = g_triggers.end(); it != end; ++it )
    {
        if ( !strcmpi(it->m_name, name) )
            return (it->m_used != 0); // Returns true or false for known triggers
    }
    return true; //Must return true for custom triggers
}

void TriglistInit()
{
    T_TRIGGERS	trig;
    g_triggers.clear();

#define ADD(_a_)	strcpy(trig.m_name, "@"); strcat(trig.m_name, #_a_); trig.m_used = 0; g_triggers.emplace_back(trig);
#include "../tables/triggers.tbl"
#undef ADD
}

void TriglistClear()
{
    for ( auto it = g_triggers.begin(), end = g_triggers.end(); it != end; ++it )
    {
        it->m_used = 0;
    }
}

void TriglistAdd(E_TRIGGERS id)
{
    if ( g_triggers.size() )
        ++ g_triggers[id].m_used;
}

void TriglistAdd(const char *name)
{
    for ( auto it = g_triggers.begin(), end = g_triggers.end(); it != end; ++it )
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
    for ( auto it = g_triggers.begin(), end = g_triggers.end(); it != end; ++it )
    {
        ++total;
        if ( it->m_used )
            ++used;
    }
}

void TriglistPrint()
{
    for ( auto it = g_triggers.begin(), end = g_triggers.end(); it != end; ++it )
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
