/*
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
//#define STASSID "Ace Monster Toys"
//#define STAPSK  "learn42everyday"
#define STASSID "TheM&M's-2.4AC"
#define STAPSK  "Beaueau0"


#endif

#define HWSERIAL Serial 

// function prototypes
bool StopAuto(void);
bool ReadPM(void);

const char *ssid = STASSID;
const char *password = STAPSK;

ESP8266WebServer server(80);

const int led = BUILTIN_LED;   //Low active???


// calculated particle measurement values
unsigned int PM25, PM100;

// warning vaues (set to 9999 for no warning)
// these defaults are based on EPA values for 24 hours
unsigned int WPM25 = 35;
unsigned int WPM100 = 150;

int delaymsecs = 2000; // delay for read (min is <6000 from data sheet)
                       // testing indicates 2 seconds will work


void handleRoot() {
  digitalWrite(led, 0);
  char temp[400];

  snprintf(temp, 400,

           "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>Air Particle Sensor </title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <big>\
    <h1>Air Particle Sensor (ppm)</h1>\
    <h1>PM25: %02d</h1>\
    <h1>PM100: %02d</h1>\
    </big>\
    \
  </body>\
</html>",

            PM25, PM100
          );
  server.send(200, "text/html", temp);

}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(led, 1);
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  //Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  //Serial.println("");
  digitalWrite(led, 0);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }
    digitalWrite(led, 0);

  //Serial.println("");
  //Serial.print("Connected to ");
  //Serial.println(ssid);
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    ;//Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  //Serial.println("HTTP server started");

  HWSERIAL.begin(9600);
  // display template on OLED
  
  // stop HPM default auto send
  while (StopAuto() == false) {
    digitalWrite(led,HIGH);
    ;
  }
    digitalWrite(led,LOW);
  
  // clear error on recovery
  
}

void loop(void) {  
  
  
  delay(delaymsecs);
  
  
  if (ReadPM() == false) {
    digitalWrite(led,LOW);
    // display read error ('e')
    // note: 'e' will be cleared on a succesful read below
  }
  else {
    digitalWrite(led,HIGH);
    server.handleClient();
    MDNS.update();
    // display PM values on the OLED
    // note: we only display 1000s down as
    // per max value from data sheet

  }

  if (PM25 > 1023 || PM100 > 1023){
    while (StopAuto() == false) {
    digitalWrite(led,LOW);
    ;
  }
    digitalWrite(D3,HIGH);
  }

}

bool StopAuto(void) {
  // attempt to stop the default power up auto send
  // if it fails, an 'E' will be displayed on the screen
  // in a terminal loop until a successful transmission
  // Note: this should never happen an indicates a
  //      chronic serial transmission error.
  byte maxattempts = 2; // set this as you like
  byte attempts;
  int R1 = 0, R2 = 0;

  // wait until there is a pause in Auto-Send
  while (HWSERIAL.available()>0)
  {
    HWSERIAL.read();
  }
  attempts = 0;
  bool result = false;
  while (attempts < maxattempts) {
    HWSERIAL.write(0x68);
    HWSERIAL.write(0x01);
    HWSERIAL.write(0x20);
    HWSERIAL.write(0x77);
    // read response
    delay(25);
    // if there are no bytes ready, -1 is returned
    R1 = HWSERIAL.read();
    R2 = HWSERIAL.read();
    if ( (R1 != 0xA5) || (R2 != 0xA5) ) {
      attempts++;
    }
    else {
      attempts = maxattempts;  // force an exit
      result = true;          // with an ok return
    }
  }
  return (result);
}

bool ReadPM(void) {
  // send a read request and get the particle measures in the response
  // if we don't get the bytes back or we have a checksum error
  // return false - else return true

  byte maxattempts = 2;   // set this as you like
  byte attempts = 0;      // attempt counter
  byte R[8];              // response bytes
  int nbytes;             // byte counter
  unsigned long ck;       // for checksum calculation
  bool result = false;

  while (attempts < maxattempts) {
    // send a read request send
    HWSERIAL.write(0x68);
    HWSERIAL.write(0x01);
    HWSERIAL.write(0x04);
    HWSERIAL.write(0x93); //cksum
    delay(25);
    // we want to read 8 bytes of data
    // Pos ACK - 0x40,0x05,0x04
    // DF1,DF2 - 2.5 high / low
    // DF1,DF2 - 10.0 high / low
    // Cksum (mod 256)
    nbytes = 0;
    ck = 0;
    while (HWSERIAL.available() && nbytes < 8) {
      R[nbytes++] = HWSERIAL.read(); // store the byte
    }
    if (nbytes == 8) {
      // we got them and R[7] hold the checksum
      ck = ((65536 - (R[0] + R[1] + R[2] + R[3] + R[4] + R[5] + R[6])) & 255);
      if (ck == R[7]) {
        // everything looks good so calculate the global particle measures
        PM25 = (R[3] * 256) + R[4];
        PM100 = (R[5] * 256) + R[6];
        attempts = maxattempts;
        result = true;
      }
      else {
        //checksum error
        attempts++;
      }
    }
    else {
      // serial tx error [not enough bytes]
      attempts++;
    }
  }
  return (result);
}
