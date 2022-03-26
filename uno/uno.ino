
/**
 * seg reference for tlc pin outputs
 * 
*        --0--   --15-- 
*       | \    |    / |
*       3  2   1  14 13
*       |   \  |  /   |
*        --4--   --12-
*       |   /  |  \   |
*       7  6   5  11  10
*       | /    |    \ |
 *       --8--   --9-- 
 * 
 * 
 * 
 */

#include "Tlc5940.h"
#include <Adafruit_NeoPixel.h>
//Sliders
#define VEFX A1
#define LOWEQ A2
#define HIEQ A3
#define FILTER A4
#define PLAYVOL A5
#define REFRESH 300
#define DEADZONE 1
#define NO_OF_LEDS 4
int timer =0;
int count =0;

//74hc595 pin setup
//
// SH_CP; 11
const int clockPin = 4;
// DS; 14
const int dataPin = 2;
// ST_CP; 12
const int latchPin = 7;
//OE 13
const int OE = 6;

bool readingSeg = false;


int vefxState = 0;
int lowEqState = 0;
int hiEqState = 0;
int filterState = 0;
int playVolState = 0;
int oldvefxState = 0;
int oldlowEqState = 0;
int oldhiEqState = 0;
int oldfilterState = 0;
int oldplayVolState = 0;

bool estConnect = false;
bool sendVals = false;

char inChar = ' ' ;
String inString = " BEATMANIA";
int segCount = 0;
int sliderCount = 0;

uint16_t ledState = 0;
bool ledUpdate = false;
unsigned long lastLedUpdate=0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(4, A0, NEO_GRB + NEO_KHZ800);


void setup() {
  Tlc.init(0);
  
  pinMode(OE, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(0, OUTPUT);

  pinMode(5, OUTPUT);
  displayOff();

  strip.setBrightness(100);
  strip.begin();

  
  delay(1000); //let USB MCU setup
  // put your setup code here, to run once:
  Serial.begin(115200);
  EstablishConnection();
}

void loop() {
  // put your main code here, to run repeatedly:
  handleSerial();
  if(sendVals){
    SendSliderVals();
  }
  timer++;
  if(timer >=200){
    count ++;
    timer =0;
  }
  updateSeg();

  delayMicroseconds(REFRESH);
}

void lights(uint16_t lightState){
    for(int i =0; i < NO_OF_LEDS; i++){
       if((lightState>>i)&1){
          strip.setPixelColor( i, strip.Color(255, 0, 0));
       }else{
          strip.setPixelColor( i, strip.Color(0, 0, 0));
       }
    }
    strip.show();
}

void GetSliderVals(){
  switch(sliderCount){
    case 0:
      vefxState = analogRead(VEFX)/4;
      sliderCount++;
      break;
    case 1:
      lowEqState = analogRead(LOWEQ)/4;
      sliderCount++;
      break;
    case 2:
      hiEqState = analogRead(HIEQ)/4;
      sliderCount++;
      break;
    case 3:
      filterState = analogRead(FILTER)/4;
      sliderCount++;
      break;
    case 4:
      playVolState = analogRead(PLAYVOL)/4;
      sliderCount = 0;
      break;
  }
}

void SendSliderVals(){
    GetSliderVals();
    //only send slider update if there is a change and USB MCU asking for update
    if(abs(vefxState-oldvefxState)>DEADZONE){
     SendAnalogs();
    }else if(abs(lowEqState-oldlowEqState)>DEADZONE){
      SendAnalogs();
    }else if(abs(hiEqState-oldhiEqState)>DEADZONE){
     SendAnalogs();
    }else if(abs(filterState-oldfilterState)>DEADZONE){
      SendAnalogs();
    }else if(abs(playVolState-oldplayVolState)>DEADZONE){
      SendAnalogs();
    }
}

void SendAnalogs(){
    Serial.write(vefxState);
    Serial.write(lowEqState);
    Serial.write(hiEqState);
    Serial.write(filterState);
    Serial.write(playVolState);
    oldvefxState = vefxState;
    oldlowEqState = lowEqState;
    oldhiEqState = hiEqState;
    oldfilterState = filterState;
    oldplayVolState = playVolState;
    sendVals = false;

}
String newString = String();
void handleSerial(){
  if(Serial.available() > 0){
    inChar = Serial.read();
    if(readingSeg == true){
      if(inChar == '~'){
        sendVals = true;
      }else{
        if(inChar == '#' || Serial.available() <= 0){
           inString = newString;
           readingSeg == false;
        }else{
          newString += inChar;
        }
      }
    }
    if (inChar == '%'){
      ledUpdate = true;
      ledState = Serial.read();
      lights(ledState);
    }
    if(inChar == '~'){
      sendVals = true;
    }
    if (inChar == '@'){
      readingSeg = true;
      newString = String();
    }
  }
}

void EstablishConnection(){
  while(Serial.available() <=0){
    Serial.print('A');
    delay(300);
  }
  digitalWrite(OE, LOW);
}

void updateSeg(){
  //set the char before turning the segment on
  setTlcChar(inString.charAt(segCount+1));

  // The bit shifter only supports 8 lights, we need 9 for the display,
  // so pin 5 on the arduino is used. Seems to work fine so far.
  if(!(segCount>=8)){
      digitalWrite(latchPin, LOW);
      shiftOut(dataPin, clockPin, MSBFIRST, 1<<segCount);
      digitalWrite(latchPin, HIGH);
      segCount++;
  }
  else{
    segCount=0;
    displayOff();
    digitalWrite(5, 1);
  }
}

void displayOff(){
  digitalWrite(5, 0);
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, B00000000);
  digitalWrite(latchPin, HIGH);
}

