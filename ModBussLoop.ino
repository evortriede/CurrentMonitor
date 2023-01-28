void getConnection()
{
  if (!client.connected())
  {
    if (millis()>=timeToConnect)
    {
      Serial.print("connecting to ");
      Serial.println(configData.modbusServer);
      if (client.connect(configData.modbusServer,502))
      {
        timeToSendVolume=millis();
        timeToSendTurbidity=millis()+30000L;
        Serial.println("connected");
      }
      else
      {
        Serial.println("couldn't connect");
        timeToConnect=millis()+60000;
      }
      transId=1;
    }
  }
}

bool bypass=true;
int failureCount=0;

bool filling(long &timeToSend)
{
  if (bypass) return true;
  if (client.connected() && millis()>=timeToSend)
  {
    byte message[120];
    if (timeToSend)
    {
      message[0]=transId>>8;
      message[1]=transId&0xff;
      transId+=1;
      message[2]=0;
      message[3]=0;
      message[4]=0;
      message[5]=6;
      message[6]=0;
      message[7]=2;
      message[8]=0;
      message[9]=0;
      message[10]=0;
      message[11]=1;
      client.write(message,12);
      Serial.println("sent read coil message");
      timeToSend=0;
      watchdog=millis()+10000;
    }
    else if (client.available())
    {
      failureCount=0;
      int i=0;
      for (i=0;client.available();i++)
      {
        message[i]=client.read();
      }
      Serial.printf("size=%i coil 0 = %i %i\n",i,message[9], message[10]);
      if (message[9]!=0 || message[10]!=0)
      {
        return true;
      }
      timeToSend=millis()+(1000L * (configData.frequency/2));
    }
    else if (watchdog < millis())
    {
      failureCount++;
      if (failureCount>=5)
      {
        Serial.println("failure count exceeded - restarting");
        ESP.restart();
      }
      timeToSend=millis();
    }
  }
  return false;
}

void getVolume(long &timeToSend)
{
  if (client.connected() && millis()>=timeToSend)
  {
    byte message[12];
    if (timeToSend)
    {
      message[0]=transId>>8;
      message[1]=transId&0xff;
      transId+=1;
      message[2]=0;
      message[3]=0;
      message[4]=0;
      message[5]=6;
      message[6]=0;
      message[7]=3;
      message[8]=0;
      message[9]=0x8b;
      message[10]=0;
      message[11]=1;
      client.write(message,12);
      Serial.println("sent message");
      timeToSend=0;
      watchdog=millis()+10000;
    }
    else if (client.available())
    {
      failureCount=0;
      for (int i=0;client.available();i++)
      {
        message[i]=client.read();
      }
      byte msg[3];
      msg[0]='r';
      msg[1]=message[10];
      msg[2]=message[9];
      gp.sendData((void*)msg, 3);
      timeToSend=millis()+(1000L * configData.frequency);
      unsigned val=message[9];
      val <<= 8;
      val += message[10];
      gallons = (719 * val) / 100;
    }
    else if (watchdog < millis())
    {
      failureCount++;
      if (failureCount>=5)
      {
        Serial.println("failure count exceeded - restarting");
        ESP.restart();
      }
      timeToSend=millis();
    }
  }
}

void getTurbidity(long &timeToSend)
{
  if (client.connected() && millis()>=timeToSend)
  {
    byte message[12];
    if (timeToSend)
    {
      message[0]=transId>>8;
      message[1]=transId&0xff;
      transId+=1;
      message[2]=0;
      message[3]=0;
      message[4]=0;
      message[5]=6;
      message[6]=0;
      message[7]=3;
      message[8]=0;
      message[9]=0x64;
      message[10]=0;
      message[11]=1;
      client.write(message,12);
      Serial.println("sent turbidity message");
      timeToSend=0;
      watchdog=millis()+10000;
    }
    else if (client.available())
    {
      failureCount=0;
      for (int i=0;client.available();i++)
      {
        message[i]=client.read();
      }
      if (message[9] || message[10])
      {
        byte msg[3];
        msg[0]='T';
        msg[1]=message[10];
        msg[2]=message[9];
        gp.sendData((void*)msg, 3);
      }
      timeToSend=millis()+(1000L * configData.frequency);
      unsigned val=message[9];
      val <<= 8;
      val += message[10];
      turbidity = val;
    }
    else if (watchdog < millis())
    {
      failureCount++;
      if (failureCount>=5)
      {
        Serial.println("failure count exceeded - restarting");
        ESP.restart();
      }
      timeToSend=millis();
    }
  }
}

void modbusLoop() 
{
  if (!wifiStaConnected) return;

  getConnection();

  if (filling(timeToSendQuery))
  {
    getTurbidity(timeToSendTurbidity);
  }

  getVolume(timeToSendVolume);
    
}
