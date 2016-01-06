#include <avr/wdt.h>
#include <WiFi.h>
#include <DateTime.h>
#include <DateTimeStrings.h>
#include <String.h>
#include <EEPROM.h>
#include <StateLogger.h>

#define LOGGER_ADDRESS 32
#define TRANSFER_PERIOD 60
#define DEBOUNCE 20

String rxCmd;

char ssid[] = "J_C";     //  your network SSID (name)
char pass[] = "VictorHanny874";  // your network password

IPAddress server_ip(192, 168, 1, 160);

void digitalClockDisplay(long timeStamp);

StateLogger Logger(LOGGER_ADDRESS);

void setup() 
{
  wdt_enable(WDTO_8S);
  
  pinMode(3, INPUT_PULLUP); //PUMP blue
  pinMode(5, OUTPUT); //white
  pinMode(6, OUTPUT); //green
  pinMode(9, OUTPUT); //LED on WiFi SHIELD
  
  digitalWrite(5, HIGH); 
  digitalWrite(6, HIGH); 
  digitalWrite(9, LOW); 
  
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  
  Logger.showAll();
  
  attachInterrupt(digitalPinToInterrupt(3), serviceInputChange, CHANGE);
   
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println("WiFi shield not present");
    // don't continue:
    while(true);
  }
   
  digitalWrite(5, LOW); 
  digitalWrite(6, LOW);   

  Serial.println("Started Wifi Pump Logger");
}

void loop() 
{
  wdt_reset();

  // attempt to connect to Wifi network:
  if(WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(9, LOW); 
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    
    wdt_disable();
    // Connect to WPA/WPA2 network:        
    if(WiFi.begin(ssid, pass) != WL_CONNECTED)    
    {      
      // wait 10 seconds for connection:
      delay(10000);
    }        
    
    wdt_enable(WDTO_8S);
  }

  if(WiFi.status() == WL_CONNECTED)
  {    
    digitalWrite(9, HIGH);
    serviceClientLogs();
  }
}

void serviceInputChange()
{
   delayMicroseconds(16383);
   
  static boolean prevState = false;
  boolean pinState = digitalRead(3);
  
  if(pinState != prevState)
  {
    prevState = pinState;
    
    if(DateTime.available())
    {
      s_event evt(DateTime.now(), 3, pinState);
      Logger.log(&evt);
    }  
    else
    {
      Serial.println("ISR no time");      
    }
  }  
}

void serviceClientLogs()
{
  static long lastUpdate = 0;
    
  if(!DateTime.available() || ((lastUpdate + TRANSFER_PERIOD) <= DateTime.now()))
  { 
      printWifiData();
      if(transferToServer())
      {        
        //Logger.clear();
        
        DateTime.available();
        lastUpdate = DateTime.now();
        digitalClockDisplay();
      }
  } 
}

boolean transferToServer()
{      
  WiFiClient client;
  boolean txStatus = false;
    
  if(client.connect(server_ip, 23))
  {
    Serial.println("CONNECTED");

    if(client.connected())
    {
      replyStatus(&client);
    }
  }  
  else
  {    
    Serial.println("ERROR");
    client.stop();

    return false;
  }

 DateTime.available();
 long  startTransfer = DateTime.now();
 
  do
  {
    txStatus = serviceServerData(&client);
    if(txStatus)
      break;
      
    //wait 10s for reply from server
    DateTime.available();
    if(DateTime.now() > (startTransfer + 10))
    {
      Serial.println("time Out");
      break;
    }    
  }while(!txStatus); 
      
  client.stop();  
  return txStatus;
}

void replyStatus(WiFiClient * client)
{         
    byte mac[6];
    WiFi.macAddress(mac);
    String macStr = getMACstring(mac);

    client->flush();
    client->print("RSSI: ");
    client->println(WiFi.RSSI());
    client->println(macStr);      
  
    String statusStr;
    statusStr += getStateString(3);
    statusStr += getStateString(5);
    statusStr += getStateString(6);
    client->println(statusStr);  

    replyLogs(client);
}

void replyLogs(WiFiClient * client)
{ 
  Logger.reset();
  
  while(1)
  {
    s_event evt = Logger.getEvent();
    if(!evt.crc)
      break;
    
      String log = evt.getString();
      client->println(log);
  } 
}

