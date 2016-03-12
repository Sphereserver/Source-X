//
// Created by rot on 11/03/2016.
//

#include <vector>
#include "graycom.h"
#include "Triggers.h"
#include "CServer.h"

//Trigger function start

struct T_TRIGGERS
{
    char	m_name[48];
    long	m_used;
};

std::vector<T_TRIGGERS> g_triggers;

bool IsTrigUsed(E_TRIGGERS id)
{
    if ( g_Serv.IsLoading() == true)
        return false;
    return (( static_cast<unsigned>(id) < g_triggers.size() ) && g_triggers[id].m_used );
}

bool IsTrigUsed(const char *name)
{
    if ( g_Serv.IsLoading() == true)
        return false;
    std::vector<T_TRIGGERS>::iterator it;
    for ( it = g_triggers.begin(); it != g_triggers.end(); ++it )
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

#define ADD(_a_)	strcpy(trig.m_name, "@"); strcat(trig.m_name, #_a_); trig.m_used = 0; g_triggers.push_back(trig);
#include "../tables/triggers.tbl"

}

void TriglistClear()
{
    std::vector<T_TRIGGERS>::iterator it;
    for ( it = g_triggers.begin(); it != g_triggers.end(); ++it )
    {
        (*it).m_used = 0;
    }
}

void TriglistAdd(E_TRIGGERS id)
{
    if ( g_triggers.size() )
        g_triggers[id].m_used++;
}

void TriglistAdd(const char *name)
{
    std::vector<T_TRIGGERS>::iterator it;
    for ( it = g_triggers.begin(); it != g_triggers.end(); ++it )
    {
        if ( !strcmpi(it->m_name, name) )
        {
            it->m_used++;
            break;
        }
    }
}

void Triglist(long &total, long &used)
{
    total = used = 0;
    std::vector<T_TRIGGERS>::iterator it;
    for ( it = g_triggers.begin(); it != g_triggers.end(); ++it )
    {
        total++;
        if ( it->m_used )
            used++;
    }
}

void TriglistPrint()
{
    std::vector<T_TRIGGERS>::iterator it;
    for ( it = g_triggers.begin(); it != g_triggers.end(); ++it )
    {
        if ( it->m_used )
        {
            g_Serv.SysMessagef("Trigger %s : used %ld time%s.\n", it->m_name, it->m_used, (it->m_used > 1) ? "s" : "");
        }
        else
        {
            g_Serv.SysMessagef("Trigger %s : NOT used.\n", it->m_name);
        }
    }
}

//Trigger function end
