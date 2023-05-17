#include "CurrentMonitor.h"
#include "pages.h"

char rootMsg[4096];

void handleRoot()
{
  sprintf(rootMsg
         ,rootFmt
         ,configData.ssid
         ,configData.pass
         ,configData.captive_ssid
         ,configData.captive_pass
         ,configData.sf
         ,configData.modbusServer
         ,configData.frequency);
  server.send(200, "text/html", rootMsg);
}


void reboot()
{
  ESP.restart();
}

void redirect()
{
  server.send(200, "text/html", "<html><head></head><body onLoad=\"location.replace('/');\">Redirecting...</body></html>");
}

void handleSet()
{
  strcpy(configData.ssid,server.arg("ssid").c_str());
  strcpy(configData.pass,server.arg("pass").c_str());
  strcpy(configData.captive_ssid, server.arg("captive_ssid").c_str());
  strcpy(configData.captive_pass, server.arg("captive_pass").c_str());
  configData.sf=atoi(server.arg("sf").c_str());
  strcpy(configData.modbusServer,server.arg("modbus_server").c_str());
  configData.frequency=atoi(server.arg("frequency").c_str());

  nvs_handle handle;
  esp_err_t res = nvs_open("lwc_data", NVS_READWRITE, &handle);
  Serial.printf("nvs_open %i\n",res);
  res = nvs_set_blob(handle, "cur_mon_cfg", &configData, sizeof(configData));
  Serial.printf("nvs_set_blob %i\n",res);
  nvs_commit(handle);
  nvs_close(handle);
  redirect();
}

long timeToConnect=0;
bool testMode=false;

