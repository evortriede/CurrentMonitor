#include <fsm.h>

#include <GenericProtocol.h>

#include "Arduino.h"
#include "heltec.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Update.h>
#include <esp_log.h>
#include <nvs.h>
#include <ESPmDNS.h>

WiFiServer telnetServer(23);
WiFiServer recorderServer(9023);
WiFiClient telnetClient;
WiFiClient recorderClient;
WebServer server(80);

typedef struct
{
  char ssid[25];
  char pass[25];
  char captive_ssid[25];
  char captive_pass[25];
  char modbusServer[25];
  int sf;
  int frequency;
} config_data_t;

config_data_t configData={"","","CurrentMonitor","","192.168.0.1",8,60};

bool wifiStaConnected=false;
GenericProtocol gp;

bool _connected=false;

char toPump[16];

long lastSendTime = 0;        // last send time
int x=0,y=0;
int pinValue=0;
const char *message="no current flowing\n";

WiFiClient client;

long displayTime=0;
uint16_t transId=1;
uint16_t turbTid=0;
uint16_t volTid=0;
long timeToSendVolume=millis()+60000L;
long timeToSendTurbidity=millis()+40000L;
long timeToSendQuery=millis();
long watchdog=0;
int gallons=0;
int turbidity=0;

long transitionTime=0;
int onDuration=0;
int offDuration=0;
int onTooLong=60000;
int offTooShort=150000;
char *highWaterUsage="WATER USAGE VERY HIGH\n";
char gallonsMessage[30];

void modbusLoop();
bool getConnection();
void getVolume(long &timeToSend);
uint16_t getUInt16(byte *msg);
void sendReadHoldingRegister(uint16_t registerNo, uint16_t transactionId);
bool pollResponse(uint16_t &transactionId, uint16_t &respValue);
void sendVal(byte tag, uint16_t val);

char telnetBuf[128];
char *telnetPut=telnetBuf;
char msgBuf[50];

void wifiAPSetup();
void displayIPs(int x, int y, boolean fSerialPrint);
void wifiSTASetup();
void *getLocalHotspot();
