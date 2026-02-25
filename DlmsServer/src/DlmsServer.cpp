
#include <time.h>
#include "GXDLMSServerLN.h"
#include "../include/GXDLMSBase.h"
#include "../include/DlmsServer.h"
#include "GXDLMSAssociationLogicalName.h"
#include "GXDLMSAssociationShortName.h"

#include "Logger.h"
#include "SignalHandler.h"
#include "Configuration.h"

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
#include <unistd.h>

CGXDLMSServerLN* LNServer = nullptr;

// Signalhandler callback client.
class SignalHandlerClient : public SignalCallback
{
public:
    void suspend() override
    {
        writeLogNormal("Received suspend signal.");
    }

    void resume() override
    {
        writeLogNormal("Received resume signal.");
    }

    void shutdown() override
    {
        writeLogNormal("Received shutdown signal. Closing application.");
        printf("\nReceived shutdown signal. Closing application.\r\n");
        fflush(stdout);
        exit(0);
    }

    void alarm() override
    {
        writeLogNormal("Received alarm signal.");
    }

    void reset() override
    {
        writeLogNormal("Received reset signal.");
    }

    void childExit() override
    {
        writeLogNormal("Received child exit signal.");
    }

    void userdefined1() override
    {
        writeLogNormal("Received user defined 1 signal.");
    }

    void userdefined2() override
    {
        writeLogNormal("Received user defined 2 signal.");
    }
};

int main(int argc, char* argv[])
{
    SignalHandlerClient client;

    SignalHandler sgnHandler;
    sgnHandler.registerCallbackClient(&client);
    sgnHandler.registerSignalHandlers();

    Logger::GetInstance()->setModuleName(argv[0]);
    Logger::GetInstance()->setLogDirectory("./logs");   
    Logger::GetInstance()->setLogFileSize(10 * 1024 * 1024); //10 MB
    Logger::GetInstance()->startLogging(FileAppend);

    writeLogNormal("Starting DLMS server application.");

    //Now will create the configuration objects. It loads from default loaction. "/etc/" for root and "home/%USER%/.config" for non root users.
    Configuration config;
    config.setFileName("DlmsServer");
    config.loadConfiguration();

   std::filesystem::path datapath;

    if (geteuid() == 0)
    {
        // Running as root
        datapath = std::filesystem::path("/usr/share/DlmsServer");
    }
    else
    {
        // Running as normal user
        const char* home = getenv("HOME");
        if (!home)
        {
            return -1;
        }
        datapath = std::filesystem::path(home) / ".local" / std::string("DlmsServer");
    }

    strncpy(DATAFILE, datapath.c_str(), sizeof(DATAFILE));
    char* p = strrchr(DATAFILE, '/');
    *p = '\0';
    strcpy(IMAGEFILE, DATAFILE);
    strcat(IMAGEFILE, "/empty.bin");
    strcat(DATAFILE, "/data.csv");

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

    // Print all the above in ASCII as well for easier copy pasting.
    printf("System Title: %s\r\n", LNServer->GetCiphering()->GetSystemTitle().ToString().c_str());
    printf("Authentication key: %s\r\n", LNServer->GetCiphering()->GetAuthenticationKey().ToString().c_str());
    printf("Block cipher key: %s\r\n", LNServer->GetCiphering()->GetBlockCipherKey().ToString().c_str());
    printf("Master key (KEK) title: %s\r\n", LNServer->GetKek().ToString().c_str());
    fflush(stdout);

    printf("Press Ctrl + C to close application.\r\n");
    LNServer->StartServer(4059);
    
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

