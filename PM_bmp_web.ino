#include <WiFi.h> 
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include "time.h"
#include <WiFiClient.h>

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;
struct tm timeinfo;

int PM25_1, PM25_2;
int PM10_1, PM10_2;
bool HPMAstatus = false;


Adafruit_BMP085 bmp;

const char* ssid = "W6MRR-AREDN"; 
const char* password = "Beaueau0";

WiFiServer server(80);

String header;

void printLocalTime();  //funtion protypes
bool receive_measurement_1 (void);
bool receive_measurement_2 (void);
bool start_autosend_1(void);
bool start_autosend_2(void);


void setup() 
{
Serial.begin(115200);
Serial1.begin(9600, SERIAL_8N1, 16, 17);
Serial2.begin(9600, SERIAL_8N1, 18, 19);

if (!bmp.begin()) 
{
Serial.println("Not connected with BMP180/BMP085 sensor, check connections ");
while (1) {}
}

Serial.print("Connecting to Wifi Network");
Serial.println(ssid);
WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}
Serial.println("");
Serial.println("Successfully connected to WiFi.");
Serial.println("IP address of ESP32 is : ");
Serial.println(WiFi.localIP());
server.begin();
Serial.println("Server started");
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

    start_autosend_1();
    start_autosend_2();

}

void loop() {
printLocalTime();


Serial.print("Temp = ");
Serial.print(bmp.readTemperature());
Serial.println(" *C");

Serial.print("Pressure = ");
Serial.print(bmp.readPressure());
Serial.println(" Pa");


Serial.print("Altitude = ");
Serial.print(bmp.readAltitude());
Serial.println(" meters");


Serial.println();
delay(500);

   HPMAstatus = receive_measurement_1();
    if (!HPMAstatus) {
      Serial.println("Cannot receive data from Serial1!");
    }


    HPMAstatus = receive_measurement_2();
    if (!HPMAstatus) {
      Serial.println("Cannot receive data from Serial2!");
    }
   
   Serial.println("PM 2.5:\t" + String(PM25_1) + " " + String(PM25_2) + " ug/m3");
   Serial.println("PM 10:\t" + String(PM10_1) + " " + String(PM10_2) + " ug/m3");


if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
}

WiFiClient client = server.available();

if (client) 
{ 
Serial.println("Web Client connected ");
String request = client.readStringUntil('\r'); 
client.println("HTTP/1.1 200 OK");
client.println("Content-type:text/html");
client.println("Connection: close");
client.println();
client.println("<!DOCTYPE html><html>");
client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
client.println("<link rel=\"icon\" href=\"data:,\">");
client.println("</style></head><body><h1>W6MRR Sensor Farm</h1>");

//client.println("<tr><td>GMT</td><td><span class=\"sensor\">");
client.println(&timeinfo, "%A, %B %d %Y %H:%M:%S GMT");

client.println("<h2>HPMA115S0 Particle Sensor</h2>");

client.println("<table><tr><th></th><th>PM2.5</th><th>PM10</th></tr>");

client.println("<tr><td>Sensor 1  </td><td><span class=\"sensor\">");
client.println(PM25_1);
client.println(PM10_1);
client.println(" ug/m3</span></td></tr>"); 
client.println("<tr><td>Sensor 2  </td><td><span class=\"sensor\">");
client.println(PM25_2);
client.println(PM10_2);
client.println(" ug/m3</span></td></tr>"); 
client.println(" </table>");

client.println("<h2>BMP180 Barometic sensor</h2>");
client.println("<table><tr><th>MEASUREMENT</th><th>VALUE</th></tr>");

client.println("<tr><td>Temp. Celsius</td><td><span class=\"sensor\">");
client.println(bmp.readTemperature());
client.println(" *C</span></td></tr>"); 

client.println("<tr><td>Pressure</td><td><span class=\"sensor\">");
client.println(bmp.readPressure());
client.println(" pa</span></td></tr>"); 

client.println("<tr><td>Altitude</td><td><span class=\"sensor\">");
client.println(bmp.readAltitude());
client.println(" meters</span></td></tr>");
 
client.println("</body></html>"); 
client.println();
Serial.println("Client disconnected.");
Serial.println("");
}
}

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

