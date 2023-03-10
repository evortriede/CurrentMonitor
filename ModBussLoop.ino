bool getConnection()
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
        return false;
      }
      transId=1;
    }
  }
  return true;
}

uint16_t getUInt16(byte *msg)
{
  uint16_t retVal=msg[0];
  retVal<<=8;
  retVal+=msg[1];
  return retVal;
}

void sendReadHoldingRegister(uint16_t registerNo, uint16_t transactionId)
{
  byte message[12];
  message[0]=transactionId>>8;
  message[1]=transactionId&0xff;
  message[2]=0;
  message[3]=0;
  message[4]=0;
  message[5]=6;
  message[6]=0;
  message[7]=3;
  message[8]=registerNo>>8;
  message[9]=registerNo&0xff;
  message[10]=0;
  message[11]=1;
  client.write(message,12);
}

bool pollResponse(uint16_t &transactionId, uint16_t &respValue, uint16_t &swapRet)
{
  byte message[12];
  if (!client.available()) return false;
  Serial.printf("available=%i\n",client.available());
  for (int i=0;client.available();i++)
  {
    message[i]=client.read();
    Serial.printf("%02x ",message[i]);
  }
  Serial.println();
  transactionId=getUInt16(message);
  respValue=getUInt16(&message[9]);
  swapRet=message[10];
  swapRet<<=8;
  swapRet+=message[9];
  Serial.printf("transId=%u Val=%i Swap=%04x\n",transactionId,respValue,swapRet);
  return true;
}

void sendVal(byte tag, uint16_t val)
{
  uint16_t *ps;
  byte msg[3];
  msg[0]=tag;
  ps=(uint16_t*)&msg[1];
  *ps=val;
  gp.sendData((void*)msg,3);
}

uint16_t swapBytes(uint16_t val)
{
  uint16_t retVal=val&0xff;
  retVal<<=8;
  retVal+=((val>>8)&0xff);
  return retVal;
}

void modbusLoop() 
{
  uint16_t tid,val,swapVal;
  
  if (!wifiStaConnected) return;

  if (!getConnection()) return;

  if (pollResponse(tid,val,swapVal))
  {
    Serial.printf("%i %i %04x\n",tid,val,swapVal);
    if (tid==turbTid && val!=0)
    {
      sendVal('T',val);
      turbidity = val;
    }
    else if (tid==volTid)
    {
      sendVal('r',val);
      gallons = (719 * val) / 100;
    }
  }

  if (millis()>timeToSendVolume)
  {
    timeToSendVolume=millis()+(1000*configData.frequency);
    volTid=transId++;
    if (transId==0) transId=1; // 1-FFFF never 0
    sendReadHoldingRegister(0x8b,volTid);
  }

  if (millis()>timeToSendTurbidity)
  {
    timeToSendTurbidity=millis()+(1000*configData.frequency);
    turbTid=transId++;
    if (transId==0) transId=1; // 1-FFFF never 0
    sendReadHoldingRegister(0x64,turbTid);
  }
}
