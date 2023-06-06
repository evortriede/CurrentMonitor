typedef struct 
{
  char ssid[25];
  char pass[25];
} saved_hotspot_t;

#include <hotspots.h>
                                
void *getLocalHotspot()
{
   Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) 
  {
      Serial.println("no networks found");
  } 
  else 
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) 
    {
      String id = WiFi.SSID(i);
      Serial.printf("looking for %s\n",id.c_str());
      for (int j=0;j<sizeof(savedHotspots)/sizeof(savedHotspots[0]);j++)
      {
        Serial.printf("looking at %s\n",savedHotspots[j].ssid);
        if (strcmp(savedHotspots[j].ssid,id.c_str())==0)
        {
          Serial.println("found");
          return &savedHotspots[j];
        }
      }
    }
  }
  return (void *)&configData;
}

void wifiSTASetup()
{
  saved_hotspot_t *hotspot=(saved_hotspot_t*)getLocalHotspot();
  Serial.print("Connecting to ");
  Serial.println(hotspot->ssid);
  
  WiFi.begin(hotspot->ssid, hotspot->pass);
  Serial.println("");

  long wifiTimeOut=millis()+30000l;
  // Wait for connection
  while ((WiFi.status() != WL_CONNECTED) && (millis()<wifiTimeOut)) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi STA setup timed out");
    ESP.restart();
  }
  else
  {
    wifiStaConnected=true;
    Serial.println("");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void displayIPs(int x, int y, boolean fSerialPrint)
{
  char rgIPTxt[32];

  IPAddress myIP = WiFi.softAPIP();
  sprintf(rgIPTxt,"%u.%u.%u.%u",myIP[0],myIP[1],myIP[2],myIP[3]);
  
  Heltec.display->clear();
  Heltec.display->drawStringMaxWidth(x, y, 128, rgIPTxt);

  if (fSerialPrint)
  {
    Serial.print("AP IP address: ");
    Serial.println(rgIPTxt);
  }

  myIP = WiFi.localIP();
  sprintf(rgIPTxt,"%u.%u.%u.%u",myIP[0],myIP[1],myIP[2],myIP[3]);
  Heltec.display->drawStringMaxWidth(x, y+16, 128, rgIPTxt);

  Heltec.display->display();

  if (fSerialPrint)
  {
    Serial.print("Local IP address: ");
    Serial.println(rgIPTxt);
  }
}

void wifiAPSetup()
{
  Serial.println("Configuring access point...");
  WiFi.mode(WIFI_AP_STA);
  wifiSTASetup();

  Serial.printf("Setting up soft AP for %s\n",configData.captive_ssid);
  
  WiFi.softAP(configData.captive_ssid,configData.captive_pass);

  displayIPs(0,0,true);
  
  telnetServer.begin();
  telnetClient=telnetServer.available();

  recorderServer.begin();
  recorderClient=recorderServer.available();

}
