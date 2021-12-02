//change line 148 to the proper IP of the Arduino
HardwareSerial & dbgTerminal = Serial; 
HardwareSerial & espSerial = Serial3; 

#include <DHT.h>;

#define BUFFER_SIZE 256          // Set the Buffer Size (should be updated According to Arduino Board Type) 
char buffer[BUFFER_SIZE]; 

#define DHTPIN 7     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

//Variables
int chk;
//float hum;  //Stores humidity value
float temp; //Stores temperature value
String stringOne;
 
 void setup() {  
  
   dbgTerminal.begin(9600); // Serial monitor 
   espSerial.begin(115200); // ESP8266 
     dht.begin();

       
   delay(2000); 
    
   clearSerialBuffer();  //Function to Clear the ESP8266 Buffer 
    
   //connect to WIFI router 
   connectWiFi("SSID", "PASSWORD"); 
   delay(10000);
   
   //test if the module is ready  
   dbgTerminal.print("AT : "); 
   dbgTerminal.println( GetResponse("AT",100) ); 
      
   //Change to mode 1 (Client Mode) 
   dbgTerminal.print("AT+CWMODE=1 : "); 
   dbgTerminal.println( GetResponse("AT+CWMODE=1",10) ); 
          
   //set the multiple connection mode 
   dbgTerminal.print(F("AT+CIPMUX=1 : ")); 
   dbgTerminal.println( GetResponse("AT+CIPMUX=1",10) ); 
    
   //set the server of port 80 check "no change" or "OK",  it can be changed according to your configuration 
   dbgTerminal.print(F("AT+CIPSERVER=1,8888 : ")); 
   dbgTerminal.println( GetResponse("AT+CIPSERVER=1,8888", 10) ); 
    
    //print the ip addr 
   dbgTerminal.print(F("ip address : ")); 
   dbgTerminal.println( GetResponse("AT+CIFSR", 20) ); 
   delay(200); 
  
   dbgTerminal.println(); 
   dbgTerminal.println(F("Starting Webserver"));   
 } 
 
 void loop() { 

   int ch_id, packet_len; 
   char *pb;   
   espSerial.readBytesUntil('\n', buffer, BUFFER_SIZE); 
    
   if(strncmp(buffer, "+IPD,", 5)==0) { 
     // request: +IPD,ch,len:data 
     sscanf(buffer+5, "%d,%d", &ch_id, &packet_len); 
     if (packet_len > 0) { 
       // read serial until packet_len character received 
       // start from : 
       pb = buffer+5; 
       while(*pb!=':') pb++; 
       pb++; 

         if (strncmp(pb, "GET / ", 6) == 0) { 
         dbgTerminal.print(millis()); 
         dbgTerminal.print(" : "); 
         dbgTerminal.println(buffer); 
         dbgTerminal.print( "get Status from ch:" ); 
         dbgTerminal.println(ch_id);       
         delay(100); 
         clearSerialBuffer();
         
         temp= dht.readTemperature();
         Serial.println(temp);
         stringOne = String(temp, 1);
         
         homepage1(ch_id); 
       }

        
     } 
   } 
   clearBuffer(); 
 } 
 

 // Get the data from the WiFi module and send it to the debug serial port 
 String GetResponse(String AT_Command, int wait){ 
   String tmpData; 
    
   espSerial.println(AT_Command); 
   delay(10); 
   while (espSerial.available() >0 )  { 
     char c = espSerial.read(); 
     tmpData += c; 
      
     if ( tmpData.indexOf(AT_Command) > -1 )          
       tmpData = ""; 
     else 
       tmpData.trim();        
            
    } 
    return tmpData; 
 } 
 
 void clearSerialBuffer(void) { 
        while ( espSerial.available() > 0 ) { 
          espSerial.read(); 
        } 
 } 
 
 void clearBuffer(void) { 
        for (int i =0;i<BUFFER_SIZE;i++ ) { 
          buffer[i]=0; 
        } 
 } 
           
 boolean connectWiFi(String NetworkSSID,String NetworkPASS) { 
   String cmd = "AT+CWJAP=\""; 
   cmd += NetworkSSID; 
   cmd += "\",\""; 
   cmd += NetworkPASS; 
   cmd += "\""; 
    
   dbgTerminal.println(cmd);  
   dbgTerminal.println(GetResponse(cmd,10)); 
 } 

void homepage1(int ch_id) { 

   String Header; 
 
   Header =  "HTTP/1.1 200 OK\r\n"; 
   Header += "Content-Type: text/html\r\n"; 
   Header += "Connection: close\r\n";   
   Header += "Refresh: 600;URL='//192.168.10.113:8888/'>\r\n"; 
    
   String Content; 

           Content ="<HTML>\r\n";
           Content +="<HEAD>\r\n";
           Content +="<meta name='apple-mobile-web-app-capable' content='yes' />\r\n";
           Content +="<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />\r\n";
           Content +="<link rel='stylesheet' type='text/css' href='http://randomnerdtutorials.com/ethernetcss.css' />\r\n";
           Content +="<TITLE>Temperatur</TITLE>\r\n";
           Content +="</HEAD>\r\n";
           Content +="<BODY style='background-color:black;'>\r\n";
           Content +="<br />\r\n";
           Content +="<H2 style='font-size:200px;color:white;'>" + stringOne + " &#186C</H2>\r\n"; //temperatur
           Content +="<br />\r\n";           
           Content +="</BODY>\r\n";
           Content +="</HTML>\r\n";
    
   Header += "Content-Length: "; 
   Header += (int)(Content.length()); 
   Header += "\r\n\r\n"; 
    
   espSerial.print("AT+CIPSEND="); 
   espSerial.print(ch_id); 
   espSerial.print(","); 
   espSerial.println(Header.length()+Content.length()); 
   delay(10); 
    
   if (espSerial.find(">")) { 
       espSerial.print(Header); 
       espSerial.print(Content); 
       delay(10); 
    } 
 }
