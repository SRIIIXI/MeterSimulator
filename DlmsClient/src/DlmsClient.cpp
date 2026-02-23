#include <string.h>
#include "Communication.h"

static void ShowHelp();
static int StartClient(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        ShowHelp();
        return -1;
    }

    return StartClient(argc, argv);
}

static void ShowHelp()
{
    printf("GuruxDlmsSample reads data from the DLMS/COSEM device.\n");
    printf("GuruxDlmsSample -h [Meter IP Address] -p [Meter Port No] -c 16 -s 1 -r SN\n");
    printf(" -h \t host name or IP address.\n");
    printf(" -p \t port number or name (Example: 1000).\n");
    printf(" -S [COM1:9600:8None1]\t serial port.\n");
    printf(" -a \t Authentication (None, Low, High, HighMd5, HighSha1, HighGmac, HighSha256).\n");
    printf(" -P \t Password for authentication.\n");
    printf(" -c \t Client address. (Default: 16)\n");
    printf(" -s \t Server address. (Default: 1)\n");
    printf(" -n \t Server address as serial number.\n");
    printf(" -l \t Logical Server address.");
    printf(" -r [sn, sn]\t Short name or Logican Name (default) referencing is used.\n");
    printf(" -t Trace messages.\n");
    printf(" -g \"0.0.1.0.0.255:1; 0.0.1.0.0.255:2\" Get selected object(s) with given attribute index.\n");
    printf(" -C \t Security Level. (None, Authentication, Encrypted, AuthenticationEncryption)\n");
    printf(" -v \t Invocation counter data object Logical Name. Ex. 0.0.43.1.1.255\n");
    printf(" -I \t Auto increase invoke ID\n");
    printf(" -o \t Cache association view to make reading faster. Ex. -o C:\\device.xml\n");
    printf(" -T \t System title that is used with chiphering. Ex -T 4775727578313233\n");
    printf(" -A \t Authentication key that is used with chiphering. Ex -A D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF\n");
    printf(" -B \t Block cipher key that is used with chiphering. Ex -B 000102030405060708090A0B0C0D0E0F\n");
    printf(" -D \t Dedicated key that is used with chiphering. Ex -D 00112233445566778899AABBCCDDEEFF\n");
    printf(" -i \t Used communication interface. Ex. -i WRAPPER.\n");
    printf(" -m \t Used PLC MAC address. Ex. -m 1.\n");
    printf(" -W \t General Block Transfer window size.\n");
    printf(" -w \t HDLC Window size. Default is 1\n");
    printf(" -f \t HDLC Frame size. Default is 128\n");
    printf(" -L \t Manufacturer ID (Flag ID) is used to use manufacturer depending functionality. -L LGZ\n");
    printf("Example:\n");
    printf("Read LG device using TCP/IP connection.\n");
    printf("GuruxDlmsSample -r SN -c 16 -s 1 -h [Meter IP Address] -p [Meter Port No]\n");
    printf("Read LG device using serial port connection.\n");
    printf("GuruxDlmsSample -r SN -c 16 -s 1 -sp COM1 -i\n");
    printf("Read Indian device using serial port connection.\n");
    printf("GuruxDlmsSample -S COM1:9600:8None1 -c 16 -s 1 -a Low -P [password]\n");
}

