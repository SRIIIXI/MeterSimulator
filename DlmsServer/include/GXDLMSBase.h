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

#pragma once

#include "GXDLMSSecureServer.h"
#include "GXDLMSSecuritySetup.h"
#include "GXByteBuffer.h"
#include <queue>
#include <mutex>

extern char DATAFILE[FILENAME_MAX];
extern char IMAGEFILE[FILENAME_MAX];

class CGXDLMSBase : public CGXDLMSSecureServer
{
    int SendPush(CGXDLMSPushSetup* target);

private:
    int m_ServerSocket;
    pthread_t m_ReceiverThread;
    CGXDLMSAssociationLogicalName* m_ln;
    CGXDLMSAssociationShortName* m_sn;
    CGXDLMSTcpUdpSetup* m_wrapper;
    CGXDLMSIecHdlcSetup* m_hdlc;
public:
    GX_TRACE_LEVEL m_Trace;
    std::mutex m_mutex;

    /////////////////////////////////////////////////////////////////////////
    //Constructor.
    /////////////////////////////////////////////////////////////////////////
    CGXDLMSBase(
        CGXDLMSAssociationLogicalName* ln,
        CGXDLMSIecHdlcSetup* hdlc) :
        CGXDLMSSecureServer(ln, hdlc)
    {
        m_ServerSocket = -1;
        m_ReceiverThread = -1;
        m_ln = ln;
        m_sn = NULL;
        m_wrapper = NULL;
        m_hdlc = hdlc;
        SetMaxReceivePDUSize(1024);
        CGXDLMSSecuritySetup* s = new CGXDLMSSecuritySetup();
        s->SetServerSystemTitle(GetCiphering()->GetSystemTitle());
        GetItems().push_back(s);
    }

    /////////////////////////////////////////////////////////////////////////
    //Constructor.
    /////////////////////////////////////////////////////////////////////////
    CGXDLMSBase(
        CGXDLMSAssociationLogicalName* ln,
        CGXDLMSTcpUdpSetup* wrapper) :
        CGXDLMSSecureServer(ln, wrapper)
    {
        m_ServerSocket = -1;
        m_ReceiverThread = -1;
        m_ln = ln;
        m_sn = NULL;
        m_wrapper = wrapper;
        m_hdlc = NULL;
        SetMaxReceivePDUSize(1024);
    }

    /////////////////////////////////////////////////////////////////////////
    //Constructor.
    /////////////////////////////////////////////////////////////////////////
    CGXDLMSBase(
        CGXDLMSAssociationShortName* sn,
        CGXDLMSIecHdlcSetup* hdlc) :
        CGXDLMSSecureServer(sn, hdlc)
    {
        m_ServerSocket = -1;
        m_ReceiverThread = -1;
        m_ln = NULL;
        m_sn = sn;
        m_wrapper = NULL;
        m_hdlc = hdlc;
        SetMaxReceivePDUSize(1024);
    }

    /////////////////////////////////////////////////////////////////////////
    //Constructor.
    /////////////////////////////////////////////////////////////////////////
    CGXDLMSBase(
        CGXDLMSAssociationShortName* sn,
        CGXDLMSTcpUdpSetup* wrapper) :
        CGXDLMSSecureServer(sn, wrapper)
    {
        m_ServerSocket = -1;
        m_ReceiverThread = -1;
        m_ln = NULL;
        m_sn = sn;
        m_wrapper = wrapper;
        m_hdlc = NULL;
        SetMaxReceivePDUSize(1024);
    }

    /////////////////////////////////////////////////////////////////////////
    //Destructor.
    /////////////////////////////////////////////////////////////////////////
    ~CGXDLMSBase(void)
    {
        StopServer();
    }

    bool IsConnected();

    int GetSocket();

    int StartServer(int port);

    int StopServer();

    int Init(int port, GX_TRACE_LEVEL trace);
    int Init();
    void InitializeObjects();

    CGXDLMSObject* FindObject(
        DLMS_OBJECT_TYPE objectType,
        int sn,
        std::string& ln);

    void PreRead(
        std::vector<CGXDLMSValueEventArg*>& args);

    void PreWrite(
        std::vector<CGXDLMSValueEventArg*>& args);

    void PreAction(
        std::vector<CGXDLMSValueEventArg*>& args);

    void PostRead(
        std::vector<CGXDLMSValueEventArg*>& args);

    void PostWrite(
        std::vector<CGXDLMSValueEventArg*>& args);

    void PostAction(
        std::vector<CGXDLMSValueEventArg*>& args);

    bool IsTarget(
        unsigned long int serverAddress,
        unsigned long clientAddress);

    DLMS_SOURCE_DIAGNOSTIC ValidateAuthentication(
        DLMS_AUTHENTICATION authentication,
        CGXByteBuffer& password);

    /**
    * Get attribute access mode.
    *
    * @param arg
    *            Value event argument.
    * @return Access mode.
    * @throws Exception
    *             Server handler occurred exceptions.
    */
    DLMS_ACCESS_MODE GetAttributeAccess(CGXDLMSValueEventArg* arg);

    /**
    * Get method access mode.
    *
    * @param arg
    *            Value event argument.
    * @return Method access mode.
    * @throws Exception
    *             Server handler occurred exceptions.
    */
    DLMS_METHOD_ACCESS_MODE GetMethodAccess(CGXDLMSValueEventArg* arg);

    /**
    * Accepted connection is made for the server. All initialization is done
    * here.
    */
    void Connected(
        CGXDLMSConnectionEventArgs& connectionInfo);

    /**
     * Client has try to made invalid connection. Password is incorrect.
     *
     * @param connectionInfo
     *            Connection information.
     */
    void InvalidConnection(
        CGXDLMSConnectionEventArgs& connectionInfo);
    /**
     * Server has close the connection. All clean up is made here.
     */
    void Disconnected(
        CGXDLMSConnectionEventArgs& connectionInfo);

    /**
    * Get selected value(s). This is called when example profile generic
    * request current value.
    *
    * @param type
    *            Update type.
    * @param args
    *            Value event arguments.
    */
    void PreGet(
        std::vector<CGXDLMSValueEventArg*>& args);

    /**
    * Get selected value(s). This is called when example profile generic
    * request current value.
    *
    * @param type
    *            Update type.
    * @param args
    *            Value event arguments.
    */
    void PostGet(
        std::vector<CGXDLMSValueEventArg*>& args);
};
