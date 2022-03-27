#include "iivx_leo.h"
iivxReport_t report;

#define REPORT_DELAY 2000
// Number of microseconds between HID reports
// 2000 = 500hz

uint8_t buttonCount = 4;
uint8_t buttonPins[] = {5, 2, 3, 4};


unsigned int sliderVals[] = {0, 0, 0, 0, 0};

int serialCount = 0;

int inval = 0;
bool readyToRead = false;

int timer = 0;
int counter = 0;
bool sendLED = true;

volatile uint16_t oldLightState = 2;
volatile uint16_t oldButtonState = 2;
unsigned long lastHidSend = 0;


String segString = "@beatmania#";

void setup() {
  delay(1000);
  // Setup I/O for pins
  for (int i = 0; i < buttonCount; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  Serial1.begin(115200);
  Serial.begin(9600);
}

void loop() {
  // Read buttons
  GetSliders();
  update16Seg();

  for (int i = 0; i < buttonCount; i++) {
    if (digitalRead(buttonPins[i]) != HIGH) {
      report.buttons |= (uint16_t)1 << i;
    } else {
      report.buttons &= ~((uint16_t)1 << i);
    }
  }
  // Light LEDs
  if ((millis() - lastHidUpdate) >= 1000) {
    //do react code here
    if (oldButtonState != report.buttons) {
      lights(report.buttons);
      oldButtonState = report.buttons;
    }
  } else {
    //do HID code
    if (oldLightState != hidLedState) {
      uint16_t sendstate = hidLedState;
      lights(sendstate);
      oldLightState = hidLedState;
    }
  }

  // Send report and delay
  iivx.setState(&report);
  delayMicroseconds(REPORT_DELAY);
}

void lights(uint16_t lightDesc) {
  Serial1.write('%');
  Serial1.write(lightDesc);
  Serial1.flush();
}

void GetSliders() {
  while (Serial1.available() > 0) {
    inval = Serial1.read();
    //Serial.println(inval);
    if (!readyToRead) {
      if (char(inval) == 'A') {
        readyToRead = true;
      }
    } else {
      //get all sliders vals
      sliderVals[serialCount] = inval;
      serialCount++;
      if (serialCount > 4) {
        report.xAxis  = sliderVals[0];
        report.yAxis  = sliderVals[1];
        report.zAxis  = sliderVals[2];
        report.rxAxis = sliderVals[3];
        report.ryAxis = sliderVals[4];
        serialCount = 0;
        Serial1.write('~');
        //break;

      }
    }
  }
}

void send16Seg() {
  //  //do 16 seg shit
  char charOut[segString.length() + 1];
  for (int i = 0; i < sizeof(charOut); i++) {
    charOut[i] = segString.charAt(i);
  }
  Serial1.write('@');
  Serial1.write(charOut);
  Serial1.write('#');
  Serial1.flush();
}


void update16Seg() {
  //should handle this better
  //hopefully only unique string is ever passed here but should check anyway.
  String newString;
  if (Serial.available() > 0) {
    char inChar;
    while (Serial.available() > 0) {
      inChar = Serial.read();
      if (inChar != '\n')
        newString += inChar;
    }
    if (newString != segString) {
      segString = newString;
      send16Seg();
    }
  }
}
