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
bool isRainBow = 0;
byte scoreboardLeft = 0;
byte scoreboardRight = 0;
// init wifi 
// Set your Static IP address
IPAddress local_IP(192, 168, 1, 60);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

MDNSResponder mdns;
ESP8266WebServer server(80);
WiFiUDP ntpUDP;

// init ntp 
NTPClient timeClient(ntpUDP, "time.nist.gov", 7*3600, 60000); //GMT+5:30 : 5*3600+30*60=19800

// init led and color
#define LED_PIN D4
#define LED_COUNT 30
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint32_t BLACK_COLOR = strip.Color(0, 0, 0); 
uint32_t RED_COLOR = strip.Color(255, 0, 0); 
uint32_t GREEN_COLOR = strip.Color(0, 255, 0); 
uint32_t BLUE_COLOR = strip.Color(0, 0, 255); 
uint32_t ALTERNATE_COLOR = BLACK_COLOR; 
uint32_t scoreboardColorLeft = RED_COLOR;
uint32_t scoreboardColorRight = GREEN_COLOR;
// variables for countdown
uint32_t countdownColor = strip.Color(0, 255, 0); 
unsigned long countdownMilliSeconds;
unsigned long endCountDownMillis;

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
void countdownHandler() {    
  countdownMilliSeconds = server.arg("ms").toInt();     
  byte cd_r_val = server.arg("r").toInt();
  byte cd_g_val = server.arg("g").toInt();
  byte cd_b_val = server.arg("b").toInt();
  countdownColor = strip.Color(cd_r_val, cd_g_val, cd_b_val); 
  endCountDownMillis = millis() + countdownMilliSeconds;
  strip.clear();
  clockMode = 1;     
  server.send(200, "text/json", "{\"result\":\"ok\"}");
}
void rainbowHandler() {
  isRainBow = server.arg("isRainbow").toInt();
}
void scoreboardHandler() {   
  scoreboardLeft = server.arg("left").toInt();
  scoreboardRight = server.arg("right").toInt();
  scoreboardColorLeft = strip.Color(server.arg("rl").toInt(),server.arg("gl").toInt(),server.arg("bl").toInt());
  scoreboardColorRight = strip.Color(server.arg("rr").toInt(),server.arg("gr").toInt(),server.arg("br").toInt());
  clockMode = 3;     
  server.send(200, "text/json", "{\"result\":\"ok\"}");
}
void hourformatHandler() {   
  hourFormat = server.arg("hourformat").toInt();
  clockMode = 0;     
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
    uint32_t color_set;
    if(isRainBow) {
      uint32_t color_rain;
      switch (i%3)
      {
      case 0:
        color_rain = RED_COLOR;
        break;
      case 1:
        color_rain = GREEN_COLOR;
        break;
      case 2:
        color_rain = BLUE_COLOR;
        break;
      }
      color_set = color_rain;
    } else {
      color_set = color;
    }
    color_set = ((numbers[number] & 1 << i) == 1 << i) ? color_set : ALTERNATE_COLOR;
    strip.setPixelColor(i + startindex, color_set);
  } 
}
void updateCountdown() {

  if (countdownMilliSeconds == 0 && endCountDownMillis == 0) 
    return;
    
  unsigned long restMillis = endCountDownMillis - millis();
  unsigned long hours   = ((restMillis / 1000) / 60) / 60;
  unsigned long minutes = (restMillis / 1000) / 60;
  unsigned long seconds = restMillis / 1000;
  int remSeconds = seconds - (minutes * 60);
  int remMinutes = minutes - (hours * 60); 
  
  Serial.print(restMillis);
  Serial.print(" ");
  Serial.print(hours);
  Serial.print(" ");
  Serial.print(minutes);
  Serial.print(" ");
  Serial.print(seconds);
  Serial.print(" | ");
  Serial.print(remMinutes);
  Serial.print(" ");
  Serial.println(remSeconds);

  byte h1 = hours / 10;
  byte h2 = hours % 10;
  byte m1 = remMinutes / 10;
  byte m2 = remMinutes % 10;  
  byte s1 = remSeconds / 10;
  byte s2 = remSeconds % 10;

  uint32_t color = countdownColor;
  if (restMillis <= 60000) {
    color = strip.Color(255, 0, 0);
  }

  if (hours > 0) {
    // hh:mm
    displayNumber(h1,3,color); 
    displayNumber(h2,2,color);
    displayNumber(m1,1,color);
    displayNumber(m2,0,color);  
  } else {
    // mm:ss   
    displayNumber(m1,3,color);
    displayNumber(m2,2,color);
    displayNumber(s1,1,color);
    displayNumber(s2,0,color);  
  }

  displayDots(color);  

  if (hours <= 0 && remMinutes <= 0 && remSeconds <= 0) {
    Serial.println("Countdown timer ended.");
    endCountdown();
    countdownMilliSeconds = 0;
    endCountDownMillis = 0;
//    digitalWrite(COUNTDOWN_OUTPUT, HIGH);
//    delay(500);
//    digitalWrite(COUNTDOWN_OUTPUT,LOW);
//    delay(500);
//    digitalWrite(COUNTDOWN_OUTPUT, HIGH);
//    delay(500);
//    digitalWrite(COUNTDOWN_OUTPUT,LOW);
//    delay(500);
//    digitalWrite(COUNTDOWN_OUTPUT, HIGH);
//    delay(500);
//    digitalWrite(COUNTDOWN_OUTPUT,LOW);
//    delay(500);
    return;
  }  
}
void endCountdown() {
  strip.clear();
  for (int i=0; i<LED_COUNT; i++) {
    if (i>0)
      strip.setPixelColor(i-1, BLACK_COLOR);
    uint32_t colorSet;
    switch(i%3) {
      case 0:
        colorSet = RED_COLOR;
        break;
      case 1:
        colorSet = GREEN_COLOR;
        break;
      case 2:
        colorSet = BLUE_COLOR;
        break;  
    }
    strip.setPixelColor(i, colorSet);
    strip.show();
    delay(25);
  }  
}
void updateScoreboard() {
  byte sl1 = scoreboardLeft / 10;
  byte sl2 = scoreboardLeft % 10;
  byte sr1 = scoreboardRight / 10;
  byte sr2 = scoreboardRight % 10;

  displayNumber(sl1,3,scoreboardColorLeft);
  displayNumber(sl2,2,scoreboardColorLeft);
  displayNumber(sr1,1,scoreboardColorRight);
  displayNumber(sr2,0,scoreboardColorRight);
  hideDots();
}

void displayDots(uint32_t color) {
  uint32_t colorSet = dotsOn ? color : BLACK_COLOR;
  strip.fill(colorSet, 14, 2);

  dotsOn = !dotsOn;  
}

void hideDots() {
  strip.fill(BLACK_COLOR, 14, 2);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // init led 
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  // connect wifi 
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
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
  server.on("/countdown", countdownHandler);
  server.on("/toogleRainbow", rainbowHandler);
  server.on("/scoreboard", scoreboardHandler);
  server.on("/hourformat", hourformatHandler);

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
        updateCountdown();  
      } else if (clockMode == 3) {
        updateScoreboard();            
      }
  
      strip.setBrightness(brightness);
      strip.show();
    }
  }

}
