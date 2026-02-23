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

#include "DlmsClient.h"

#include "Communication.h"
#include "GXDLMSConverter.h"
#include "GXDLMSProfileGeneric.h"
#include "GXDLMSDemandRegister.h"
#include "GXDLMSTranslator.h"
#include "GXDLMSData.h"

void CGXCommunication::WriteValue(GX_TRACE_LEVEL trace, std::string line)
{
    if (trace > GX_TRACE_LEVEL_WARNING)
    {
        printf("%s", line.c_str());
    }
    GXHelpers::Write("LogFile.txt", line);
}


CGXCommunication::CGXCommunication(CGXDLMSSecureClient* pParser, uint16_t wt, GX_TRACE_LEVEL trace, char* invocationCounter) :
    m_WaitTime(wt), m_Parser(pParser),
    m_Trace(trace), m_InvocationCounter(invocationCounter), m_SocketFd(INVALID_HANDLE_VALUE)
{
}

CGXCommunication::~CGXCommunication(void)
{
    Close();
}

//Connect to TCP server.
int CGXCommunication::Connect(const char* address, int port)
{
    struct sockaddr_in server_addr;
    int ret;

    // Create socket
    m_SocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_SocketFd < 0) {
        printf("Failed to create socket\n");
        return DLMS_ERROR_TYPE_COMMUNICATION_ERROR;
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, address, &server_addr.sin_addr) <= 0) {
        printf("Invalid address: %s\n", address);
        close(m_SocketFd);
        m_SocketFd = INVALID_HANDLE_VALUE;
        return DLMS_ERROR_TYPE_COMMUNICATION_ERROR;
    }

    // Connect to server
    if (connect(m_SocketFd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection failed to %s:%d\n", address, port);
        close(m_SocketFd);
        m_SocketFd = INVALID_HANDLE_VALUE;
        return DLMS_ERROR_TYPE_COMMUNICATION_ERROR;
    }

    return 0;
}

//Close connection to the meter.
int CGXCommunication::Disconnect()
{
    int ret;
    std::vector<CGXByteBuffer> data;
    CGXReplyData reply;
    if (m_SocketFd != INVALID_HANDLE_VALUE)
    {
        if ((ret = m_Parser->DisconnectRequest(data)) != 0 ||
            (ret = ReadDataBlock(data, reply)) != 0)
        {
            //Show error but continue close.
            printf("DisconnectRequest failed (%d) %s.\n", ret, CGXDLMSConverter::GetErrorMessage(ret));
        }
    }
    return 0;
}

//Release connection to the meter.
int CGXCommunication::Release()
{
    int ret;
    std::vector<CGXByteBuffer> data;
    CGXReplyData reply;
    if (m_SocketFd != INVALID_HANDLE_VALUE)
    {
        if ((ret = m_Parser->ReleaseRequest(data)) != 0 ||
            (ret = ReadDataBlock(data, reply)) != 0)
        {
            //Show error but continue close.
            printf("DisconnectRequest failed (%d) %s.\n", ret, CGXDLMSConverter::GetErrorMessage(ret));
        }
    }
    return 0;
}

//Close connection to the meter and close the communcation channel.
int CGXCommunication::Close()
{
    int ret;
    std::vector<CGXByteBuffer> data;
    CGXReplyData reply;

    if ((m_SocketFd != INVALID_HANDLE_VALUE) &&
        (m_Parser->GetInterfaceType() == DLMS_INTERFACE_TYPE_WRAPPER ||
            m_Parser->GetCiphering()->GetSecurity() != DLMS_SECURITY_NONE))
    {
        if ((ret = m_Parser->ReleaseRequest(data)) != 0 ||
            (ret = ReadDataBlock(data, reply)) != 0)
        {
            //Show error but continue close.
            printf("ReleaseRequest failed (%d) %s.\n", ret, CGXDLMSConverter::GetErrorMessage(ret));
        }
    }
    if (m_SocketFd != INVALID_HANDLE_VALUE )
    {
        if ((ret = m_Parser->DisconnectRequest(data)) != 0 ||
            (ret = ReadDataBlock(data, reply)) != 0)
        {
            //Show error but continue close.
            printf("DisconnectRequest failed (%d) %s.\n", ret, CGXDLMSConverter::GetErrorMessage(ret));
        }
    }
    // Close the socket
    if (m_SocketFd != INVALID_HANDLE_VALUE)
    {
        close(m_SocketFd);
        m_SocketFd = INVALID_HANDLE_VALUE;
    }

    return 0;
}

bool IsIPv6Address(const char* pAddress)
{
    return strstr(pAddress, ":") != NULL;
}

