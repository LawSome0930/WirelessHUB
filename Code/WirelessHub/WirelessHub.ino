#include "ESP32Encoder.h"  // https://github.com/madhephaestus/ESP32Encoder/
#include   "Keypad.h"           // https://github.com/Chris--A/Keypad
#include "BleGamepad.h"     // https://github.com/lemmingDev/ESP32-BLE-Gamepad

#define numOfButtons        28
#define numOfHatSwitches    0
#define enableX             false
#define enableY             false
#define enableZ             false
#define enableRZ            false
#define enableRX            false
#define enableRY            false
#define enableSlider1       false
#define enableSlider2       false
#define enableRudder        false
#define enableThrottle      false
#define enableAccelerator   false
#define enableBrake         false
#define enableSteering      false

BleGamepad bleGamepad("WirelessHub", "HUB", 100);


////////////////////// BUTTON MATRIX //////////////////////
#define ROWS 5
#define COLS 4
uint8_t rowPins[ROWS] = {13, 14, 15, 16, 33};
uint8_t colPins[COLS] = {17, 18, 19, 21};
uint8_t keymap[ROWS][COLS] = {
  {0,1,2,3},
  {4,5,6,7},
  {8,9,10,11},
  {12,13,14,15},
  {24,25,26,27}
};
Keypad customKeypad = Keypad( makeKeymap(keymap), rowPins, colPins, ROWS, COLS); 


//////////// ROTARY ENCODERS (WITH PUSH SWITCHES) ////////////
#define MAXENC 4
uint8_t uppPin[MAXENC] = {22, 25, 27, 04};
uint8_t dwnPin[MAXENC] = {23, 26, 32, 05};
uint8_t encoderUpp[MAXENC] = {16,18,20,22};
uint8_t encoderDwn[MAXENC] = {17,19,21,23};
ESP32Encoder encoder[MAXENC];
unsigned long holdoff[MAXENC] = {0,0,0,0};
int32_t prevenccntr[MAXENC] = {0,0,0,0};
bool prevprs[MAXENC] = {0,0,0,0};
#define HOLDOFFTIME 150   

//////////// BatteryMeasure ////////////
#define batteryMeasurepin 36  //battery voltage measure GPIO
uint8_t batteryLevel = 100; //record battery power level
uint64_t batteryTiming = 14400000000; //Battery power timing report
uint64_t  batteryNow = 14400000000;

////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  for (uint8_t i=0; i<MAXENC; i++) {
    encoder[i].clearCount();
    encoder[i].attachSingleEdge(dwnPin[i], uppPin[i]);
  }
  customKeypad.addEventListener(keypadEvent);
  customKeypad.setHoldTime(1);
  bleGamepad.begin();
}

////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  
  unsigned long now = millis();

  // -- ROTARY ENCODERS : ROTATION -- //

  for (uint8_t i=0; i<MAXENC; i++) {
    int32_t cntr = encoder[i].getCount();
    if (cntr!=prevenccntr[i]) {
      if (!holdoff[i]) {
        if (cntr>prevenccntr[i]) { sendKey(encoderUpp[i]); }
        if (cntr<prevenccntr[i]) { sendKey(encoderDwn[i]); }
        holdoff[i] = now;
        if (holdoff[i]==0) holdoff[i] = 1;  // SAFEGUARD WRAP AROUND OF millis() (WHICH IS TO 0) SINCE holdoff[i]==0 HAS A SPECIAL MEANING ABOVE
      }
      else if (now-holdoff[i] > HOLDOFFTIME) {
        prevenccntr[i] = encoder[i].getCount();
        holdoff[i] = 0;
      }
    }
  }
  
  // -- ROTARY ENCODERS : PUSH SWITCH -- //
  
  customKeypad.getKey();    // READ BUTTON MATRIX (EVENT CALLBACK SETUP)

  batteryUpdate();    //battery power report
}




///////////////////////////getKey() EVENT CALLBACK FUNCTIOn////////////////////////////

void keypadEvent(KeypadEvent key){
  uint8_t keystate = customKeypad.getState();
  if (keystate==PRESSED)  { pressKey(key); }
  if (keystate==RELEASED) { releaseKey(key); }
}


////////////////////////////////////////////////////////////////////////////////////////

void sendKey(uint8_t key) {
    uint32_t gamepadbutton = pow(2,key);      // CONVERT TO THE BINARY MAPPING GAMEPAD KEYS USE
    if(bleGamepad.isConnected()) {
      bleGamepad.press(gamepadbutton);
      delay(150);
      bleGamepad.release(gamepadbutton);
    }
}

void pressKey(uint8_t key) {
    uint32_t gamepadbutton = pow(2,key);      // CONVERT TO THE BINARY MAPPING GAMEPAD KEYS USE
    if(bleGamepad.isConnected()) {
      bleGamepad.press(gamepadbutton);
    }
}

void releaseKey(uint8_t key) {
    uint32_t gamepadbutton = pow(2,key);      // CONVERT TO THE BINARY MAPPING GAMEPAD KEYS USE
    if(bleGamepad.isConnected()) {
      bleGamepad.release(gamepadbutton);
    }
}


/*battery power mearsure function*/
void batteryUpdate()
{
  batteryNow++;
  if (batteryNow == batteryTiming)
  {
    batteryNow = 0;
    int analogValue = analogRead(batteryMeasurepin);
    double batteryVoltage = 2*(((double)analogValue)/4095)*3.3;
    batteryLevel = ((batteryVoltage -3.6)/(4.2 - 3.6))*100;
    bleGamepad.setBatteryLevel(batteryLevel);
  }
}