bool receive_measurement_1 (void)
{

  while (Serial1.available() < 32);

  byte HEAD0 = Serial1.read();
  byte HEAD1 = Serial1.read();

  while (HEAD0 != 0x42) {
    if (HEAD1 == 0x42) {
      HEAD0 = HEAD1;
      HEAD1 = Serial1.read();
    } else {
      HEAD0 = Serial1.read();
      HEAD1 = Serial1.read();
    }
  }

  if (HEAD0 == 0x42 && HEAD1 == 0x4D) {
    byte LENH = Serial1.read();
    byte LENL = Serial1.read();
    byte Data0H = Serial1.read();
    byte Data0L = Serial1.read();
    byte Data1H = Serial1.read();
    byte Data1L = Serial1.read();
    byte Data2H = Serial1.read();
    byte Data2L = Serial1.read();
    byte Data3H = Serial1.read();
    byte Data3L = Serial1.read();
    byte Data4H = Serial1.read();
    byte Data4L = Serial1.read();
    byte Data5H = Serial1.read();
    byte Data5L = Serial1.read();
    byte Data6H = Serial1.read();
    byte Data6L = Serial1.read();
    byte Data7H = Serial1.read();
    byte Data7L = Serial1.read();
    byte Data8H = Serial1.read();
    byte Data8L = Serial1.read();
    byte Data9H = Serial1.read();
    byte Data9L = Serial1.read();
    byte Data10H = Serial1.read();
    byte Data10L = Serial1.read();
    byte Data11H = Serial1.read();
    byte Data11L = Serial1.read();
    byte Data12H = Serial1.read();
    byte Data12L = Serial1.read();
    byte CheckSumH = Serial1.read();
    byte CheckSumL = Serial1.read();
    if (((HEAD0 + HEAD1 + LENH + LENL + Data0H + Data0L + Data1H + Data1L + Data2H + Data2L + Data3H + Data3L + Data4H + Data4L + Data5H + Data5L + Data6H + Data6L + Data7H + Data7L + Data8H + Data8L + Data9H + Data9L + Data10H + Data10L + Data11H + Data11L + Data12H + Data12L) % 256) != CheckSumL) {
      Serial.println("Checksum fail");
      return 0;
    }
    PM25_1 = (Data1H * 256) + Data1L;
    PM10_1 = (Data2H * 256) + Data2L;
    return 1;
  }
}



bool receive_measurement_2 (void)
{
  while (Serial2.available() < 32);
  byte HEAD0 = Serial2.read();
  byte HEAD1 = Serial2.read();
  while (HEAD0 != 0x42) {
    if (HEAD1 == 0x42) {
      HEAD0 = HEAD1;
      HEAD1 = Serial2.read();
    } else {
      HEAD0 = Serial2.read();
      HEAD1 = Serial2.read();
    }
  }
  if (HEAD0 == 0x42 && HEAD1 == 0x4D) {
    byte LENH = Serial2.read();
    byte LENL = Serial2.read();
    byte Data0H = Serial2.read();
    byte Data0L = Serial2.read();
    byte Data1H = Serial2.read();
    byte Data1L = Serial2.read();
    byte Data2H = Serial2.read();
    byte Data2L = Serial2.read();
    byte Data3H = Serial2.read();
    byte Data3L = Serial2.read();
    byte Data4H = Serial2.read();
    byte Data4L = Serial2.read();
    byte Data5H = Serial2.read();
    byte Data5L = Serial2.read();
    byte Data6H = Serial2.read();
    byte Data6L = Serial2.read();
    byte Data7H = Serial2.read();
    byte Data7L = Serial2.read();
    byte Data8H = Serial2.read();
    byte Data8L = Serial2.read();
    byte Data9H = Serial2.read();
    byte Data9L = Serial2.read();
    byte Data10H = Serial2.read();
    byte Data10L = Serial2.read();
    byte Data11H = Serial2.read();
    byte Data11L = Serial2.read();
    byte Data12H = Serial2.read();
    byte Data12L = Serial2.read();
    byte CheckSumH = Serial2.read();
    byte CheckSumL = Serial2.read();
    if (((HEAD0 + HEAD1 + LENH + LENL + Data0H + Data0L + Data1H + Data1L + Data2H + Data2L + Data3H + Data3L + Data4H + Data4L + Data5H + Data5L + Data6H + Data6L + Data7H + Data7L + Data8H + Data8L + Data9H + Data9L + Data10H + Data10L + Data11H + Data11L + Data12H + Data12L) % 256) != CheckSumL) {
      Serial.println("Checksum fail");
      return 0;
    }
    PM25_2 = (Data1H * 256) + Data1L;
    PM10_2 = (Data2H * 256) + Data2L;
    return 1;
  }
}

bool start_autosend_1(void)
{
  // Start auto send
  byte start_autosend[] = {0x68, 0x01, 0x40, 0x57 };
  Serial2.write(start_autosend, sizeof(start_autosend));
  Serial2.flush();
  delay(500);
  //Then we wait for the response
  while (Serial2.available() < 2);
  byte read1 = Serial2.read();
  byte read2 = Serial2.read();
  // Test the response
  if ((read1 == 0xA5) && (read2 == 0xA5)) {
    // ACK
    return 1;
  }
  else if ((read1 == 0x96) && (read2 == 0x96))
  {
    // NACK
    return 0;
  }
  else return 0;
}


bool start_autosend_2(void)
{
  // Start auto send
  byte start_autosend[] = {0x68, 0x01, 0x40, 0x57 };
  Serial2.write(start_autosend, sizeof(start_autosend));
  Serial2.flush();
  delay(500);
  //Then we wait for the response
  while (Serial2.available() < 2);
  byte read1 = Serial2.read();
  byte read2 = Serial2.read();
  // Test the response
  if ((read1 == 0xA5) && (read2 == 0xA5)) {
    // ACK
    return 1;
  }
  else if ((read1 == 0x96) && (read2 == 0x96))
  {
    // NACK
    return 0;
  }
  else return 0;
}