boolean serviceServerData(WiFiClient * client)
{  
  static String rxString;
  boolean ackFlag = false;
  
  while (client->available()) 
  {
    char c = client->read();
    //Serial.write(c);
  
    rxString += String(c);
  
    if(rxString.endsWith("\n") || rxString.endsWith("\r"))
    {
      rxString.trim();
  
      if(rxString.length() < 2)
        continue;
      
      //Serial.print("RX: ");
      //Serial.println(rxString);
  
      if(rxString.charAt(0) == 'S')
      {
        int port = rxString.substring(1).toInt();
        
        if((port == 5) || (port == 6))
        {
            Serial.print("Set ");
            Serial.println(port);
  
            digitalWrite(port, HIGH);
        }
      }
  
      if(rxString.charAt(0) == 'R')
      {
        int port = rxString.substring(1).toInt();
        
        if((port == 5) || (port == 6))
        {
            Serial.print("Reset ");
            Serial.println(port);
  
            digitalWrite(port, LOW);
        }
      }
  
      if(rxString.charAt(0) == 'T')
      {  
        long timeStamp = rxString.substring(1).toInt();          
        DateTime.sync(timeStamp);
        
        ackFlag = true;
      }

      if(rxString.charAt(0) == 'A')
      { 
        Logger.reset();
         
        int ack = rxString.substring(1).toInt();          
        Serial.print("Ack: ");
        Serial.println(ack);
          
        for(uint8_t k = 0; k < ack; k++)
        {
          Logger.ack();  
        }
      }
      
      rxString = String();
    }      
  }

  return ackFlag;
}


String getMACstring(byte mac[6])
{
  String macString;
  
  macString += String(mac[5],HEX);
  macString += String(":");
  macString += String(mac[4],HEX);
  macString += String(":");
  macString += String(mac[3],HEX);
  macString += String(":");
  macString += String(mac[2],HEX);
  macString += String(":");
  macString += String(mac[1],HEX);
  macString += String(":");
  macString += String(mac[0],HEX);

  return macString;
}

String getStateString(int port)
{
  String str;

  if(digitalRead(port))
  {
    str += String("S");
  }
  else
  {
    str += String("R");
  }
  
  str += String(port);
  str += String("\r\n");
  
  return str;
}


void printWifiData() {
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
  Serial.println(ip);

  ip = WiFi.subnetMask();
    Serial.print("Mask: ");
  Serial.println(ip);

  ip = WiFi.gatewayIP();
    Serial.print("Gateway: ");
  Serial.println(ip);
 
  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  String macStr = getMACstring(mac);
  Serial.print("MAC address: ");
  Serial.println(macStr);
 
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);    
  String bssStr = getMACstring(bssid);
  Serial.print("BSSID: ");
  Serial.println(bssStr);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption,HEX);
  Serial.println();
}

void digitalClockDisplay()
{
  // digital clock display of current time
  digitalClockDisplay(DateTime.Hour, 
                      DateTime.Minute,
                      DateTime.Second,
                      DateTime.Day,
                      DateTime.DayofWeek,
                      DateTime.Month,
                      DateTime.Year);                      
}

void digitalClockDisplay(long timeStamp)
{
   byte h;
   byte m;
   byte s;
   byte D;
   byte wD; // Sunday is day 0 
   byte M;     // Jan is month 0
   byte Y;      // the Year minus 1900   
   DateTime.localTime((time_t*)&timeStamp, &s, &m, &h, &D, &wD, &M, &Y);
   digitalClockDisplay(h,m,s,D,wD,M,Y);
}

void digitalClockDisplay(byte h, byte m, byte s, byte D, byte wD, byte M, byte Y)
{  
  Serial.print(h,DEC);  
  printDigits(m);  
  printDigits(s);
  Serial.print(" ");
  Serial.print(DateTimeStrings.dayStr(wD));
  Serial.print(" ");
  Serial.print(D, DEC);  
  Serial.print(" ");    
  Serial.print(DateTimeStrings.monthStr(M));  
  Serial.print(" ");
  Serial.println(Y);  
}

void printDigits(byte digits){
  // utility function for digital clock display: prints colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits,DEC);  
}
