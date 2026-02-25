#include <string.h>
#include <iostream>
#include <string>
#include "GXDLMSData.h"
#include "Communication.h"
#include <mosquitto.h>
#include <unistd.h>

#include "Logger.h"
#include "SignalHandler.h"
#include "Configuration.h"


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

static CGXCommunication* commptr = nullptr;
static struct mosquitto *mosq = nullptr;

void parse_obis(const char* obis_str, unsigned char ln[6])
{
    sscanf(obis_str, "%hhu.%hhu.%hhu.%hhu.%hhu.%hhu", &ln[0], &ln[1], &ln[2], &ln[3], &ln[4], &ln[5]);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    //We assume that the payload is a command to update firmware. 
    // In real application, you should parse the message and handle different commands accordingly.
    //So check for the valid communication pointer before processing the message.

    printf("Message received.\n");

    //The topic format is /idms/{command}/{device_id}/, so we can parse the command and device_id from the topic if needed.
    //IDMS stands for "IoT Device Management System". The command can be "update_firmware", "restart_device", etc. The device_id can be used to identify which device the command is for.

        // Extract payload into string
    std::string payload(static_cast<const char*>(msg->payload), msg->payloadlen);

    // Parse topic: /idms/{command}/{deviceid}
    std::string topic(msg->topic);
    std::string command, deviceid;

    // Split by '/'
    std::vector<std::string> parts;
    std::istringstream ss(topic);
    std::string token;

    while (std::getline(ss, token, '/'))
    {
        if (!token.empty())
            parts.push_back(token);
    }

    // Expected: parts[0]="idms", parts[1]=command, parts[2]=deviceid
    if (parts.size() >= 3 && parts[0] == "idms")
    {
        command  = parts[1];
        deviceid = parts[2];
    }
    else
    {
        // Malformed topic
        return;
    }

    // command, deviceid and payload are ready to use. For example, if command is "update_firmware", we can call a function to handle firmware update for the device with given deviceid and payload.

    // Print the command and device ID for debugging
    printf("Received command: %s for device: %s with payload: %s\n", command.c_str(), deviceid.c_str(), payload.c_str());

    //For testing only, we will send back a dummy response to the IoT server to confirm that we received the command and processed it. In real application, you should implement the actual logic to handle the command and send appropriate response.
    std::string response_topic = "/idms/" + command + "/" + deviceid + "/response";
    std::string response_payload = "OK"; // In real application, this can be the result of processing the command.
    mosquitto_publish(mosq, NULL, response_topic.c_str(), response_payload.length(), response_payload.c_str(), 1, false);

    if (commptr == nullptr)
    {
        printf("Meter connection is not initialized.\n");
        return;
    }

    // Firmware update
    std::string fwId;
    std::string image;
    std::cout << "Enter firmware ID: ";
    std::getline(std::cin, fwId);
    std::cout << "Enter image binary: ";
    std::getline(std::cin, image);
    // Send firmware update
    CGXDLMSData fwData("0.0.1.0.0.255");
    CGXDLMSVariant fwValue;
    fwValue = fwId + ";" + image;
    int fwRet = commptr->Write(&fwData, 2, fwValue);
    if (fwRet != 0)
    {
        printf("Firmware update failed (%d)\n", fwRet);
    }

    printf("Received message on topic %s: %s\n", msg->topic, (char*)msg->payload);
}

static void ShowHelp();
static int StartClient(/*int argc, char* argv[]*/);