int CGXCommunication::Read(unsigned char eop, CGXByteBuffer& reply)
{
    int pos;
    bool bFound = false;
    int lastReadIndex = reply.GetPosition();
    
    do
    {
        ssize_t bytesRead = recv(m_SocketFd, m_Receivebuff, RECEIVE_BUFFER_SIZE, 0);
        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout or no data available
                usleep(100000); // Sleep 100ms
                continue;
            }
            return DLMS_ERROR_TYPE_COMMUNICATION_ERROR;
        }
        if (bytesRead == 0) {
            // Connection closed
            return DLMS_ERROR_CODE_RECEIVE_FAILED;
        }
        
        reply.Set(m_Receivebuff, bytesRead);
        
        if (reply.GetSize() > 5)
        {
            // Look for end of packet marker
            for (pos = reply.GetSize() - 1; pos != lastReadIndex; --pos)
            {
                if (reply.GetData()[pos] == eop)
                {
                    bFound = true;
                    break;
                }
            }
            lastReadIndex = pos;
        }
    } while (!bFound);
    
    return DLMS_ERROR_CODE_OK;
}

/// Read Invocation counter (frame counter) from the meter and update it.
int CGXCommunication::UpdateFrameCounter()
{
    int ret = 0;
    //Read frame counter if GeneralProtection is used.
    if (m_InvocationCounter != NULL && m_Parser->GetCiphering() != NULL && m_Parser->GetCiphering()->GetSecurity() != DLMS_SECURITY_NONE)
    {
        if (m_Trace > GX_TRACE_LEVEL_WARNING)
        {
            printf("UpdateFrameCounter\n");
        }
        m_Parser->SetProposedConformance((DLMS_CONFORMANCE)(m_Parser->GetProposedConformance() | DLMS_CONFORMANCE_GENERAL_PROTECTION));
        unsigned long add = m_Parser->GetClientAddress();
        DLMS_AUTHENTICATION auth = m_Parser->GetAuthentication();
        DLMS_SECURITY security = m_Parser->GetCiphering()->GetSecurity();
        CGXByteBuffer challenge = m_Parser->GetCtoSChallenge();
        std::vector<CGXByteBuffer> data;
        CGXReplyData reply;
        m_Parser->SetClientAddress(16);
        m_Parser->SetAuthentication(DLMS_AUTHENTICATION_NONE);
        m_Parser->GetCiphering()->SetSecurity(DLMS_SECURITY_NONE);
        //Get meter's send and receive buffers size.
        if ((ret = m_Parser->SNRMRequest(data)) != 0 ||
            (ret = ReadDataBlock(data, reply)) != 0 ||
            (ret = m_Parser->ParseUAResponse(reply.GetData())) != 0)
        {
            printf("SNRMRequest failed %d.\n", ret);
            return ret;
        }
        reply.Clear();
        if ((ret = m_Parser->AARQRequest(data)) != 0 ||
            (ret = ReadDataBlock(data, reply)) != 0 ||
            (ret = m_Parser->ParseAAREResponse(reply.GetData())) != 0)
        {
            if (ret == DLMS_ERROR_CODE_APPLICATION_CONTEXT_NAME_NOT_SUPPORTED)
            {
                printf("Use Logical Name referencing is wrong. Change it!\n");
                return ret;
            }
            printf("AARQRequest failed %s\n", CGXDLMSConverter::GetErrorMessage(ret));
            return ret;
        }
        reply.Clear();
        std::string ln;
        ln.append(m_InvocationCounter);
        CGXDLMSData d(ln);
        std::string str;
        if ((ret = Read(&d, 2, str)) == 0)
        {
            m_Parser->GetCiphering()->SetInvocationCounter(1 + d.GetValue().ToInteger());
        }
        printf("Invocation counter: %lu\n", m_Parser->GetCiphering()->GetInvocationCounter());
        reply.Clear();
        Disconnect();
        m_Parser->SetClientAddress(add);
        m_Parser->SetAuthentication(auth);
        m_Parser->GetCiphering()->SetSecurity(security);
        m_Parser->SetCtoSChallenge(challenge);
    }
    return ret;
}


//Initialize connection to the meter.
int CGXCommunication::InitializeConnection()
{
    int ret = 0;
    if ((ret = UpdateFrameCounter()) != 0)
    {
        return ret;
    }
    if (m_Trace > GX_TRACE_LEVEL_WARNING)
    {
        printf("InitializeConnection\n");
    }
    std::vector<CGXByteBuffer> data;
    CGXReplyData reply;

    ret = m_Parser->SNRMRequest(data);
    if(ret != 0)
    {
        printf("SNRMRequest failed %d.\n", ret);
        return ret;
    }

    ret = ReadDataBlock(data, reply);
    if(ret != 0)
    {
        printf("SNRMRequest failed %d at ReadDataBlock.\n", ret);
        return ret;
    }

    ret = m_Parser->ParseUAResponse(reply.GetData());    //Get meter's send and receive buffers size.
    if (ret != 0)
    {
        printf("SNRMRequest failed %d.\n", ret);
        return ret;
    }

    reply.Clear();

    if ((ret = m_Parser->AARQRequest(data)) != 0 ||
        (ret = ReadDataBlock(data, reply)) != 0 ||
        (ret = m_Parser->ParseAAREResponse(reply.GetData())) != 0)
    {
        if (ret == DLMS_ERROR_CODE_APPLICATION_CONTEXT_NAME_NOT_SUPPORTED)
        {
            printf("Use Logical Name referencing is wrong. Change it!\n");
            return ret;
        }
        printf("AARQRequest failed (%d) %s\n", ret, CGXDLMSConverter::GetErrorMessage(ret));
        return ret;
    }
    reply.Clear();
    // Get challenge Is HLS authentication is used.
    if (m_Parser->GetAuthentication() > DLMS_AUTHENTICATION_LOW)
    {
        if ((ret = m_Parser->GetApplicationAssociationRequest(data)) != 0 ||
            (ret = ReadDataBlock(data, reply)) != 0 ||
            (ret = m_Parser->ParseApplicationAssociationResponse(reply.GetData())) != 0)
        {
            printf("Authentication failed (%d) %s\n", ret, CGXDLMSConverter::GetErrorMessage(ret));
            return ret;
        }
    }
    return DLMS_ERROR_CODE_OK;
}