void webServerSetup()
{
  server.on("/",handleRoot);
  server.on("/set",handleSet);
  server.on("/reboot", HTTP_GET, []() {
    reboot();
  });
  server.on("/teston", HTTP_GET, []() {
    testMode=true;
    redirect();
  });
  server.on("/testoff", HTTP_GET, []() {
    testMode=true;
    redirect();
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    timeToConnect=millis()+3600000L;
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
}

void eepromSetup()
{
  nvs_handle handle;

  esp_err_t res = nvs_open("lwc_data", NVS_READWRITE, &handle);
  Serial.printf("nvs_open %i\n",res);
  size_t sz=sizeof(configData);
  res = nvs_get_blob(handle, "cur_mon_cfg", &configData, &sz);
  Serial.printf("nvs_get_blob %i; size %i\n",res,sz);
  nvs_close(handle);
  
  Serial.printf("ssid=%s\npass=%s\ncaptive_ssid=%s\ncaptive_pass=%s\nSF=%i\nmodbusServer=%s\nfrequency=%i\n"
               ,configData.ssid
               ,configData.pass
               ,configData.captive_ssid
               ,configData.captive_pass
               ,configData.sf
               ,configData.modbusServer
               ,configData.frequency);
}


void loraSend(byte *rgch,int len)
{
  delay(100);
  Serial.printf("lora sending %i bytes ",len);
  for (int i=0;i<len;i++)
  {
    Serial.printf(" %02x",rgch[i]);
  }
  Serial.println();
  LoRa.setTxPower(20,RF_PACONFIG_PASELECT_PABOOST);
  LoRa.beginPacket();
  for (int i=0;i<len;i++)
  {
    LoRa.write(*rgch++);
  }
  LoRa.endPacket();
  LoRa.receive();
}

void onLoRaReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return

  if (packetSize > 20)
  {
    while (LoRa.available())
    {
      LoRa.read();
    }
    return;
  }

  byte buff[100];
  for (int i=0;LoRa.available();i++)
  {
    buff[i]=LoRa.read();
    buff[i+1]=0;
  }
  Serial.printf("lora receive %i bytes ",packetSize);
  for (int i=0;i<packetSize;i++)
  {
    Serial.printf(" %02x",buff[i]);
  }
  Serial.println();
  gp.processRecv((void*)buff,packetSize);
}

void logger(const char *msg)
{
  Serial.print(msg);
}

void gotData(byte *data,int len)
{
  Serial.println("Got data");
  /*
   * Should be from CurrentRecorder to forward to the CL17 to ChlorinePump
   * controller.
   */
  if (*data!='P') return;
  unsigned short *ps=(unsigned short *)&data[1];
  sprintf(toPump,"P%u\n",*ps);
  Serial.printf("Message to pump: %s",toPump);
  if (telnetClient)
  {
    telnetClient.print(toPump);
    telnetClient.print("\r");
  }
  else
  {
    Serial.println("telnetClient unavailable");
  }
}

void connected()
{
  Serial.println("connected");
  _connected=true;
}

void disconnected()
{
  Serial.println("disconnected");
  _connected=false;
}

int myprintf(const char *format, va_list list)
{
  return Serial.printf(format,list);
}

#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

void setup() 
{
  esp_log_set_vprintf(&myprintf);
  
  Heltec.begin(true /*DisplayEnable Enable*/, 
               true /*Heltec.LoRa Disable*/, 
               true /*Serial Enable*/, 
               true /*PABOOST Enable*/, 
               BAND /*long BAND*/);
               
  Serial.begin(115200);
  
  pinMode(12,INPUT_PULLUP);
  pinMode(0,INPUT_PULLUP);
  
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setLogBuffer(5, 64);
  Heltec.display->clear();
  Heltec.display->drawStringMaxWidth(0, 0, 128, "about to set up LoRa");
  Heltec.display->display();
  
  eepromSetup();
  
  LoRa.setSpreadingFactor(configData.sf);
  LoRa.receive();

  Serial.println("Heltec.LoRa init succeeded.");

  wifiAPSetup();

  MDNS.begin(configData.captive_ssid);
  MDNS.addService("http", "tcp", 80);

  webServerSetup();
  
  Serial.println("back in setup after WiFi setup");
  gp.setSendMethod(&loraSend);
  gp.setOnReceive(&gotData);
  gp.setOnConnect(&connected);
  gp.setOnDisconnect(&disconnected);
  gp.setLogMethod(&logger);
  gp.setTimeout(2000);
  
  //gp.setMonitorMode(true);
  
  gp.start();
}

void report(void *data, int len)
{
  gp.sendData(data,len);
  gp.handler();
}


/*
 * These messages represent the readings from the CL17 chlorine meter.
 * They are converted into LoRa messages for the CurrentRecorder
 */
void handleTelnetBuf()
{
  Serial.printf("\nFrom Telnet: %s\n\n",telnetBuf);

  unsigned short n=(unsigned short)atoi(&telnetBuf[1]);
  msgBuf[0]=telnetBuf[0];
  unsigned short *ps=(unsigned short*)(&msgBuf[1]);
  *ps=n;
  
  report((void *)msgBuf,3);
}

void handleTelnetCharacter(char ch)
{
  if (ch=='\n')
  {
    handleTelnetBuf();
    telnetPut=telnetBuf;
  }
  else if (ch!='\r')
  {
    *telnetPut++=ch;
    *telnetPut=0;
  }
}

void handleTelnet()
{
  unsigned long metricTracker=millis();
  if (telnetServer.hasClient())
  {
    if (telnetClient) // already have one?
    {
      Serial.println("\nTrashing existing telnet connection\n");
      telnetClient.stop();
    }
    telnetClient=telnetServer.available();
    while (telnetClient.available()) telnetClient.read();
    Serial.printf("\ngot a telnet connection %lu\n",millis()-metricTracker);
  }
  else
  {
    if (millis()-metricTracker>1)
    {
      Serial.printf("LONG TIME %lu\n",millis()-metricTracker);
    }
  }
  if (telnetClient)
  {
    while (telnetClient.available())
    {
      handleTelnetCharacter(telnetClient.read());
    }
  }
}

void checkForTransients()
{
  int newValue=digitalRead(testMode?0:12);
  
  if ((newValue != pinValue) || (millis()-lastSendTime)>=(1000*configData.frequency))
  {
    if (pinValue!=newValue) // transition
    {
      if (newValue) // transition from on to off
      {
        onDuration=millis()-transitionTime; // note on duration
        timeToSendVolume=millis()+5000; // get tank reading in 5 seconds
      }
      else // transition from off to on
      {
        offDuration=millis()-transitionTime; // note off duration
        if (offDuration < offTooShort)
        {
          Serial.printf("off too short %i\n",offDuration);
          report(highWaterUsage,1);
        }
      }
      transitionTime=millis();
    }
    else if (!newValue) // not a transition but on for configured timeout seconds or more
    {
      onDuration = millis()-transitionTime;
      if (onDuration > onTooLong)
      {
        Serial.printf("on too long %i\n",onDuration);
        report(highWaterUsage,1);
      }
    }
    else // not a transition and we are accumulating offDuration
    {
      offDuration = millis()-transitionTime;
    }
    pinValue=newValue;
    lastSendTime=millis();
    sprintf(gallonsMessage,"%s %i gallons",(pinValue==0)?"on":"off",gallons);
    message=(pinValue==0)?"current is flowing":"no current flowing";
//      sprintf(msgBuf,"%s %i %i\n",message,onDuration,offDuration);
    msgBuf[0]=message[0];
    unsigned *pi=(unsigned*)(&msgBuf[1]);
    pi[0]=onDuration;
    pi[1]=offDuration;
    report((void *)msgBuf,9);
  }
}

void loop() 
{
  if (_connected)
  {
    checkForTransients();
    if (displayTime<=millis())
    {
      Heltec.display->clear();
      Heltec.display->drawStringMaxWidth(x, y, 128, gallonsMessage);
      Heltec.display->display();
      x = (x+1)%80;
      y = (y+1)%60;
      displayTime=millis()+100;
    }
    modbusLoop();
  }
  onLoRaReceive(LoRa.parsePacket());
  gp.handler();
  server.handleClient();
  handleTelnet();
}
