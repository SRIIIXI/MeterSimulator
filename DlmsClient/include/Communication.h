//
// --------------------------------------------------------------------------
//  Gurux Ltd
//
//
//
// Filename:        $HeadURL$
//
// Version:         $Revision$,
//                  $Date$
//                  $Author$
//
// Copyright (c) Gurux Ltd
//
//---------------------------------------------------------------------------
//
//  DESCRIPTION
//
// This file is a part of Gurux Device Framework.
//
// Gurux Device Framework is Open Source software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2 of the License.
// Gurux Device Framework is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// More information of Gurux products: http://www.gurux.org
//
// This code is licensed under the GNU General Public License v2.
// Full text may be retrieved at http://www.gnu.org/licenses/gpl-2.0.txt
//---------------------------------------------------------------------------


#ifndef GXCOMMUNICATION_H
#define GXCOMMUNICATION_H

#include <stdio.h>

#define INVALID_HANDLE_VALUE -1
#include <unistd.h>
#include <stdlib.h>
#include <errno.h> //Add support for sockets
#include <netdb.h> //Add support for sockets
#include <sys/types.h> //Add support for sockets
#include <sys/socket.h> //Add support for sockets
#include <netinet/in.h> //Add support for sockets
#include <arpa/inet.h> //Add support for sockets
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "GXDLMSSecureClient.h"

class CGXCommunication
{
    GX_TRACE_LEVEL m_Trace;
    CGXDLMSSecureClient* m_Parser;
    static const unsigned int RECEIVE_BUFFER_SIZE = 200;
    unsigned char   m_Receivebuff[RECEIVE_BUFFER_SIZE];
    char* m_InvocationCounter;

    uint16_t m_WaitTime;
    int m_SocketFd; // Socket file descriptor for TCP connection
    int Read(unsigned char eop, CGXByteBuffer& reply);
    /// Read Invocation counter (frame counter) from the meter and update it.
    int UpdateFrameCounter();
public:
    void WriteValue(GX_TRACE_LEVEL trace, std::string line);
public:

    CGXCommunication(CGXDLMSSecureClient* pCosem, uint16_t wt, GX_TRACE_LEVEL trace, char* invocationCounter);
    ~CGXCommunication(void);

    //Disconnect from the meter.
    int Disconnect();
    //Release connection to the meter.
    int Release();

    int Close();

    //Open TCP connection.
    int Connect(const char* address, int port);

    //Get current time as a string.
    static void Now(std::string& str);

    //Send data to the media.
    int TransmitBuffer(CGXByteBuffer& data);
    //Read data from the media.
    int ReceiveBuffer(CGXByteBuffer& reply, std::string& str);

    int ReadDLMSPacket(CGXByteBuffer& data, CGXReplyData& reply);
    int ReadDataBlock(CGXByteBuffer& data, CGXReplyData& reply);
    int ReadDataBlock(std::vector<CGXByteBuffer>& data, CGXReplyData& reply);

    int InitializeConnection();
    int GetAssociationView();

    //Read selected object.
    int Read(CGXDLMSObject* pObject, int attributeIndex, std::string& value);

    //Read selected objects.
    int ReadList(std::vector<std::pair<CGXDLMSObject*, unsigned char> >& list);

    //Write selected objects.
    int WriteList(std::vector<std::pair<CGXDLMSObject*, unsigned char> >& list);

    //Write selected object.
    int Write(CGXDLMSObject* pObject, int attributeIndex, CGXDLMSVariant& value);

    //Write selected object.
    int Write(CGXDLMSObject* pObject, int attributeIndex);

    //Call action of selected object.
    int Method(CGXDLMSObject* pObject, int ActionIndex, CGXDLMSVariant& value);


    int ReadRowsByRange(CGXDLMSProfileGeneric* pObject, CGXDateTime& start, CGXDateTime& end, CGXDLMSVariant& rows);

    int ReadRowsByRange(CGXDLMSProfileGeneric* pObject, struct tm* start, struct tm* end, CGXDLMSVariant& rows);

    int ReadRowsByEntry(CGXDLMSProfileGeneric* pObject, unsigned int Index, unsigned int Count, CGXDLMSVariant& rows);

    int ReadScalerAndUnits();
    int GetProfileGenericColumns();

    int GetReadOut();
    int GetProfileGenerics();

    /*
    * Read all objects from the meter. This is only example. Usually there is
    * no need to read all data from the meter.
    */
    int ReadAll(const char* outputFile);

    //Read values using Access request.
    int ReadByAccess(std::vector<CGXDLMSAccessItem>& list);

};
#endif //GXCOMMUNICATION_H
