#ifndef DLMS_SERVER
#define DLMS_SERVER

#include "GXEnums.h"
#include <string>

int InitDlms();
std::string BytesToHex(unsigned char* pBytes, int count, char addSpaces);

#endif
