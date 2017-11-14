#include <SoftwareSerial.h>
#include "DHT.h"

//*-- Hardware esp1
#define _baudrate   9600

//*-- Software esp1
// 
#define _rxpin      2
#define _txpin      3
#define DHTPIN 6     // what pin we're connected to
#define dbg Serial
//SoftwareSerial dbg( _rxpin, _txpin ); // RX, TX
SoftwareSerial esp1( _rxpin, _txpin ); // RX, TX
#define DHTTYPE DHT11   // DHT 11 
#define SSID ""   // 分享器的SSID
#define PASS ""  // 分享器的密碼

//*-- IoT Information
const String APIKEYWRITE = "";   // ThingSpeak API Key
const String APIKEYREAD = "";   // ThingSpeak API Key
const String CHANNELID = "";  // ThingSpeak Channel Id
const String IP = "api.thingspeak.com"; // ThingSpeak IP Address: 184.106.153.149
const String GET = "GET /update";
long nowTime;
boolean isSet = true;
long nowTime2;
uint8_t dhtbuf[2];
boolean isConnecting = false;
String msg = "";
DHT dht(DHTPIN, DHTTYPE);

void setup() {
    dbg.begin( _baudrate );    
    esp1.begin(_baudrate);
    dht.begin();
    dhtbuf[0] = 0;
    dhtbuf[1] = 0;
    nowTime = millis();
}

void loop() {
  while(!isConnecting){
      sendSerial("AT\r\n");
      delay(5000);
      if(esp1.find("OK"))
      {
         dbg.println("Connecting OK");
         connectWiFi();
      }  
      if(esp1.available()){
        Serial.println(esp1.read());
      }
  }
  
  if(millis()-nowTime>20000){    
    if(isSet){
        esp_close();
        delay(100);
        getHTData();
        //isSet = false;
    }else{
        esp_close();
        delay(100);
        getData();    
        isSet = true;      
    }
    nowTime = millis();
  }
  while(esp1.available()){
        char c = esp1.read();      
        msg += c;
  }    
  if(msg != ""){
      dbg.print(msg);
      msg="";
  }
//    delay(10000);   // 60 second
}
void getHTData()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit
  float f = dht.readTemperature(true);
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    dbg.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index
  // Must send in temp in Fahrenheit!
    float hi = dht.computeHeatIndex(f, h);    
    dhtbuf[0] = h;
    dhtbuf[1] = t;
    /*
    dbg.print("Humidity: "); 
    dbg.print(h);
    dbg.print(" %\t");
    dbg.print("Temperature: "); 
    dbg.println(t);    
    */
    char buf[3];
    String HH, TT;
    buf[0] = 0x30 + dhtbuf[1] / 10;
    buf[1] = 0x30 + dhtbuf[1] % 10;
    TT = (String(buf)).substring( 0, 2 );
    buf[0] = 0x30 + dhtbuf[0] / 10;
    buf[1] = 0x30 + dhtbuf[0] % 10;
    HH = (String(buf)).substring( 0, 2 );
    //updateDHT11( TT, HH );
    setData(TT, HH);  
}

void sendSerial(String cmd)
{
    esp1.print(cmd);
//    dbg.print(cmd);
} 
 
boolean connectWiFi()
{
    esp1.println("AT+CWMODE=1");
    delay(2000);
    String cmd="AT+CWJAP=\"";
    cmd+=SSID;
    cmd+="\",\"";
    cmd+=PASS;
    cmd+="\"";
    sendSerial(cmd+"\r\n");
    delay(5000);
    if(esp1.find("OK"))
    {
        isConnecting = true;
        dbg.println("Connect OK");
        return true;
    }
    else
    {
        dbg.println("Connect Error");
        return false;
    }

    cmd = "AT+CIPMUX=0";
    sendSerial(cmd+"\r\n");
    if( esp1.find( "Error") )
    {
        dbg.println("Error2");
        return false;
    }
  }

void updateDHT11( String T, String H )
{
    // 設定 ESP8266 作為 Client 端
    dbg.println("update");
    if(esp_ConnectIP(IP)){
        String cmd = "";
        String postStr = APIKEYWRITE + "&field1=" +T + "&field2=" + H + "\r\n\r\n";
        cmd = "POST /update HTTP/1.1\r\n";
        cmd += "Host: api.thingspeak.com\r\n" ;
        cmd +="Connection: close\r\n";
        cmd +="X-THINGSPEAKAPIKEY: "+APIKEYWRITE+"\r\n";
        cmd +="Content-Type: application/x-www-form-urlencoded\r\n";
        cmd +="Content-Length: ";
        cmd +=postStr.length();
        cmd +="\r\n";
        cmd +=postStr;    
        cmd +="\r\n";    
        esp_sendData(cmd);    
    }else{
       dbg.println("SEND: ERROR");    
    }
}


void setData(String T,String H)
{
    dbg.println("setData");  
    if(esp_ConnectIP(IP)){
        String cmd = GET+"?key="+APIKEYWRITE+"&field1="+T+"&field2="+H+"\r\n";
        dbg.println("SetData OK");
        esp_sendData(cmd);
        if(esp1.find("OK")){
           dbg.println("SEND-GET: OK");
         }else{
           dbg.println("SEBND-GET ERROR");
         }      
    }else{
        dbg.print( "RECEIVED: Error\nExit1" );
    }
}

void getData()
{
    if(esp_ConnectIP(IP))
    {      
        String cmd = "GET /channels/39446/fields/1/last?key=QYBPH6E9HEIMPX5H HTTP/1.1\r\nHost: api.thingspeak.com\r\nConnection: keep-alive\r\n\r\n";
        esp_sendData(cmd);
        if(esp1.find("OK")){
           dbg.println("RECEIVED-GET: OK");
         }else{
           dbg.println("RECEIVED: ERROR");
         }              
    }else{
        dbg.print( "RECEIVED: Error\nExit1" );
    }
}

boolean esp_ConnectIP(String ip){
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    boolean isOK = false;
    cmd += IP;
    cmd += "\",80";
    sendSerial(cmd+"\r\n");  
    if( esp1.find( "OK" )){
        dbg.println("ConnectIP OK");
        isOK = true;
    }else{                  
        dbg.println( "ConnectIP Error" );
    }
    return isOK;
}

void esp_sendData(String cmd){
    sendSerial("AT+CIPSEND=");
    sendSerial(String(cmd.length()));
    dbg.print("AT+CIPSEND=");
    dbg.println(String(cmd.length()));
    sendSerial("\r\n");
    delay(100);
    if(esp1.find( ">" ) ){
        dbg.println("CMD>");
        sendSerial(cmd);
        dbg.print(cmd);
    }              
    delay(1000);      
}

void esp_close(){
   esp1.print("AT+CIPCLOSE\r\n"); 
   dbg.println("ESP_CLOSE");
}
