#ifndef DLMS_CLIENT_INTERFACE
#define DLMS_CLIENT_INTERFACE

#include <stdbool.h>
#include <stdint.h>

extern int DLMS_CLIENT_AUTHENTICATION_HIGH_GMAC;
extern int DLMS_CLIENT_AUTHENTICATION_NONE;
extern int DLMS_CLIENT_INTERFACE_TYPE_HDLC;
extern int DLMS_CLIENT_SECURITY_AUTHENTICATION_ENCRYPTION;
extern int DLMS_CLIENT_SECURITY_AUTHENTICATION_NONE;

#ifdef __cplusplus
extern "C"
{
#endif

	extern char* dlms_client_system_information(void);

	extern bool dlms_client_create(int clientid, int authenticationtype, int interfacetype, int securitytype, bool uselogicalnaming, int serveraddress, int clientaddress, const char* password, const char* systemtitle, const char* autheticationkey, const char* blockcipherkey, const char* dedicatedkey);
	extern bool dlms_client_set_session_parameters(int clientid, bool autoincreaseid , const char* invokationcounter, unsigned char gbtwindowsize, unsigned char windowsize, uint16_t maxinfo, char* manufacturerid);
	extern bool dlms_client_destroy(int clientid);
	extern bool dlms_client_release_buffer(int clientid);

	extern unsigned char* dlms_client_encode_SNRM_request_payload(int clientid, int* len);
	extern unsigned char* dlms_client_decode_UAR_response_payload(int clientid, const unsigned char* buffer, int len);

	extern unsigned char* dlms_client_encode_AARQ_request_payload(int clientid);
	extern unsigned char* dlms_client_decode_AARE_response_payload(int clientid, const unsigned char* buffer, int len);

	extern unsigned char* dlms_client_encode_OBIS_request_payload(int clientid);
	extern unsigned char* dlms_client_decode_OBIS_response_payload(int clientid, const unsigned char* buffer, int len);

	extern unsigned char* dlms_client_get_challenge(int clientid, int* len);
	extern void dlms_client_set_security(int clientid, int clientaddress, int authenticationtype, int securitytype, unsigned char* challenge_buffer, int challenge_len);


#ifdef __cplusplus
}
#endif

#endif