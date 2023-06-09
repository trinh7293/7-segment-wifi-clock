#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <NTPClient.h>
#include <FastLED.h>
#include <TimeLib.h>
#define NUM_LEDS 86                         // Total of 30 LED's     1X7X4+2=30. 1 pixel/segment
#define DATA_PIN D4                          // Change this if you are using another type of ESP board than a WeMos D1 Mini
#define MILLI_AMPS 800 


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time.nist.gov", 7*3600, 60000); //GMT+5:30 : 5*3600+30*60=19800

CRGB LEDs[NUM_LEDS];
int period = 2000;   //Update frequency
unsigned long time_now = 0;
int Second, Minute, Hour;
bool dotsOn = true;
byte r_val = 255;
byte g_val = 0;
byte b_val = 0;
CRGB alternateColor = CRGB::Black; 

long numbers[] = {
  0b000111111111111111111,  // [0] 0
  0b000111000000000000111,  // [1] 1
  0b111111111000111111000,  // [2] 2
  0b111111111000000111111,  // [3] 3
  0b111111000111000000111,  // [4] 4
  0b111000111111000111111,  // [5] 5
  0b111000111111111111111,  // [6] 6
  0b000111111000000000111,  // [7] 7
  0b111111111111111111111,  // [8] 8
  0b111111111111000111111,  // [9] 9
  0b000000000000000000000,  // [10] off
  0b111111111111000000000,  // [11] degrees symbol
  0b000000111111111111000,  // [12] C(elsius)
  0b111000111111111000000,  // [13] F(ahrenheit)
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(LEDs, NUM_LEDS);
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  fill_solid(LEDs, NUM_LEDS, CRGB::Black);
  FastLED.show();
  // dht.begin();
  // pinMode(BUTTON_PIN, INPUT);

//  WiFi.begin(ssid, password);
  Serial.print("Connecting.");
  WiFiManager wm;
  bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("DIGITAL_CLOCK"); // password protected ap

  if(!res) {
      Serial.println("Failed to connect");
      // ESP.restart();
  } 
  else {
      //if you get here you have connected to the WiFi    
      Serial.println("connected...yeey :)");
  }
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");
  timeClient.begin();
  delay(10);
}

void updateClock() {  
  // if (hourFormat == 12 && hour > 12)
  //   hour = hour - 12;
  
  byte h1 = Hour / 10;
  byte h2 = Hour % 10;
  byte m1 = Minute / 10;
  byte m2 = Minute % 10;  
  // byte s1 = Second / 10;
  // byte s2 = Second % 10;
  
  CRGB color = CRGB(r_val, g_val, b_val);

  if (h1 > 0)
    displayNumber(h1,3,color);
  else 
    displayNumber(10,3,color);  // Blank
    
  displayNumber(h2,2,color);
  displayNumber(m1,1,color);
  displayNumber(m2,0,color); 

  displayDots(color);  
}

void displayDots(CRGB color) {
  if (dotsOn) {
    LEDs[42] = color;
    LEDs[43] = color;
  } else {
    LEDs[42] = CRGB::Black;
    LEDs[43] = CRGB::Black;
  }

  dotsOn = !dotsOn;  
}

void displayNumber(byte number, byte segment, CRGB color) {
  /*
   * 
      __ __ __        __ __ __          __ __ __        12 13 14  
    __        __    __        __      __        __    11        15
    __        __    __        __      __        __    10        16
    __        __    __        __  42  __        __    _9        17
      __ __ __        __ __ __          __ __ __        20 19 18  
    __        65    __        44  43  __        21    _8        _0
    __        __    __        __      __        __    _7        _1
    __        __    __        __      __        __    _6        _2
      __ __ __       __ __ __           __ __ __       _5 _4 _3   

   */
 
  // segment from left to right: 3, 2, 1, 0
  byte startindex = 0;
  switch (segment) {
    case 0:
      startindex = 0;
      break;
    case 1:
      startindex = 21;
      break;
    case 2:
      startindex = 44;
      break;
    case 3:
      startindex = 65;
      break;    
  }

  for (byte i=0; i<21; i++){             //// value start index digit led no 2.
    yield();
    LEDs[i + startindex] = ((numbers[number] & 1 << i) == 1 << i) ? color : alternateColor;
  } 
}

void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.status() == WL_CONNECTED) { // check WiFi connection status
    // int sensor_val = analogRead(LDR_PIN);
    // Brightness =40;
    timeClient.update();
    // int Hours;
    unsigned long unix_epoch = timeClient.getEpochTime();   // get UNIX Epoch time
    Second = second(unix_epoch);                            // get seconds
    Minute = minute(unix_epoch);                            // get minutes
    Hour  = hour(unix_epoch);                              // get hours
    while (millis() > time_now + period) {
      time_now = millis();
      updateClock();     // Show Time
      FastLED.show();
    }
  }
}