int CGXCommunication::TransmitBuffer(CGXByteBuffer& data)
{
    int ret;
    uint16_t len = (uint16_t)data.GetSize();

    if (m_SocketFd != INVALID_HANDLE_VALUE)
    {
        ssize_t sent = send(m_SocketFd, data.GetData(), len, 0);
        if (sent < 0) {
            printf("Send failed\n");
            return DLMS_ERROR_TYPE_COMMUNICATION_ERROR;
        }
        if (sent != len) {
            printf("Partial send: %zd/%d\n", sent, len);
            return DLMS_ERROR_TYPE_COMMUNICATION_ERROR;
        }
    }

    return 0;
}

int CGXCommunication::ReceiveBuffer(CGXByteBuffer& reply, std::string& str)
{
    int ret;
    uint16_t len = 0;

    if (m_SocketFd != INVALID_HANDLE_VALUE)
    {
        unsigned short pos = (unsigned short)reply.GetSize();
        if (ret = Read(0x7E, reply) != 0)
        {
            str += reply.ToHexString(pos, reply.GetSize() - pos, true);
            printf("Read failed.\n%s", str.c_str());
            return DLMS_ERROR_CODE_RECEIVE_FAILED;
        }
        if (str.size() == 0)
        {
            Now(str);
            str = "RX:\t" + str + "\t";
        }
        else
        {
            str += " ";
        }
        str += reply.ToHexString(pos, reply.GetSize() - pos, true);
    }

    return 0;
}

// Read DLMS Data frame from the device.
int CGXCommunication::ReadDLMSPacket(CGXByteBuffer& data, CGXReplyData& reply)
{
    int ret;
    CGXByteBuffer bb;
    std::string tmp;
    CGXReplyData notify;
    if (data.GetSize() == 0 && !reply.IsStreaming())
    {
        return DLMS_ERROR_CODE_OK;
    }
    Now(tmp);
    tmp = "TX:\t" + tmp;
    tmp += "\t" + data.ToHexString();
    if (m_Trace > GX_TRACE_LEVEL_INFO)
    {
        printf("%s\n", tmp.c_str());
    }
    GXHelpers::Write("trace.txt", tmp + "\n");
    if ((ret = TransmitBuffer(data)) != 0)
    {
        return ret;
    }
    // Loop until whole DLMS packet is received.
    tmp = "";
    unsigned char pos = 0;

    while(true)
    {
        if (notify.GetData().GetSize() != 0)
        {
            //Handle notify.
            if (!notify.IsMoreData())
            {
                //Show received push message as XML.
                std::string xml;
                CGXDLMSTranslator t(DLMS_TRANSLATOR_OUTPUT_TYPE_SIMPLE_XML);
                if ((ret = t.DataToXml(notify.GetData(), xml)) != 0)
                {
                    printf("ERROR! DataToXml failed.");
                }
                else
                {
                    printf("%s\n", xml.c_str());
                }
                notify.Clear();
            }
            continue;
        }

        ret = ReceiveBuffer(bb, tmp);

        if (ret != 0)
        {
            if (ret != DLMS_ERROR_CODE_RECEIVE_FAILED || pos == 3)
            {
                break;
            }
            ++pos;
            printf("Data send failed. Try to resend %d/3\n", pos);
            if ((ret = TransmitBuffer(data)) != 0)
            {
                break;
            }
        }

        ret = m_Parser->GetData(bb, reply, notify);

        if(ret == DLMS_ERROR_CODE_FALSE)
        {
            continue;
        }
        else
        {
            break;
        }
    }

    tmp += "\n";

    if (m_Trace > GX_TRACE_LEVEL_INFO)
    {
        printf("%s", tmp.c_str());
    }
    GXHelpers::Write("trace.txt", tmp);
    if (ret == DLMS_ERROR_CODE_REJECTED)
    {
        sleep(1000);
        ret = ReadDLMSPacket(data, reply);
    }
    return ret;
}