//this was used for testing 
void setTLC(int SegNum){
  displayOff();
  Tlc.clear();
  setTlcChar('c');
  Tlc.update();  
  delay(1);
  if(count >= 16){
    count =0;
  }
}


/* This is the tlc "font"
 * can be improved, currently a mandatory delay(1) to allow the tlc to update and stop ghosting
 * Maybe a custom TLC library could imrpove this, as a smaller delay here means a longer delay in the loop 
 * which would increase led brighness (more time on per loop)
 */
void setTlcChar(char letter){
  displayOff();
  switch(letter){
    case 'A':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'B':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 0);
      Tlc.set(11, 4095);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 4095);
      break;
    case 'C':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'D':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case 'E':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'F':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'G':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'H':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case 'I':
      Tlc.set(0, 4095);
      Tlc.set(1, 4095);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 4095);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'J':
      Tlc.set(0, 4095);
      Tlc.set(1, 4095);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 4095);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 4095);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'K':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 4095);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 0);
      break;
    case 'L':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case 'M':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 4095);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 4095);
      Tlc.set(14, 4095);
      Tlc.set(15, 0);
      break;
    case 'N':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 4095);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 4095);
      Tlc.set(11, 4095);
      Tlc.set(12, 0);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case 'O':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'P':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'Q':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 4095);
      Tlc.set(12, 0);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'R':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 4095);
      Tlc.set(12, 4095);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'S':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'T':
      Tlc.set(0, 4095);
      Tlc.set(1, 4095);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 4095);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'U':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case 'V':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 4095);
      Tlc.set(7, 4095);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 0);
      break;
    case 'W':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 4095);
      Tlc.set(7, 4095);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 4095);
      Tlc.set(11, 4095);
      Tlc.set(12, 0);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case 'X':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 4095);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 4095);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 4095);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 0);
      break;
    case 'Y':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 4095);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 4095);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 0);
      break;
    case 'Z':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 4095);
      Tlc.set(7, 0);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 4095);
      break;
    case '1':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case '2':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case '3':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case '4':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case '5':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case '6':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case '7':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case '8':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case '9':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case '0':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 4095);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 4095);
      Tlc.set(14, 4095);
      Tlc.set(15, 4095);
      break;
    case '!':
      Tlc.set(0, 0);
      Tlc.set(1, 4095);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 4095);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 4095);
      break;
    case '?':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 4095);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case '-':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    //m is used for periods for some reason
    case 'm':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 4095);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case '&':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 4095);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 4095);
      Tlc.set(7, 0);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 4095);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 4095);
      break;
    case '(':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 4095);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 0);
      break;
    case ')':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 4095);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 4095);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    //q is used for ' 
    case 'q':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 0);
      break;
    case '\"':
      Tlc.set(0, 0);
      Tlc.set(1, 4095);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case '/':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 4095);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 0);
      break;
    case '\\':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 4095);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 4095);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case '*':
      Tlc.set(0, 0);
      Tlc.set(1, 4095);
      Tlc.set(2, 4095);
      Tlc.set(3, 0);
      Tlc.set(4, 4095);
      Tlc.set(5, 4095);
      Tlc.set(6, 4095);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 4095);
      Tlc.set(12, 4095);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 0);
      break;
    case '_':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    //u is used for ,
    case 'u':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 4095);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case '%':
      Tlc.set(0, 4095);
      Tlc.set(1, 4095);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 4095);
      Tlc.set(6, 4095);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 4095);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 0);
      break;
    case '[':
      Tlc.set(0, 4095);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 4095);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 0);
      Tlc.set(14, 0);
      Tlc.set(15, 0);
      break;
    case ']':
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 0);
      Tlc.set(3, 0);
      Tlc.set(4, 0);
      Tlc.set(5, 0);
      Tlc.set(6, 0);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 0);
      Tlc.set(12, 0);
      Tlc.set(13, 4095);
      Tlc.set(14, 0);
      Tlc.set(15, 4095);
      break;
    case 'g':
      Tlc.set(0, 4095);
      Tlc.set(1, 4095);
      Tlc.set(2, 4095);
      Tlc.set(3, 4095);
      Tlc.set(4, 4095);
      Tlc.set(5, 4095);
      Tlc.set(6, 4095);
      Tlc.set(7, 4095);
      Tlc.set(8, 4095);
      Tlc.set(9, 4095);
      Tlc.set(10, 4095);
      Tlc.set(11, 4095);
      Tlc.set(12, 4095);
      Tlc.set(13, 4095);
      Tlc.set(14, 4095);
      Tlc.set(15, 4095);
      break;
    case ' ':
      Tlc.clear();
      break;

    //helps find letters/symbols i havent written yet
    default:
      Tlc.set(0, 0);
      Tlc.set(1, 0);
      Tlc.set(2, 4095);
      Tlc.set(3, 0);
      Tlc.set(4, 4095);
      Tlc.set(5, 0);
      Tlc.set(6, 4095);
      Tlc.set(7, 0);
      Tlc.set(8, 0);
      Tlc.set(9, 0);
      Tlc.set(10, 0);
      Tlc.set(11, 4095);
      Tlc.set(12, 4095);
      Tlc.set(13, 0);
      Tlc.set(14, 4095);
      Tlc.set(15, 0);
      break;
  }
  Tlc.update(); 
  delay(1); 
}
