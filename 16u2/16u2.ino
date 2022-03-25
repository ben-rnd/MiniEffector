#include "HID-Project.h"
#include "HID-Settings.h"


char income;

bool debug = true;

int incomingByte = 0;


//Defining Button Pins (also used when setting button states);
#define P1 1
#define EFFECT 2
#define VEFX 3
#define P2 4
//button states
int P1State = 0;
int effState = 0;
int vefxState = 0;
int P2State = 0;

unsigned int slider1State = 0;
unsigned int slider2State = 0;
unsigned int slider3State = 0;
unsigned int slider4State = 0;
unsigned int slider5State = 0;

int serialCount = 0;
int inval= 0;

bool readyToRead = false;

int timer =0;
int counter =0;

unsigned int sliderVals[] = {0,0,0,0,0};

String segString = "@beatmania#";


void setup() {
  //Setup Panel Inputs
  pinMode(P1, INPUT_PULLUP);
  pinMode(EFFECT, INPUT_PULLUP);
  pinMode(VEFX, INPUT_PULLUP);
  pinMode(P2, INPUT_PULLUP);

  //Begin Serial to MCU
  Serial1.begin(115200);

  //Declare Gamepad to PC
  Gamepad.begin();

  if(debug){
     Serial.begin(9600);
  }
}

void loop() {
  
  GetSliders();
  SetSliders();
  GetAndSetStates();
  update16Seg();
  Gamepad.write();

  //delay to not overload the UNO
  delay(10);

}


void GetSliders(){
  while(Serial1.available() > 0){
      inval = Serial1.read();
      Serial.write(inval);
      if(!readyToRead){
          if(char(inval)== 'A'){
            Serial1.write('~');
            readyToRead = true;
          }
      }else{
        //get all sliders vals
        sliderVals[serialCount] = inval;
        serialCount++;
        if(serialCount >4){
            slider1State = sliderVals[0];
            slider2State = sliderVals[1];
            slider3State = sliderVals[2];
            slider4State = sliderVals[3];
            slider5State = sliderVals[4];
            serialCount = 0;
            //break;
         //   Serial1.write('~');
          }
      }
   }
}

void SetSliders(){
    Gamepad.xAxis(slider1State);
    Gamepad.yAxis(slider2State);
    Gamepad.zAxis(slider3State);
    Gamepad.rxAxis(slider4State);
    Gamepad.ryAxis(slider5State);
}

void GetAndSetStates(){
  //read States
  P1State = digitalRead(P1);
  effState = digitalRead(EFFECT);
  vefxState = digitalRead(VEFX);
  P2State = digitalRead(P2);

  //set gamePad states
  if(P1State == LOW){
    Gamepad.press(P1);
  }else{
     Gamepad.release(P1);
  }
  if(effState == LOW){
   Gamepad.press(EFFECT);
  }else{
     Gamepad.release(EFFECT);
  }
  if(vefxState == LOW){
   Gamepad.press(VEFX);
  }else{
    Gamepad.release(VEFX);
  }
  if(P2State == LOW){
    Gamepad.press(P2);
  }else{
     Gamepad.release(P2);
  }
}
void send16Seg(){
//  //do 16 seg shit
  char charOut[segString.length()+1];
  for(int i=0; i < sizeof(charOut); i++){
    charOut[i] = segString.charAt(i);
  }
  Serial1.write('@');
  Serial1.write(charOut);
  Serial1.write('#');
  Serial1.flush();
}


void update16Seg(){
  //should handle this better
  //hopefully only unique string is ever passed here but should check anyway.
  String newString;
  if(Serial.available() > 0){
      char inChar;
      while(Serial.available() > 0){
        inChar = Serial.read();
        if(inChar !='\n')
        newString +=inChar;
      }
      if(newString != segString){
        segString = newString;
        send16Seg();
      }
  }
}
