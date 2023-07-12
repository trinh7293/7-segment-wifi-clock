#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <NTPClient.h>
#include <FastLED.h>
#include <TimeLib.h>
#define NUM_LEDS 31                        // Total of 30 LED"s     1X7X4+2=30. 1 pixel/segment
#define DATA_PIN D4                          // Change this if you are using another type of ESP board than a WeMos D1 Mini
#define MILLI_AMPS 800 

MDNSResponder mdns;
ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time.nist.gov", 7*3600, 60000); //GMT+5:30 : 5*3600+30*60=19800
// init wifi 
// Set your Static IP address
IPAddress local_IP(192, 168, 1, 60);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

CRGB LEDs[NUM_LEDS];
int period = 1000;   //Update frequency
unsigned long time_now = 0;
bool dotsOn = true;
byte r_val = 0;
byte g_val = 0;
byte b_val = 255;
CRGB alternateColor = CRGB::Black; 
// TODO TEST add all variables for modes
unsigned long countdownMilliSeconds;
unsigned long endCountDownMillis;
byte clockMode = 0; 
byte digitH1 = 0;
byte digitH2 = 0;
byte digitM1 = 0;
byte digitM2 = 0;
byte hourFormat = 24;                         // Change this to 12 if you want default 12 hours format instead of 24               
CRGB countdownColor = CRGB::Green;
byte scoreboardLeft = 0;
byte scoreboardRight = 0;
CRGB scoreboardColorLeft = CRGB::Green;
CRGB scoreboardColorRight = CRGB::Red;
// palette
DEFINE_GRADIENT_PALETTE( greenblue_gp ) { 
  0,   2,  0, 36,
  46,  9,  9,  121,
  255, 0,  212, 255
};
CRGBPalette16 greenblue = greenblue_gp;
uint8_t colorIndex[NUM_LEDS];
// color change continously
bool isFluidColor = true;
/** color mode
 * 0: mono
 * 1: blue gradient pallet
 * 2: pacifica
 * */
int colorMode = 2;
byte brightness = 50;
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
int WAKE_HOU = 5;
int WAKE_MIN = 0;
int WAKE_SEC = 0;
int SLEEP_HOU = 22;
int SLEEP_MIN= 0;
int SLEEP_SEC= 0;
byte WAKE_BRIGHTNESS = 50;
byte SLEEP_BRIGHTNESS = 1;



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(LEDs, NUM_LEDS);
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(LEDs, NUM_LEDS, CRGB::Black);
  FastLED.show();
  //Fill the colorIndex array with random numbers
  for (int i = 0; i < NUM_LEDS; i++) {
    colorIndex[i] = random8();
  }
  // dht.begin();
  // pinMode(BUTTON_PIN, INPUT);

//  WiFi.begin(ssid, password);
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
      // ESP.restart();
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("connected...yeey :)");
  }
//  while ( WiFi.status() != WL_CONNECTED ) {
//    delay(500);
//    Serial.print(".");
//  }
//  Serial.println("connected");
  Serial.println(WiFi.localIP());
  if (mdns.begin("esp8266", WiFi.localIP()))
  Serial.println("MDNS responder started");

  server.on("/color", colorHandler);
  server.on("/hourformat", hourformatHandler);
  server.on("/brightness", brightnessHandler);
  server.on("/countdown", countdownHandler);
  server.on("/scoreboard", scoreboardHandler);
  server.on("/clock", clockHandler);
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
  Serial.println(); 

  // TODO add html zip
  
  
  timeClient.begin();
  delay(10);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.status() == WL_CONNECTED) { // check WiFi connection status
    // int sensor_val = analogRead(LDR_PIN);
    // Brightness =40;
    server.handleClient();
    while (millis() > time_now + period) {
      time_now = millis();
      // TODO TEST update all mode
      if (clockMode == 0) {
        updateClock();
      } else if (clockMode == 1) {
        updateCountdown();  
      } else if (clockMode == 3) {
        updateScoreboard();            
      }
  
      FastLED.setBrightness(brightness);
      FastLED.show();
    }
    // TODO add logic fastled show for color fluid 
    EVERY_N_MILLISECONDS(5){
      for (int i = 0; i < NUM_LEDS; i++) {
        colorIndex[i]++;
      }
      if (colorMode != 0) {
        switch (colorMode)
        {
        case 1:
          simple_blue_pallete();
          break;
        case 2:
          pacifica_loop();
          break;
        
        default:
          break;
        }
        // if(fluidType == 0) {
        //   pacifica_loop();
        // }
        displayNumber(digitH1, 3, alternateColor);
        displayNumber(digitH2, 2, alternateColor);
        displayNumber(digitM1, 1, alternateColor);
        displayNumber(digitM2, 0, alternateColor); 
        // displayDots(alternateColor);
        // TODO change digit color fluid and switch isDigitChange, fasled show 
        FastLED.show();
      }
    }
  }
}

