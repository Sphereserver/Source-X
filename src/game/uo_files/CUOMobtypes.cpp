/**
* @file CUOMobtypes.cpp
*
*/

#include "../../sphere/threads.h"
#include "../../common/CException.h"
#include "../../common/CExpression.h"
#include "../../common/CLog.h"
#include "../../common/CUOInstall.h"
#include "../../game/uo_files/uofiles_enums_creid.h"
#include "CUOMobtypes.h"

void CUOMobTypes::Load()
{
    ADDTOCALLSTACK("CUOMobTypes::Load");
    g_Log.Event(LOGM_INIT, "Caching mobtypes.txt...\n");

    CUOMobTypesType mobTypesRow = {0,0};

    _mobTypesEntries.clear();

    CSFileText csvMobTypes;

    if (g_Install.OpenFile(csvMobTypes, "mobtypes.txt", (word)(OF_READ | OF_TEXT | OF_DEFAULTMODE)))
    {
        _mobTypesEntries.resize(CREID_QTY);
        for (int i = 0; i < _mobTypesEntries.size(); i++)
        {
            mobTypesRow.m_type = 4;
            mobTypesRow.m_flags = 0;
            _mobTypesEntries[i] = mobTypesRow;
        }

        tchar* pszTemp = Str_GetTemp();
        size_t count = 0;
        while (!csvMobTypes.IsEOF())
        {
            csvMobTypes.ReadString(pszTemp, 200);
            if (*pszTemp)
            {
                count++;

                std::string tmpString = pszTemp;

                int len = (int)tmpString.length();
                len = Str_TrimEndWhitespace(tmpString.data(), len);

                if (len == 0 || tmpString[0] == '#') //Empty line or commented
                    continue;

                //Split the string
                std::vector<tchar*> splitArray;
                splitArray.resize(3);
                size_t iQty = Str_ParseCmds(tmpString.data(), splitArray.data(), 3, " \t#");

                if (splitArray.size() < 3)
                {
                    g_Log.EventError("Mobtypes.txt: not enough parameters on line %" PRIuSIZE_T " \n", count);
                    continue;
                }

                if (!IsStrNumeric(splitArray[0]))
                {
                    g_Log.EventError("Mobtypes.txt: non numeric ID on line %" PRIuSIZE_T " \n", count);
                    continue;
                }

                int animIndex = std::stoi(splitArray[0]);
                std::string sType = splitArray[1];
                if (sType == "MONSTER")
                    mobTypesRow.m_type = 0;
                else if (sType == "SEA_MONSTER")
                    mobTypesRow.m_type = 1;
                else if (sType == "ANIMAL")
                    mobTypesRow.m_type = 2;
                else if (sType == "HUMAN")
                    mobTypesRow.m_type = 3;
                else if (sType == "EQUIPMENT")
                    mobTypesRow.m_type = 4;
                else
                {
                    mobTypesRow.m_type = 0;
                    g_Log.EventError("Mobtypes.txt: wrong type found on line %" PRIuSIZE_T " \n", count);
                }

                mobTypesRow.m_flags = std::strtol(splitArray[2], NULL, 16);

                _mobTypesEntries[animIndex] = mobTypesRow;
            }
        }
        csvMobTypes.Close();
    }
}

