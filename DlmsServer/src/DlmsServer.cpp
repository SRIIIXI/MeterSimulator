
#include <time.h>
#include "GXDLMSServerLN.h"
#include "../include/GXDLMSBase.h"
#include "../include/DlmsServer.h"
#include "GXDLMSAssociationLogicalName.h"
#include "GXDLMSAssociationShortName.h"

#include <malloc.h>

#include <algorithm>
#include <iomanip>
#include <set>
#include <sstream>
#include <vector>
#include <thread>
#include <functional>
#include <iostream>
#include <chrono>
#include <mutex>

CGXDLMSServerLN* LNServer = nullptr;

int main(int argc, char* argv[])
{
    InitDlms(argv[1]);
    return 0;
}

std::string BytesToHex(unsigned char* pBytes, int count, char addSpaces)
{
    const char hexArray[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
    std::string hexChars(addSpaces ? 3 * count : 2 * count, 0);
    int tmp;
    int index = 0;
    for (int pos = 0; pos != count; ++pos)
    {
        tmp = pBytes[pos] & 0xFF;
        hexChars[index++] = hexArray[tmp >> 4];
        hexChars[index++] = hexArray[tmp & 0x0F];
        if (addSpaces)
        {
            hexChars[index++] = ' ';
        }
    }
    //Remove last separator.
    if (addSpaces && count != 0)
    {
        hexChars.resize(hexChars.size() - 1);
    }
    return hexChars;
}

int InitDlms(const char* path)
{
    strncpy(DATAFILE, path, sizeof(DATAFILE));
    char* p = strrchr(DATAFILE, '\\');
    *p = '\0';
    strcpy(IMAGEFILE, DATAFILE);
    strcat(IMAGEFILE, "\\empty.bin");
    strcat(DATAFILE, "\\data.csv");

    int ret = 0;
    
    LNServer = new CGXDLMSServerLN(new CGXDLMSAssociationLogicalName(), new CGXDLMSIecHdlcSetup());

    if ((ret = LNServer->Init()) != 0)
    {
        return ret;
    }

    printf("-------------------------------------------------------\n");
    printf("System Title: %s\r\n", LNServer->GetCiphering()->GetSystemTitle().ToHexString().c_str());
    printf("Authentication key: %s\r\n", LNServer->GetCiphering()->GetAuthenticationKey().ToHexString().c_str());
    printf("Block cipher key: %s\r\n", LNServer->GetCiphering()->GetBlockCipherKey().ToHexString().c_str());
    printf("Master key (KEK) title: %s\r\n", LNServer->GetKek().ToHexString().c_str());
    printf("-------------------------------------------------------\n");

    printf("Press Ctrl + C to close application.\r\n");
    return 0;
}