static int StartClient(int argc, char* argv[])
{
    int ret;
    //Remove trace file if exists.
    remove("trace.txt");
    remove("LogFile.txt");
    GX_TRACE_LEVEL trace = GX_TRACE_LEVEL_INFO;
    bool useLogicalNameReferencing = true;
    int clientAddress = 16, serverAddress = 1;
    DLMS_AUTHENTICATION authentication = DLMS_AUTHENTICATION_NONE;
    DLMS_INTERFACE_TYPE interfaceType = DLMS_INTERFACE_TYPE_HDLC;
    DLMS_SECURITY security = DLMS_SECURITY_NONE;
    char* password = NULL;
    char* p, * p2, * readObjects = NULL;
    int index, a, b, c, d, e, f;
    int opt = 0;
    int port = 0;
    char* address = NULL;
    bool autoIncreaseInvokeID = false;
    char* invocationCounter = NULL;
    char* outputFile = NULL;
    char* systemTitle = NULL;
    char* authenticationKey = NULL;
    char* blockCipherKey = NULL;
    char* dedicatedKey = NULL;
    uint16_t macDestinationAddress;
    unsigned char gbtWindowSize = 1;
    unsigned char windowSize = 1;
    uint16_t maxInfo = 128;
    char* manufacturerId = NULL;

    //
    useLogicalNameReferencing = true;
    clientAddress = 16;
    serverAddress = 1;
    authentication = DLMS_AUTHENTICATION_HIGH_GMAC;
    password = nullptr;
    interfaceType = DLMS_INTERFACE_TYPE_HDLC;
    autoIncreaseInvokeID = true;
    trace = GX_TRACE_LEVEL_VERBOSE;
    port = 4059;
    security = DLMS_SECURITY_NONE;

    password = (char*)calloc(1, 64);
    address = (char*)calloc(1, 64);
    readObjects = (char*)calloc(1, 64);
    invocationCounter = (char*)calloc(1, 64);
    outputFile = (char*)calloc(1, 64);

    strcpy(address, "192.168.0.50");
    strcpy(readObjects, "0-0:42.0.0.255");
    strcpy(invocationCounter, "0.0.43.1.1.255");
    strcpy(outputFile, "/root/crystal.xml");
    //

    CGXDLMSSecureClient cl(useLogicalNameReferencing, clientAddress, serverAddress, authentication, password, interfaceType);
    cl.GetCiphering()->SetSecurity(security);
    cl.SetAutoIncreaseInvokeID(autoIncreaseInvokeID);
    cl.SetGbtWindowSize(gbtWindowSize);
    cl.GetHdlcSettings().SetWindowSizeRX(windowSize);
    cl.GetHdlcSettings().SetWindowSizeTX(windowSize);
    cl.GetHdlcSettings().SetMaxInfoRX(maxInfo);
    cl.GetHdlcSettings().SetMaxInfoTX(maxInfo);
    cl.GetHdlcSettings().SetMaxInfoTX(maxInfo);
    cl.SetManufacturerId(manufacturerId);
    CGXByteBuffer bb;

    if (systemTitle != NULL)
    {
        bb.Clear();
        bb.SetHexString(systemTitle);
        cl.GetCiphering()->SetSystemTitle(bb);
    }

    if (authenticationKey != NULL)
    {
        bb.Clear();
        bb.SetHexString(authenticationKey);
        cl.GetCiphering()->SetAuthenticationKey(bb);
    }

    if (blockCipherKey != NULL)
    {
        bb.Clear();
        bb.SetHexString(blockCipherKey);
        cl.GetCiphering()->SetBlockCipherKey(bb);
    }

    if (dedicatedKey != NULL)
    {
        bb.Clear();
        bb.SetHexString(dedicatedKey);
        cl.GetCiphering()->SetDedicatedKey(bb);
    }

    CGXCommunication comm(&cl, 5000, trace, invocationCounter);

    if (port != 0 || address != NULL)
    {
        if (port == 0)
        {
            printf("Missing mandatory port option.\n");
            return 1;
        }
        if (address == NULL)
        {
            printf("Missing mandatory host option.\n");
            return 1;
        }
        if ((ret = comm.Connect(address, port)) != 0)
        {
            printf("Connect failed %s.\n", CGXDLMSConverter::GetErrorMessage(ret));
            return 1;
        }
    }
    else
    {
        printf("Missing mandatory connection information for TCP/IP or serial port connection.\n");
        printf("-----------------------------------------------------------------------------.\n");
        ShowHelp();
        return 1;
    }

    if (readObjects != NULL)
    {
        bool read = false;
        if (outputFile != NULL)
        {
            if ((ret = cl.GetObjects().Load(outputFile)) == 0)
            {
                ret = 0;
                read = true;
            }
        }

        ret = comm.InitializeConnection();

        if (ret == 0 && !read)
        {
            ret = comm.GetAssociationView();
        }

        printf("\n-----------------------------------------------------------------------------\n");
        printf("Reading %s\n", readObjects);
        printf("-----------------------------------------------------------------------------\n");

        if (ret == 0)
        {
            std::string str;
            std::string value;
            char buff[200];
            p = readObjects;
            do
            {
                if (p != readObjects)
                {
                    ++p;
                }

                str.clear();
                p2 = strchr(p, ':');
                ++p2;

                sscanf(p2, "%d", &index);

                str.append(p, p2 - p);
                CGXDLMSObject* obj = cl.GetObjects().FindByLN(DLMS_OBJECT_TYPE_ALL, str);
                if (obj != NULL)
                {
                    value.clear();
                    if ((ret = comm.Read(obj, index, value)) != DLMS_ERROR_CODE_OK)
                    {
                        sprintf(buff, "Error! Index: %d read failed: %s\n", index, CGXDLMSConverter::GetErrorMessage(ret));
                        comm.WriteValue(GX_TRACE_LEVEL_ERROR, buff);
                        //Continue reading.
                    }
                    else
                    {
                        sprintf(buff, "Index: %d Value: ", index);
                        comm.WriteValue(trace, buff);
                        comm.WriteValue(trace, value.c_str());
                        comm.WriteValue(trace, "\n");
                    }
                }
                else
                {
                    sprintf(buff, "Unknown object: %s", str.c_str());
                    str = buff;
                    comm.WriteValue(GX_TRACE_LEVEL_ERROR, str);
                }
            } while ((p = strchr(p, ',')) != NULL);

            comm.Close();
            if (outputFile != NULL && ret == 0)
            {
                ret = cl.GetObjects().Save(outputFile);
            }
        }
    }
    else
    {
        ret = comm.ReadAll(outputFile);
    }

    comm.Close();
    return 0;
}
