/**
* @file triggers.h
*
*/

#ifndef _INC_TRIGGERS_H
#define _INC_TRIGGERS_H


//	Triggers list

#define TRIGGER_NAME_MAX_LEN    48

enum E_TRIGGERS
{
#define ADD(a) TRIGGER_##a,
#include "../tables/triggers.tbl"
    TRIGGER_QTY,
};

bool IsTrigUsed(E_TRIGGERS id);
bool IsTrigUsed(const char *name);
void TriglistInit();
void TriglistClear();
void TriglistAdd(E_TRIGGERS id);
void TriglistAdd(const char *name);
void Triglist(int &total, int &used);
void TriglistPrint();


#endif // _INC_TRIGGERS_H
