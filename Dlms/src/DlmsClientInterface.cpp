#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>

#include <map>

#include "DlmsClientInterface.h"
#include "GXDLMSSecureClient.h"
#include "GXDLMSTranslator.h"

#ifndef NULL
#define NULL 0
#endif

int DLMS_CLIENT_AUTHENTICATION_HIGH_GMAC = 5;
int DLMS_CLIENT_AUTHENTICATION_NONE = 0;
int DLMS_CLIENT_INTERFACE_TYPE_HDLC = 0;
int DLMS_CLIENT_SECURITY_AUTHENTICATION_ENCRYPTION = 48;
int DLMS_CLIENT_SECURITY_AUTHENTICATION_NONE = 0;

char information_buffer[33] = { 0 };

class DlmsClientInterface
{
public:
	DlmsClientInterface()
	{
		client = NULL;
		password = NULL;
		systemtitle = NULL;
		autheticationkey = NULL;
		blockcipherket = NULL;
		dedicatedkey = NULL;

		autoincreaseid = true;
		invocationcounter = NULL;
		gbtwindowsize = 1;
		windowsize = 1;
		maxinfo = 128;
		manufacturerid = NULL;
	}

	~DlmsClientInterface()
	{

	}

	CGXDLMSSecureClient* client;

	char* password;
	char* systemtitle;
	char* autheticationkey;
	char* blockcipherket;
	char* dedicatedkey;

	bool autoincreaseid;
	char* invocationcounter;
	unsigned char gbtwindowsize;
	unsigned char windowsize;
	uint16_t maxinfo;
	char* manufacturerid;

};

static std::map<int, DlmsClientInterface*> clientMap;

char* dlms_client_system_information(void)
{
	memset(information_buffer, 0, 33);
	sprintf(information_buffer, "EDMI.DLMS.HDLC.BLE");
	return &information_buffer[0];
}

bool dlms_client_create(int clientid, int authenticationtype, int interfacetype, int securitytype, bool uselogicalnaming, int serveraddress, int clientaddress, const char* password, const char* systemtitle, const char* autheticationkey, const char* blockcipherkey, const char* dedicatedkey)
{
	DlmsClientInterface* clientstruct = new DlmsClientInterface();

	if (!clientstruct)
	{
		return false;
	}

	clientstruct->client = new CGXDLMSSecureClient(uselogicalnaming, clientaddress, serveraddress, (DLMS_AUTHENTICATION)authenticationtype, password, (DLMS_INTERFACE_TYPE)interfacetype);

	if (!clientstruct->client)
	{
		return false;
	}

	clientstruct->client->GetCiphering()->SetSecurity((DLMS_SECURITY)securitytype);
	CGXByteBuffer bb;

	if (systemtitle != NULL)
	{
		bb.Clear();
		bb.SetHexString(systemtitle);
		clientstruct->client->GetCiphering()->SetSystemTitle(bb);
	}

	if (autheticationkey != NULL)
	{
		bb.Clear();
		bb.SetHexString(autheticationkey);
		clientstruct->client->GetCiphering()->SetAuthenticationKey(bb);
	}

	if (blockcipherkey != NULL)
	{
		bb.Clear();
		bb.SetHexString(blockcipherkey);
		clientstruct->client->GetCiphering()->SetBlockCipherKey(bb);
	}

	if (dedicatedkey != NULL)
	{
		bb.Clear();
		bb.SetHexString(dedicatedkey);
		clientstruct->client->GetCiphering()->SetDedicatedKey(bb);
	}

	clientMap[clientid] = clientstruct;

	return true;
}

bool dlms_client_set_session_parameters(int clientid, bool autoincrease_id, const char* invocation_counter, unsigned char gbt_window_size, unsigned char window_size, uint16_t max_info, char* manufacturer_id)
{
	DlmsClientInterface* clientstruct = clientMap[clientid];

	if (!clientstruct)
	{
		return false;
	}

	if (!clientstruct->client)
	{
		return false;
	}

	clientstruct->client->SetAutoIncreaseInvokeID(autoincrease_id);
	clientstruct->client->SetGbtWindowSize(gbt_window_size);
	clientstruct->client->GetHdlcSettings().SetWindowSizeRX(window_size);
	clientstruct->client->GetHdlcSettings().SetWindowSizeTX(window_size);
	clientstruct->client->GetHdlcSettings().SetMaxInfoRX(max_info);
	clientstruct->client->GetHdlcSettings().SetMaxInfoTX(max_info);
	clientstruct->client->GetHdlcSettings().SetMaxInfoTX(max_info);
	clientstruct->client->SetManufacturerId(manufacturer_id);

	clientstruct->autoincreaseid = autoincrease_id;
	clientstruct->gbtwindowsize = gbt_window_size;
	clientstruct->windowsize = window_size;
	clientstruct->maxinfo = max_info;

	if (invocation_counter)
	{
		clientstruct->invocationcounter = (char*)calloc(1, strlen(invocation_counter) + 1);
		if (clientstruct->invocationcounter)
		{
			strcpy(clientstruct->invocationcounter, invocation_counter);
		}
	}

	if (manufacturer_id)
	{
		clientstruct->manufacturerid = (char*)calloc(1, strlen(manufacturer_id) + 1);
		if (clientstruct->manufacturerid)
		{
			strcpy(clientstruct->manufacturerid, manufacturer_id);
		}
	}

	return true;
}