int main(int argc, char* argv[])
{
    int ret = 0;

    SignalHandlerClient client;

    SignalHandler sgnHandler;
    sgnHandler.registerCallbackClient(&client);
    sgnHandler.registerSignalHandlers();

    Logger::GetInstance()->setModuleName(argv[0]);
    Logger::GetInstance()->setLogDirectory("./logs");   
    Logger::GetInstance()->setLogFileSize(10 * 1024 * 1024); //10 MB
    Logger::GetInstance()->startLogging(FileAppend);

    writeLogNormal("Starting DLMS client application.");

    //Now will create the configuration objects. It loads from default loaction. "/etc/" for root and "home/%USER%/.config" for non root users.
    Configuration config;
    config.setFileName("DlmsClient");
    config.loadConfiguration();

    std::string mqtthost = config.getValue("MQTT", "Host", "127.0.0.1");
    std::string mqttport = config.getValue("MQTT", "Port", "1883");
    std::string mqttuser = config.getValue("MQTT", "Username", "");
    std::string mqttpassword = config.getValue("MQTT", "Password", "");
    std::string mqtttClientId = config.getValue("MQTT", "ClientId", "dlms/commands");

    // Initialize MQTT
    mosquitto_lib_init();
    mosq = mosquitto_new(mqtttClientId.c_str(), true, NULL);
    if (!mosq)
    {
        printf("Error: Out of memory.\n");
        return 1;
    }
    mosquitto_message_callback_set(mosq, on_message);
    if (mosquitto_connect(mosq, mqtthost.c_str(), std::stoi(mqttport), 60))
    {
        printf("Unable to connect to MQTT broker.\n");
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    printf("Connected to MQTT broker.\n");

    mosquitto_subscribe(mosq, NULL, "/idms/+/+/", 1);
    mosquitto_loop_start(mosq);

    // if (argc < 2)
    // {
    //     ShowHelp();
    //     mosquitto_loop_stop(mosq, true);
    //     mosquitto_destroy(mosq);
    //     mosquitto_lib_cleanup();
    //     return -1;
    // }

    //ret = StartClient(argc, argv);

    while(true)
    {
        sleep(5);
    }

    // Cleanup MQTT
    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return ret;
}

static void ShowHelp()
{
    printf("GuruxDlmsSample reads data from the DLMS/COSEM device.\n");
    printf("GuruxDlmsSample -h [Meter IP Address] -p [Meter Port No] -c 16 -s 1 -r SN\n");
    printf(" -h \t host name or IP address.\n");
    printf(" -p \t port number or name (Example: 1000).\n");
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
}

static int StartClient(/*int argc, char* argv[]*/)
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
    commptr = &comm;

    if(comm.Connect(address, port) != DLMS_ERROR_CODE_OK)
    {
        printf("Failed to connect to meter at address %s port %d\n", address, port);
        return -1;
    }

    // Initialize connection
    ret = comm.InitializeConnection();
    if (ret != 0)
    {
        printf("Failed to initialize connection: %d\n", ret);
        return ret;
    }

    // DCU polling loop for 5 registers
    const char* obis_codes[] = {
        "1.0.1.8.0.255",  // Active energy import total
        "1.0.1.7.0.255",  // Active power total
        "1.0.1.8.1.255",  // Active energy import tariff 1
        "1.0.2.8.1.255",  // Active energy export tariff 1
        "1.0.14.7.0.255"  // Frequency
    };

    const char* attribute_names[] = {
        "Active Energy Import Total",
        "Active Power Total",
        "Active Energy Import Tariff 1",
        "Active Energy Export Tariff 1",
        "Frequency"
    };

    const char* uom[] = {
        "kWh",
        "kW",
        "kWh",
        "kWh",
        "Hz"
    };

    while (true)
    {
        for (int i = 0; i < 5; ++i)
        {
            unsigned char ln[6];
            parse_obis(obis_codes[i], ln);
            CGXDLMSObject* obj = cl.GetObjects().FindByLN(DLMS_OBJECT_TYPE_ALL, ln);
            if (obj != NULL)
            {
                std::string value;
                if (comm.Read(obj, 2, value) == DLMS_ERROR_CODE_OK)
                {
                    // Publish to MQTT with attribute name and UOM
                    std::string topic = "dlms/data/" + std::string(obis_codes[i]);
                    std::string payload = std::string(attribute_names[i]) + "," + value + "," + std::string(uom[i]);
                    mosquitto_publish(mosq, NULL, topic.c_str(), payload.length(), payload.c_str(), 0, false);
                    printf("Published %s: %s\n", obis_codes[i], payload.c_str());
                }
                else
                {
                    printf("Failed to read %s\n", obis_codes[i]);
                }
            }
            else
            {
                printf("Object not found: %s\n", obis_codes[i]);
            }
        }
        sleep(10);  // Poll every 10 seconds
    }

    comm.Close();
    return 0;
}
