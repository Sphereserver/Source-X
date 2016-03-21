
#pragma once
#ifndef TRIGGERS_H
#define TRIGGERS_H


//	Triggers list
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


#endif // TRIGGERS_H
