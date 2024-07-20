/**
* @file CUOMobtypes.cpp
*
*/

#include "../../sphere/threads.h"
#include "../../common/CException.h"
#include "../../common/CExpression.h"
#include "../../common/CLog.h"
#include "../../common/CUOInstall.h"
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
        _mobTypesEntries.resize(4096);
        for (int i = 0; i < 4096; i++)
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

                std::vector<std::string> splitArray;

                int len = (int)tmpString.length();
                len = Str_TrimEndWhitespace(tmpString.data(), len);

                if (len == 0 || tmpString[0] == '#') //Empty line or commented
                    continue;

                //Split the string
                char* pch;
                pch = strtok(tmpString.data(), " \t");
                while (pch != NULL)
                {
                    splitArray.push_back(pch);
                    pch = strtok(NULL, " \t");
                }

                if (splitArray.size() < 3)
                {
                    g_Log.EventError("Mobtypes.txt: not enough parameters on line %" PRIuSIZE_T " \n", count);
                    continue;
                }

                if (!IsStrNumeric(splitArray[0].data()))
                {
                    g_Log.EventError("Mobtypes.txt: non numeric ID on line %" PRIuSIZE_T " \n", count);
                    continue;
                }

                int animIndex = std::stoi(splitArray[0]);
                if (splitArray[1] == "MONSTER")
                    mobTypesRow.m_type = 0;
                else if (splitArray[1] == "SEA_MONSTER")
                    mobTypesRow.m_type = 1;
                else if (splitArray[1] == "ANIMAL")
                    mobTypesRow.m_type = 2;
                else if (splitArray[1] == "HUMAN")
                    mobTypesRow.m_type = 3;
                else if (splitArray[1] == "EQUIPMENT")
                    mobTypesRow.m_type = 4;
                else
                {
                    mobTypesRow.m_type = 0;
                    g_Log.EventError("Mobtypes.txt: wrong type found on line %" PRIuSIZE_T " \n", count);
                }

                //Comment check
                std::size_t posComment = splitArray[2].find('#');

                if (posComment == -1) //No comment
                {

                }
                else if (posComment > 0) //Comment after flags without a space or tab
                {
                    splitArray[2] = splitArray[2].substr(0, posComment - 1);
                }
                else if (posComment == 0)
                {
                    g_Log.EventError("Mobtypes.txt: comment instead of flags found on line %" PRIuSIZE_T " \n", count);
                    continue;
                }

                mobTypesRow.m_flags = std::strtol(splitArray[2].data(), NULL, 16);

                _mobTypesEntries[animIndex] = mobTypesRow;
            }
        }
        csvMobTypes.Close();
    }
}