int CGXCommunication::ReadDataBlock(CGXByteBuffer& data, CGXReplyData& reply)
{
    //If ther is no data to send.
    if (data.GetSize() == 0)
    {
        return DLMS_ERROR_CODE_OK;
    }
    int ret;
    CGXByteBuffer bb;
    //Send data.
    if ((ret = ReadDLMSPacket(data, reply)) != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    while (reply.IsMoreData())
    {
        bb.Clear();
        if ((ret = m_Parser->ReceiverReady(reply.GetMoreData(), bb)) != 0)
        {
            return ret;
        }
        if ((ret = ReadDLMSPacket(bb, reply)) != DLMS_ERROR_CODE_OK)
        {
            return ret;
        }
    }
    return DLMS_ERROR_CODE_OK;
}


int CGXCommunication::ReadDataBlock(std::vector<CGXByteBuffer>& data, CGXReplyData& reply)
{
    //If ther is no data to send.
    if (data.size() == 0)
    {
        return DLMS_ERROR_CODE_OK;
    }
    int ret;
    CGXByteBuffer bb;
    //Send data.
    for (std::vector<CGXByteBuffer>::iterator it = data.begin(); it != data.end(); ++it)
    {
        reply.Clear();
        //Send data.
        if ((ret = ReadDLMSPacket(*it, reply)) != DLMS_ERROR_CODE_OK)
        {
            break;
        }
        while (reply.IsMoreData())
        {
            bb.Clear();
            if (!reply.IsStreaming())
            {
                if ((ret = m_Parser->ReceiverReady(reply.GetMoreData(), bb)) != 0)
                {
                    break;
                }
            }
            if ((ret = ReadDLMSPacket(bb, reply)) != DLMS_ERROR_CODE_OK)
            {
                break;
            }
        }
    }
    return ret;
}

//Get Association view.
int CGXCommunication::GetAssociationView()
{
    if (m_Trace > GX_TRACE_LEVEL_WARNING)
    {
        printf("GetAssociationView\n");
    }

    int ret;
    std::vector<CGXByteBuffer> data;
    CGXReplyData reply;

    if(m_Parser->GetObjectsRequest(data) != 0)
    {
        return -1;
    }

    if(ReadDataBlock(data, reply) != 0)
    {
        return -1;
    }

    if(m_Parser->ParseObjects(reply.GetData(), true) != 0)
    {
        return -1;
    }

    /*
    if ((ret = m_Parser->GetObjectsRequest(data)) != 0 ||
        (ret = ReadDataBlock(data, reply)) != 0 ||
        (ret = m_Parser->ParseObjects(reply.GetData(), true)) != 0)
    {
        printf("GetObjects failed %d.\n", ret);
        return ret;
    }
    */

    CGXDLMSConverter c;
    c.UpdateOBISCodeInformation(m_Parser->GetObjects());
    return DLMS_ERROR_CODE_OK;
}

//Read selected object.
int CGXCommunication::Read(CGXDLMSObject* pObject, int attributeIndex, std::string& value)
{
    value.clear();
    int ret;
    std::vector<CGXByteBuffer> data;
    CGXReplyData reply;
    //Read data from the meter.
    if ((ret = m_Parser->Read(pObject, attributeIndex, data)) != 0 ||
        (ret = ReadDataBlock(data, reply)) != 0 ||
        (ret = m_Parser->UpdateValue(*pObject, attributeIndex, reply.GetValue())) != 0)
    {
        return ret;
    }
    //Update data type.
    DLMS_DATA_TYPE type;
    if ((ret = pObject->GetDataType(attributeIndex, type)) != 0)
    {
        return ret;
    }
    if (type == DLMS_DATA_TYPE_NONE)
    {
        type = reply.GetValue().vt;
        if ((ret = pObject->SetDataType(attributeIndex, type)) != 0)
        {
            return ret;
        }
    }
    //Get read value as string.
    //Note! This is for example. It's faster if you handle read COSEM object directly.
    std::vector<std::string> values;
    pObject->GetValues(values);
    value = values[attributeIndex - 1];
    return DLMS_ERROR_CODE_OK;
}

int CGXCommunication::ReadList(
    std::vector<std::pair<CGXDLMSObject*, unsigned char> >& list)
{
    int ret;
    CGXReplyData reply;
    std::vector<CGXByteBuffer> data;
    if (list.size() == 0)
    {
        return 0;
    }
    //Get values from the meter.
    if ((ret = m_Parser->ReadList(list, data)) != 0)
    {
        return ret;
    }

    std::vector<CGXDLMSVariant> values;
    for (std::vector<CGXByteBuffer>::iterator it = data.begin(); it != data.end(); ++it)
    {
        if ((ret = ReadDataBlock(*it, reply)) != 0)
        {
            return ret;
        }
        if (reply.GetValue().vt == DLMS_DATA_TYPE_ARRAY)
        {
            values.insert(values.end(), reply.GetValue().Arr.begin(), reply.GetValue().Arr.end());
        }
        reply.Clear();
    }
    if (values.size() != list.size())
    {
        //Invalid reply. Read items count do not match.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return m_Parser->UpdateValues(list, values);
}

int CGXCommunication::WriteList(
    std::vector<std::pair<CGXDLMSObject*, unsigned char> >& list)
{
    int ret;
    CGXReplyData reply;
    std::vector<CGXByteBuffer> data;
    if (list.size() == 0)
    {
        return 0;
    }
    //Get values from the meter.
    if ((ret = m_Parser->WriteList(list, data)) != 0)
    {
        return ret;
    }
    std::vector<CGXDLMSVariant> values;
    for (std::vector<CGXByteBuffer>::iterator it = data.begin(); it != data.end(); ++it)
    {
        if ((ret = ReadDataBlock(*it, reply)) != 0)
        {
            break;
        }
        reply.Clear();
    }
    return ret;
}


//Write selected object.
int CGXCommunication::Write(CGXDLMSObject* pObject, int attributeIndex, CGXDLMSVariant& value)
{
    int ret;
    std::vector<CGXByteBuffer> data;
    CGXReplyData reply;
    //Get meter's send and receive buffers size.
    if ((ret = m_Parser->Write(pObject, attributeIndex, value, data)) != 0 ||
        (ret = ReadDataBlock(data, reply)) != 0)
    {
        return ret;
    }
    return DLMS_ERROR_CODE_OK;
}

//Write selected object.
int CGXCommunication::Write(CGXDLMSObject* pObject, int attributeIndex)
{
    int ret;
    std::vector<CGXByteBuffer> data;
    CGXReplyData reply;
    //Get meter's send and receive buffers size.
    if ((ret = m_Parser->Write(pObject, attributeIndex, data)) != 0 ||
        (ret = ReadDataBlock(data, reply)) != 0)
    {
        return ret;
    }
    return DLMS_ERROR_CODE_OK;
}

int CGXCommunication::Method(CGXDLMSObject* pObject, int attributeIndex, CGXDLMSVariant& value)
{
    int ret;
    std::vector<CGXByteBuffer> data;
    CGXReplyData reply;
    //Get meter's send and receive buffers size.
    if ((ret = m_Parser->Method(pObject, attributeIndex, value, data)) != 0 ||
        (ret = ReadDataBlock(data, reply)) != 0)
    {
        return ret;
    }
    return DLMS_ERROR_CODE_OK;
}

int CGXCommunication::ReadRowsByRange(
    CGXDLMSProfileGeneric* pObject,
    CGXDateTime& start,
    CGXDateTime& end,
    CGXDLMSVariant& rows)
{
    return ReadRowsByRange(pObject, &start.GetValue(), &end.GetValue(), rows);
}

int CGXCommunication::ReadRowsByRange(
    CGXDLMSProfileGeneric* pObject,
    struct tm* start,
    struct tm* end,
    CGXDLMSVariant& rows)
{
    rows.Clear();
    int ret;
    std::vector<CGXByteBuffer> data;
    CGXReplyData reply;
    if ((ret = m_Parser->ReadRowsByRange(pObject, start, end, data)) != 0 ||
        (ret = ReadDataBlock(data, reply)) != 0 ||
        (ret = m_Parser->UpdateValue(*pObject, 2, reply.GetValue())) != 0)
    {
        return ret;
    }
    //Get rows value as string.
    //Note! This is for example. It's faster if you handle read COSEM object directly.
    std::vector<std::string> values;
    pObject->GetValues(values);
    rows = values[2 - 1];
    return DLMS_ERROR_CODE_OK;
}

int CGXCommunication::ReadRowsByEntry(
    CGXDLMSProfileGeneric* pObject,
    unsigned int index,
    unsigned int count,
    CGXDLMSVariant& rows)
{
    rows.Clear();
    int ret;
    std::vector<CGXByteBuffer> data;
    CGXReplyData reply;
    if ((ret = m_Parser->ReadRowsByEntry(pObject, index, count, data)) != 0 ||
        (ret = ReadDataBlock(data, reply)) != 0 ||
        (ret = m_Parser->UpdateValue(*pObject, 2, reply.GetValue())) != 0)
    {
        return ret;
    }
    //Get rows value as string.
    //Note! This is for example. It's faster if you handle read COSEM object directly.
    std::vector<std::string> values;
    pObject->GetValues(values);
    rows = values[2 - 1];
    return DLMS_ERROR_CODE_OK;
}

int CGXCommunication::ReadScalerAndUnits()
{
    int ret = 0;
    std::string str;
    std::string ln;
    if ((m_Parser->GetNegotiatedConformance() & DLMS_CONFORMANCE_ACCESS) != 0)
    {
        std::vector<CGXDLMSAccessItem> list;
        // Read scalers and units from the device.
        for (std::vector<CGXDLMSObject*>::iterator it = m_Parser->GetObjects().begin(); it != m_Parser->GetObjects().end(); ++it)
        {
            if (((*it)->GetObjectType() == DLMS_OBJECT_TYPE_REGISTER ||
                (*it)->GetObjectType() == DLMS_OBJECT_TYPE_EXTENDED_REGISTER) &&
                ((*it)->GetAccess(3) & DLMS_ACCESS_MODE_READ) != 0)
            {
                list.push_back(CGXDLMSAccessItem(DLMS_ACCESS_SERVICE_COMMAND_TYPE_GET, *it, 3));
            }
            else if ((*it)->GetObjectType() == DLMS_OBJECT_TYPE_DEMAND_REGISTER &&
                ((*it)->GetAccess(4) & DLMS_ACCESS_MODE_READ) != 0)
            {
                list.push_back(CGXDLMSAccessItem(DLMS_ACCESS_SERVICE_COMMAND_TYPE_GET, *it, 4));
            }
        }
        if ((ret = ReadByAccess(list)) != 0)
        {
            printf("Err! Failed to read register: %s", CGXDLMSConverter::GetErrorMessage(ret));
            return ret;
        }
    }
    else if ((m_Parser->GetNegotiatedConformance() & DLMS_CONFORMANCE_MULTIPLE_REFERENCES) != 0)
    {
        std::vector<std::pair<CGXDLMSObject*, unsigned char> > list;
        // Read scalers and units from the device.
        for (std::vector<CGXDLMSObject*>::iterator it = m_Parser->GetObjects().begin(); it != m_Parser->GetObjects().end(); ++it)
        {
            if (((*it)->GetObjectType() == DLMS_OBJECT_TYPE_REGISTER ||
                (*it)->GetObjectType() == DLMS_OBJECT_TYPE_EXTENDED_REGISTER) &&
                ((*it)->GetAccess(3) & DLMS_ACCESS_MODE_READ) != 0)
            {
                list.push_back(std::make_pair(*it, 3));
            }
            else if ((*it)->GetObjectType() == DLMS_OBJECT_TYPE_DEMAND_REGISTER &&
                ((*it)->GetAccess(4) & DLMS_ACCESS_MODE_READ) != 0)
            {
                list.push_back(std::make_pair(*it, 4));
            }
        }
        if ((ret = ReadList(list)) != 0)
        {
            printf("Err! Failed to read register: %s", CGXDLMSConverter::GetErrorMessage(ret));
            m_Parser->SetNegotiatedConformance((DLMS_CONFORMANCE)(m_Parser->GetNegotiatedConformance() & ~DLMS_CONFORMANCE_MULTIPLE_REFERENCES));
        }
    }
    if ((m_Parser->GetNegotiatedConformance() & DLMS_CONFORMANCE_MULTIPLE_REFERENCES) == 0)
    {
        //If readlist is not supported read one value at the time.
        for (std::vector<CGXDLMSObject*>::iterator it = m_Parser->GetObjects().begin(); it != m_Parser->GetObjects().end(); ++it)
        {
            if ((*it)->GetObjectType() == DLMS_OBJECT_TYPE_REGISTER ||
                (*it)->GetObjectType() == DLMS_OBJECT_TYPE_EXTENDED_REGISTER)
            {
                (*it)->GetLogicalName(ln);
                WriteValue(m_Trace, ln.c_str());
                if ((ret = Read(*it, 3, str)) != 0)
                {
                    printf("Err! Failed to read register: %s %s", ln.c_str(), CGXDLMSConverter::GetErrorMessage(ret));
                    //Continue reading.
                    continue;
                }
            }
            else if ((*it)->GetObjectType() == DLMS_OBJECT_TYPE_DEMAND_REGISTER)
            {
                (*it)->GetLogicalName(ln);
                WriteValue(m_Trace, ln.c_str());
                if ((ret = Read(*it, 4, str)) != 0)
                {
                    printf("Err! Failed to read register: %s %s", ln.c_str(), CGXDLMSConverter::GetErrorMessage(ret));
                    //Continue reading.
                    continue;
                }
            }
        }
    }
    return ret;
}

int CGXCommunication::GetProfileGenericColumns()
{
    int ret = 0;
    std::string ln;
    std::string value;
    //Read columns.
    CGXDLMSObjectCollection profileGenerics;
    m_Parser->GetObjects().GetObjects(DLMS_OBJECT_TYPE_PROFILE_GENERIC, profileGenerics);
    for (std::vector<CGXDLMSObject*>::iterator it = profileGenerics.begin(); it != profileGenerics.end(); ++it)
    {
        //Read Profile Generic columns first.
        CGXDLMSProfileGeneric* pg = (CGXDLMSProfileGeneric*)*it;
        if ((ret = Read(pg, 3, value)) != 0)
        {
            printf("Err! Failed to read columns: %s", CGXDLMSConverter::GetErrorMessage(ret));
            //Continue reading.
            continue;
        }

        //Update columns scalers.
        DLMS_OBJECT_TYPE ot;
        CGXDLMSObject* obj;
        for (std::vector<std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*> >::iterator it2 = pg->GetCaptureObjects().begin(); it2 != pg->GetCaptureObjects().end(); ++it2)
        {
            ot = it2->first->GetObjectType();
            if (ot == DLMS_OBJECT_TYPE_REGISTER ||
                ot == DLMS_OBJECT_TYPE_EXTENDED_REGISTER ||
                ot == DLMS_OBJECT_TYPE_DEMAND_REGISTER)
            {
                it2->first->GetLogicalName(ln);
                obj = m_Parser->GetObjects().FindByLN(ot, ln);
                if (obj != NULL)
                {
                    if (ot == DLMS_OBJECT_TYPE_REGISTER || ot == DLMS_OBJECT_TYPE_EXTENDED_REGISTER)
                    {
                        ((CGXDLMSRegister*)it2->first)->SetScaler(((CGXDLMSRegister*)obj)->GetScaler());
                        ((CGXDLMSRegister*)it2->first)->SetUnit(((CGXDLMSRegister*)obj)->GetUnit());
                    }
                    else if (ot == DLMS_OBJECT_TYPE_DEMAND_REGISTER)
                    {
                        ((CGXDLMSDemandRegister*)it2->first)->SetScaler(((CGXDLMSDemandRegister*)obj)->GetScaler());
                        ((CGXDLMSDemandRegister*)it2->first)->SetUnit(((CGXDLMSDemandRegister*)obj)->GetUnit());
                    }
                }
            }
        }
        WriteValue(m_Trace, "Profile Generic " + (*it)->GetName().ToString() + " Columns:\n");
        std::string str;
        for (std::vector<std::pair<CGXDLMSObject*, CGXDLMSCaptureObject*> >::iterator it2 = pg->GetCaptureObjects().begin(); it2 != pg->GetCaptureObjects().end(); ++it2)
        {
            if (str.size() != 0)
            {
                str.append(" | ");
            }
            str.append((*it2).first->GetName().ToString());
            str.append(" ");
            str.append((*it2).first->GetDescription());
        }
        str.append("\n");
        WriteValue(m_Trace, str);
    }
    return ret;
}

int CGXCommunication::GetReadOut()
{
    int ret = 0;
    char buff[200] = { 0 };
    std::string value;
    for (std::vector<CGXDLMSObject*>::iterator it = m_Parser->GetObjects().begin(); it != m_Parser->GetObjects().end(); ++it)
    {
        // Profile generics are read later because they are special cases.
        // (There might be so lots of data and we so not want waste time to read all the data.)
        if ((*it)->GetObjectType() == DLMS_OBJECT_TYPE_PROFILE_GENERIC)
        {
            continue;
        }
        if (dynamic_cast<CGXDLMSCustomObject*>((*it)) != NULL)
        {
            //If interface is not implemented.
            //Example manufacturer specific interface.
            printf("Unknown Interface: %d\n", (*it)->GetObjectType());
            continue;
        }
        sprintf(buff, "-------- Reading %s %s %s\n", CGXDLMSClient::ObjectTypeToString((*it)->GetObjectType()).c_str(), (*it)->GetName().ToString().c_str(), (*it)->GetDescription().c_str());

        WriteValue(m_Trace, buff);
        std::vector<int> attributes;
        (*it)->GetAttributeIndexToRead(true, attributes);
        for (std::vector<int>::iterator pos = attributes.begin(); pos != attributes.end(); ++pos)
        {
            value.clear();
            if (((*it)->GetAccess(*pos) & DLMS_ACCESS_MODE_READ) != 0)
            {
                if ((ret = Read(*it, *pos, value)) != DLMS_ERROR_CODE_OK)
                {
                    sprintf(buff, "Error! Index: %d %s\n", *pos, CGXDLMSConverter::GetErrorMessage(ret));
                    WriteValue(GX_TRACE_LEVEL_ERROR, buff);
                    //Continue reading.
                }
                else
                {
                    sprintf(buff, "Index: %d Value: ", *pos);
                    WriteValue(m_Trace, buff);
                    WriteValue(m_Trace, value.c_str());
                    WriteValue(m_Trace, "\n");
                }
            }
            else
            {
                WriteValue(GX_TRACE_LEVEL_INFO, "Attribute" + GXHelpers::IntToString(*pos) + " is not readable.");
            }
        }
    }
    return ret;
}

int CGXCommunication::GetProfileGenerics()
{
    char buff[200];
    int ret = 0;
    std::string str;
    std::string value;
    //Find profile generics and read them.
    CGXDLMSObjectCollection pgs;
    m_Parser->GetObjects().GetObjects(DLMS_OBJECT_TYPE_PROFILE_GENERIC, pgs);
    for (std::vector<CGXDLMSObject*>::iterator it = pgs.begin(); it != pgs.end(); ++it)
    {
        sprintf(buff, "-------- Reading %s %s %s\n", CGXDLMSClient::ObjectTypeToString((*it)->GetObjectType()).c_str(), (*it)->GetName().ToString().c_str(), (*it)->GetDescription().c_str());
        WriteValue(m_Trace, buff);

        if ((ret = Read(*it, 7, value)) != DLMS_ERROR_CODE_OK)
        {
            sprintf(buff, "Error! Index: %d: %s\n", 7, CGXDLMSConverter::GetErrorMessage(ret));
            WriteValue(GX_TRACE_LEVEL_ERROR, buff);
            //Continue reading.
        }

        std::string entriesInUse = value;
        if ((ret = Read(*it, 8, value)) != DLMS_ERROR_CODE_OK)
        {
            sprintf(buff, "Error! Index: %d: %s\n", 8, CGXDLMSConverter::GetErrorMessage(ret));
            WriteValue(GX_TRACE_LEVEL_ERROR, buff);
            //Continue reading.
        }
        std::string entries = value;
        str = "Entries: ";
        str += entriesInUse;
        str += "/";
        str += entries;
        str += "\n";
        WriteValue(m_Trace, str);
        //If there are no columns or rows.
        if (((CGXDLMSProfileGeneric*)*it)->GetEntriesInUse() == 0 || ((CGXDLMSProfileGeneric*)*it)->GetCaptureObjects().size() == 0)
        {
            continue;
        }
        //All meters are not supporting parameterized read.
        CGXDLMSVariant rows;
        if ((m_Parser->GetNegotiatedConformance() & (DLMS_CONFORMANCE_PARAMETERIZED_ACCESS | DLMS_CONFORMANCE_SELECTIVE_ACCESS)) != 0)
        {
            //Read first row from Profile Generic.
            if ((ret = ReadRowsByEntry((CGXDLMSProfileGeneric*)*it, 1, 1, rows)) != 0)
            {
                str = "Error! Failed to read first row:";
                str += CGXDLMSConverter::GetErrorMessage(ret);
                str += "\n";
                WriteValue(GX_TRACE_LEVEL_ERROR, str);
                //Continue reading.
            }
            else
            {
                //////////////////////////////////////////////////////////////////////////////
                //Show rows.
                WriteValue(m_Trace, rows.ToString());
            }
        }

        //All meters are not supporting parameterized read.
        if ((m_Parser->GetNegotiatedConformance() & (DLMS_CONFORMANCE_PARAMETERIZED_ACCESS | DLMS_CONFORMANCE_SELECTIVE_ACCESS)) != 0)
        {
            CGXDateTime start = CGXDateTime::Now();
            start.ResetTime();
            CGXDateTime end = CGXDateTime::Now();
            if ((ret = ReadRowsByRange((CGXDLMSProfileGeneric*)(*it), start, end, rows)) != 0)
            {
                str = "Error! Failed to read last day:";
                str += CGXDLMSConverter::GetErrorMessage(ret);
                str += "\n";
                WriteValue(GX_TRACE_LEVEL_ERROR, str);
                //Continue reading.
            }
            else
            {
                //////////////////////////////////////////////////////////////////////////////
                //Show rows.
                WriteValue(m_Trace, rows.ToString());
            }
        }
    }
    return ret;
}

int CGXCommunication::ReadAll(const char* outputFile)
{
    bool read = false;
    int ret;
    if ((ret = InitializeConnection()) == 0)
    {
        if (!read)
        {
            // Get list of objects that meter supports.
            if ((ret = GetAssociationView()) != 0)
            {
                printf("GetAssociationView failed (%d) %s\n", ret, CGXDLMSConverter::GetErrorMessage(ret));
            }
            // Read Scalers and units from the register objects.
            if (ret == 0 && (ret = ReadScalerAndUnits()) != 0)
            {
                printf("ReadScalerAndUnits failed (%d) %s\n", ret, CGXDLMSConverter::GetErrorMessage(ret));
            }
            // Read Profile Generic columns.
            if (ret == 0 && (ret = GetProfileGenericColumns()) != 0)
            {
                printf("GetProfileGenericColumns failed (%d) %s\n", ret, CGXDLMSConverter::GetErrorMessage(ret));
            }
        }
        if (ret == 0 && (ret = GetReadOut()) != 0)
        {
            printf("GetReadOut failed (%d) %s\n", ret, CGXDLMSConverter::GetErrorMessage(ret));
        }
        // Read historical data.
        if (ret == 0 && (ret = GetProfileGenerics()) != 0)
        {
            printf("GetProfileGenerics failed (%d) %s\n", ret, CGXDLMSConverter::GetErrorMessage(ret));
        }
    }
    Close();

    return ret;
}

int CGXCommunication::ReadByAccess(std::vector<CGXDLMSAccessItem>& list)
{
    int ret;
    CGXReplyData reply;
    std::vector<CGXByteBuffer> data;
    if (list.size() == 0)
    {
        return 0;
    }
    //Get values from the meter.
    if ((ret = m_Parser->AccessRequest(NULL, list, data)) != 0)
    {
        return ret;
    }

    for (std::vector<CGXByteBuffer>::iterator it = data.begin(); it != data.end(); ++it)
    {
        reply.Clear();
        if ((ret = ReadDataBlock(*it, reply)) != 0)
        {
            return ret;
        }
    }
    return m_Parser->ParseAccessResponse(list, reply.GetData());
}

void CGXCommunication::Now(std::string& str)
{
    time_t tm1 = time(NULL);
    struct tm dt;
    char tmp[10];
    int ret;
    localtime_r(&tm1, &dt);
    ret = sprintf(tmp, "%.2d:%.2d:%.2d", dt.tm_hour, dt.tm_min, dt.tm_sec);
    str.append(tmp, ret);
}