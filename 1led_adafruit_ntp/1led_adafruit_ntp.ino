// import adafruit
#include <Adafruit_NeoPixel.h>
// import Wifimanager
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
// import WEB server
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
// import NTP client
#include <NTPClient.h>
#include <TimeLib.h>

// TODO init variables
byte clockMode = 0; 
int period = 1000;   //Update frequency
unsigned long time_now = 0;
byte brightness = 100;
byte hourFormat = 24; 
byte r_val = 0;
byte g_val = 0;
byte b_val = 255;
bool dotsOn = true;
long numbers[] = {
  0b00111111,  // [0] 0                       ///jika persegment 7 led maka setelah 0b diletakan angka 0
  0b00100001,  // [1] 1
  0b01110110,  // [2] 2
  0b01110011,  // [3] 3
  0b01101001,  // [4] 4
  0b01011011,  // [5] 5
  0b01011111,  // [6] 6
  0b00110001,  // [7] 7
  0b01111111,  // [8] 8
  0b01111011,  // [9] 9
  0b00000000,  // [10] off
  0b01111000,  // [11] degrees symbol
  0b00011110,  // [12] C(elsius)
  0b01011100,  // [13] F(ahrenheit)
};
int SLEEP_HOU = 22;
int SLEEP_MIN= 0;
int WAKE_HOU = 5;
int WAKE_MIN = 0;

// init wifi 
MDNSResponder mdns;
ESP8266WebServer server(80);
WiFiUDP ntpUDP;

// init ntp 
NTPClient timeClient(ntpUDP, "time.nist.gov", 7*3600, 60000); //GMT+5:30 : 5*3600+30*60=19800

// init led and color
#define LED_PIN D4
#define LED_COUNT 30
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint32_t alternateColor = strip.Color(0, 0, 0); 
uint32_t blackColor = strip.Color(0, 0, 0); 

// TODO router handler functions 
void clockHandler() {       
  clockMode = 0;     
  server.send(200, "text/json", "{\"result\":\"ok\"}");
}
void colorHandler() {    
  r_val = server.arg("r").toInt();
  g_val = server.arg("g").toInt();
  b_val = server.arg("b").toInt();
  server.send(200, "text/json", "{\"result\":\"ok\"}");
}
void brightnessHandler() {    
  brightness = server.arg("brightness").toInt();    
  server.send(200, "text/json", "{\"result\":\"ok\"}");
}

// TODO implement helper functions
void updateClock() {  
  timeClient.update();
  unsigned long unix_epoch = timeClient.getEpochTime();   // get UNIX Epoch time
  int sec = second(unix_epoch);                            // get seconds
  int minu = minute(unix_epoch);                            // get minutes
  int hou  = hour(unix_epoch);
  byte hour_formated = hou;
  if (hourFormat == 12 && hou > 12)
    hour_formated = hour_formated - 12;
  byte h1 = hour_formated / 10;
  byte h2 = hour_formated % 10;
  byte m1 = minu / 10;
  byte m2 = minu % 10;  
  // byte s1 = Second / 10;
  // byte s2 = Second % 10;
  
  uint32_t color = strip.Color(r_val, g_val, b_val);

  if (h1 > 0)
    displayNumber(h1,3, color);
  else 
    displayNumber(10,3, color);  // Blank
    
  displayNumber(h2,2, color);
  displayNumber(m1,1, color);
  displayNumber(m2,0, color); 

  displayDots(color);  
  updateBrightnessByMoment(hou, minu);
}
void updateBrightnessByMoment(int hou, int minu) {
//int SLEEP_HOU = 22;
//int SLEEP_MIN= 0;
//int WAKE_HOU = 5;
//int WAKE_MIN = 0;
  if (hou == WAKE_HOU && minu == WAKE_MIN) {
    brightness = 100;
  }
  if (hou == SLEEP_HOU && minu == SLEEP_MIN) {
    brightness = 1;
  }
}
void displayNumber(byte number, byte segment, uint32_t color) {
  /*
   
      __ __ __        __ __ __          __ __ __        _ 4 _  
    __        __    __        __      __        __    _        _
    __        __    __        __      __        __    _3        5
    __        __    __        __  15  __        __    _        _
      __ __ __        __ __ __          __ __ __         _ 6 _  
    __        23    __        16  14  __        __    _        _
    __        __    __        __      __        7    _2        _0
    __        __    __        __      __        __    _        _
      __ __ __       __ __ __           __ __ __       _  _1 _   

   */
 
  // segment from left to right: 3, 2, 1, 0
  byte startindex = 0;
  switch (segment) {
    case 0:
      startindex = 0;
      break;
    case 1:
      startindex = 7;                 //// value start index digit led no 2. 
      break;
    case 2:
      startindex = 16;
      break;
    case 3:
      startindex = 23;
      break;    
  }

  for (byte i=0; i<7; i++){             //// value start index digit led no 2.
    yield();
    uint32_t color_set = ((numbers[number] & 1 << i) == 1 << i) ? color : alternateColor;
    strip.setPixelColor(i + startindex, color_set);
  } 
}

void displayDots(uint32_t color) {
  uint32_t colorSet = dotsOn ? color : blackColor;
  strip.setPixelColor(14, colorSet);
  strip.setPixelColor(15, colorSet);

  dotsOn = !dotsOn;  
}

void setup() {
  // put your setup code here, to run once:
  // init led 
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  // connect wifi 
  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("AutoConnectAP"); // password protected ap

  if(!res) {
      Serial.println("Failed to connect");
      ESP.restart();
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("connected...yeey :)");
  }

  // setup webserver 
  Serial.println(WiFi.localIP());
  if (mdns.begin("esp8266", WiFi.localIP()))
  Serial.println("MDNS responder started");
  // TODO setup router
  server.on("/clock", clockHandler);
  server.on("/color", colorHandler);
  server.on("/brightness", brightnessHandler);

  // setup SPIFFS contents upload 
  // Before uploading the files with the "ESP8266 Sketch Data Upload" tool, zip the files with the command "gzip -r ./data/" (on Windows I do this with a Git Bash)
  // *.gz files are automatically unpacked and served from your ESP (so you don't need to create a handler for each file).
  server.serveStatic("/", SPIFFS, "/", "max-age=86400");
  server.begin();     

  SPIFFS.begin();
  Serial.println("SPIFFS contents:");
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), String(fileSize).c_str());
  }
  // TODO start time client
  timeClient.begin();
  delay(10);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.status() == WL_CONNECTED) { // check WiFi connection status
    // int sensor_val = analogRead(LDR_PIN);
    // Brightness =40;
    server.handleClient();
    // TODO remove gettime
    while (millis() > time_now + period) {
      time_now = millis();
      if (clockMode == 0) {
        updateClock();
      } else if (clockMode == 1) {
        // TODO complete
        // updateCountdown();  
      } else if (clockMode == 3) {
        // TODO complete
        // updateScoreboard();            
      }
  
      strip.setBrightness(brightness);
      strip.show();
    }
  }

}