void updateClock() {  
  // TODO TEST update 12h format
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
  
  CRGB color = CRGB(r_val, g_val, b_val);

  digitH1 = (h1 > 0) ? h1 : 10;
  digitH2 = h2;
  digitM1 = m1;
  digitM2 = m2;
  displayNumber(digitH1,3,color);
  displayNumber(digitH2,2,color);
  displayNumber(digitM1,1,color);
  displayNumber(digitM2,0,color); 

  displayDots(color);  
  updateBrightnessByMoment(hou, minu, sec);
}
void updateBrightnessByMoment(int hou, int minu, int sec) {
//int SLEEP_HOU = 22;
//int SLEEP_MIN= 0;
//int WAKE_HOU = 5;
//int WAKE_MIN = 0;
  if (hou == WAKE_HOU && minu == WAKE_MIN && sec == WAKE_SEC) {
    brightness = WAKE_BRIGHTNESS;
  }
  if (hou == SLEEP_HOU && minu == SLEEP_MIN && sec == WAKE_SEC) {
    brightness = SLEEP_BRIGHTNESS;
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

  CRGB color = countdownColor;
  if (restMillis <= 60000) {
    color = CRGB::Red;
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

void endCountdown() {
  allBlank();
  for (int i=0; i<NUM_LEDS; i++) {
    if (i>0)
      LEDs[i-1] = CRGB::Black;
    
    LEDs[i] = CRGB::Red;
    FastLED.show();
    delay(25);
  }  
}

void displayDots(CRGB color) {
  if (isFluidColor) {
    CRGB color1 = ColorFromPalette(greenblue, colorIndex[15]);
    CRGB color2 = ColorFromPalette(greenblue, colorIndex[16]);
    LEDs[15] = color1;
    LEDs[16] = color2;
  } else {
    if (dotsOn) {
      LEDs[15] = color;
      LEDs[16] = color;
    } else {
      LEDs[15] = CRGB::Black;
      LEDs[16] = CRGB::Black;
    }

    dotsOn = !dotsOn;  
  }
}

void hideDots() {
  LEDs[15] = CRGB::Black;
  LEDs[16] = CRGB::Black;
}

void allBlank() {
  for (int i=0; i<NUM_LEDS; i++) {
    LEDs[i] = CRGB::Black;
  }
  FastLED.show();
}

// TODO add logic add color fluid in displaynumber
void displayNumber(byte number, byte segment, CRGB color) {
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
  // skip 1st led
  startindex++;
  for (byte i=0; i<7; i++){             //// value start index digit led no 2.
    yield();
    // if (isFluidColor) {
    //   color = ColorFromPalette(greenblue, colorIndex[i + startindex]);
    // }
    if (colorMode != 0) {
      if ((numbers[number] & 1 << i) == 0) {
        LEDs[i + startindex] = CRGB::Black;
      };
    } else {
      LEDs[i + startindex] = (numbers[number] & 1 << i) ? color : alternateColor;
    }
  } 
}

// Handlers
void colorHandler() {    
  r_val = server.arg("r").toInt();
  g_val = server.arg("g").toInt();
  b_val = server.arg("b").toInt();
  colorMode = server.arg("colorMode").toInt();
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
  countdownColor = CRGB(cd_r_val, cd_g_val, cd_b_val); 
  endCountDownMillis = millis() + countdownMilliSeconds;
  allBlank(); 
  clockMode = 1;     
  server.send(200, "text/json", "{\"result\":\"ok\"}");
}

void scoreboardHandler() {   
  scoreboardLeft = server.arg("left").toInt();
  scoreboardRight = server.arg("right").toInt();
  scoreboardColorLeft = CRGB(server.arg("rl").toInt(),server.arg("gl").toInt(),server.arg("bl").toInt());
  scoreboardColorRight = CRGB(server.arg("rr").toInt(),server.arg("gr").toInt(),server.arg("br").toInt());
  clockMode = 3;     
  server.send(200, "text/json", "{\"result\":\"ok\"}");
}

void clockHandler() {       
  clockMode = 0;     
  server.send(200, "text/json", "{\"result\":\"ok\"}");
}

// TODO TEST add time format handler
void hourformatHandler() {   
  hourFormat = server.arg("hourformat").toInt();
  clockMode = 0;     
  server.send(200, "text/json", "{\"result\":\"ok\"}");
}

// color helper

// simple blue pallete
void simple_blue_pallete () {
  for(byte i=0; i<NUM_LEDS; i++) {
    LEDs[i] = ColorFromPalette(greenblue, colorIndex[i]);
  }
}

// pacifica
CRGBPalette16 pacifica_palette_1 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50 };
CRGBPalette16 pacifica_palette_2 = 
    { 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117, 
      0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F };
CRGBPalette16 pacifica_palette_3 = 
    { 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33, 
      0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF };
 
 
void pacifica_loop()
{
  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  sLastms = ms;
  uint16_t speedfactor1 = beatsin16(3, 179, 269);
  uint16_t speedfactor2 = beatsin16(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88(1011,10,13));
  sCIStart2 -= (deltams21 * beatsin88(777,8,11));
  sCIStart3 -= (deltams1 * beatsin88(501,5,7));
  sCIStart4 -= (deltams2 * beatsin88(257,4,6));
 
  // Clear out the LED array to a dim background blue-green
  fill_solid( LEDs, NUM_LEDS, CRGB( 2, 6, 10));
 
  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer( pacifica_palette_1, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0-beat16( 301) );
  pacifica_one_layer( pacifica_palette_2, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  pacifica_one_layer( pacifica_palette_3, sCIStart3, 6 * 256, beatsin8( 9, 10,38), 0-beat16(503));
  pacifica_one_layer( pacifica_palette_3, sCIStart4, 5 * 256, beatsin8( 8, 10,28), beat16(601));
 
  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps();
 
  // Deepen the blues and greens a bit
  pacifica_deepen_colors();
}
 
// Add one layer of waves into the led array
void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    waveangle += 250;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16( ci) + 32768;
    uint8_t sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    LEDs[i] += c;
  }
}
 
// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps()
{
  uint8_t basethreshold = beatsin8( 9, 55, 65);
  uint8_t wave = beat8( 7 );
  
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = LEDs[i].getAverageLight();
    if( l > threshold) {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8( overage, overage);
      LEDs[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}
 
// Deepen the blues and greens
void pacifica_deepen_colors()
{
  for( uint16_t i = 0; i < NUM_LEDS; i++) {
    LEDs[i].blue = scale8( LEDs[i].blue,  145); 
    LEDs[i].green= scale8( LEDs[i].green, 200); 
    LEDs[i] |= CRGB( 2, 5, 7);
  }
}
