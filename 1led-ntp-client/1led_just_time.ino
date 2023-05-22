#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <FastLED.h>
#include <TimeLib.h>
#define NUM_LEDS 30                           // Total of 30 LED's     1X7X4+2=30. 1 pixel/segment
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

// set Wi-Fi SSID and password
/* TOTO */const char *ssid     = "Phong 22";
/* TOTO */const char *password = "@Rock1234567";

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

  WiFi.begin(ssid, password);
  Serial.print("Connecting.");
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
    LEDs[14] = color;
    LEDs[15] = color;
  } else {
    LEDs[14] = CRGB::Black;
    LEDs[15] = CRGB::Black;
  }

  dotsOn = !dotsOn;  
}

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

  for (byte i=0; i<7; i++){             //// value start index digit led no 2.
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
    // if (TIME_FORMAT == 12) {
    //   if (Hours > 12) {
    //     Hour = Hours - 12;
    //   }
    //   else
    //     Hour = Hours;
    // }
    // else
    //   Hour = Hours;
  }
}