bool dlms_client_destroy(int clientid)
{
	DlmsClientInterface* clientstruct = clientMap[clientid];

	if (!clientstruct)
	{
		return false;
	}

	if (!clientstruct->client)
	{
		return false;
	}

	return true;
}

bool dlms_client_release_buffer(int clientid)
{
	DlmsClientInterface* clientstruct = clientMap[clientid];

	if (!clientstruct)
	{
		return false;
	}

	if (!clientstruct->client)
	{
		return false;
	}

	return true;
}

unsigned char* dlms_client_get_challenge(int clientid, int* len)
{
	DlmsClientInterface* clientstruct = clientMap[clientid];

	if (!clientstruct)
	{
		return NULL;
	}

	if (!clientstruct->client)
	{
		return NULL;
	}

	CGXByteBuffer challenge = clientstruct->client->GetCtoSChallenge();

	if (challenge.GetSize() > 0)
	{
		unsigned char* buffer = (unsigned char*)calloc(1, challenge.GetSize());
		if (buffer)
		{
			memcpy(buffer, challenge.GetData(), challenge.GetSize());
			return buffer;
		}
	}

	return NULL;
}

void dlms_client_set_security(int clientid, int clientaddress, int authenticationtype, int securitytype, unsigned char* challenge_buffer, int challenge_len)
{
	DlmsClientInterface* clientstruct = clientMap[clientid];

	if (!clientstruct)
	{
		return;
	}

	if (!clientstruct->client)
	{
		return;
	}

	if (challenge_buffer)
	{
		CGXByteBuffer challenge;
		challenge.Set(challenge_buffer, challenge_len);
		clientstruct->client->SetCtoSChallenge(challenge);
	}

	clientstruct->client->SetClientAddress(clientaddress);
	clientstruct->client->SetAuthentication((DLMS_AUTHENTICATION)authenticationtype);
	clientstruct->client->GetCiphering()->SetSecurity((DLMS_SECURITY)securitytype);
}


unsigned char* dlms_client_encode_SNRM_request_payload(int clientid, int* len)
{
	DlmsClientInterface* clientstruct = clientMap[clientid];

	if (!clientstruct)
	{
		return NULL;
	}

	if (!clientstruct->client)
	{
		return NULL;
	}

	unsigned char* encoded_buffer = NULL;
	*len = 0;

	if (clientstruct->invocationcounter != NULL && clientstruct->client->GetCiphering() != NULL)
	{
		clientstruct->client->SetProposedConformance((DLMS_CONFORMANCE)(clientstruct->client->GetProposedConformance() | DLMS_CONFORMANCE_GENERAL_PROTECTION));
		std::vector<CGXByteBuffer> data;

		int ret = 0;

		ret = clientstruct->client->SNRMRequest(data);

		if (ret != 0)
		{
			return NULL;
		}

		if (data.size() > 0)
		{
			CGXByteBuffer packet = data.at(0);
			encoded_buffer = (unsigned char*)calloc(1, packet.GetSize());

			if (encoded_buffer)
			{
				memcpy(encoded_buffer, packet.GetData(), packet.GetSize());
				*len = packet.GetSize();
			}		
		}
	}

	return encoded_buffer;
}

unsigned char* dlms_client_decode_UAR_response_payload(int clientid, const unsigned char* buffer, int len)
{
	DlmsClientInterface* clientstruct = clientMap[clientid];

	if (!clientstruct)
	{
		return NULL;
	}

	if (!clientstruct->client)
	{
		return NULL;
	}

	CGXByteBuffer reply;
	std::string xml;
	reply.Set(buffer, len);

	clientstruct->client->ParseUAResponse(reply);

	//CGXDLMSTranslator translator(DLMS_TRANSLATOR_OUTPUT_TYPE_SIMPLE_XML);
	//translator.DataToXml(reply, xml);

	return NULL;
}

unsigned char* dlms_client_encode_AARQ_request_payload(int clientid)
{
	DlmsClientInterface* clientstruct = clientMap[clientid];

	if (!clientstruct)
	{
		return NULL;
	}

	if (!clientstruct->client)
	{
		return NULL;
	}

	return NULL;
}

unsigned char* dlms_client_decode_AARE_response_payload(int clientid, const unsigned char* buffer, int len)
{
	DlmsClientInterface* clientstruct = clientMap[clientid];

	if (!clientstruct)
	{
		return NULL;
	}

	if (!clientstruct->client)
	{
		return NULL;
	}

	return NULL;
}

unsigned char* dlms_client_encode_OBIS_request_payload(int clientid)
{
	DlmsClientInterface* clientstruct = clientMap[clientid];

	if (!clientstruct)
	{
		return NULL;
	}

	if (!clientstruct->client)
	{
		return NULL;
	}

	return NULL;
}

unsigned char* dlms_client_decode_OBIS_response_payload(int clientid, const unsigned char* buffer, int len)
{
	DlmsClientInterface* clientstruct = clientMap[clientid];

	if (!clientstruct)
	{
		return NULL;
	}

	if (!clientstruct->client)
	{
		return NULL;
	}

	return NULL;
}
