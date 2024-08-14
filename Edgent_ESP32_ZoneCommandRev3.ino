/*************************************************************
  Blynk is a platform with iOS and Android apps to control
  ESP32, Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build mobile and web interfaces for any
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: https://www.blynk.io
    Sketch generator:           https://examples.blynk.cc
    Blynk community:            https://community.blynk.cc
    Follow us:                  https://www.fb.com/blynkapp
                                https://twitter.com/blynk_app

  Blynk library is licensed under MIT license
 *************************************************************
  Blynk.Edgent implements:
  - Blynk.Inject - Dynamic WiFi credentials provisioning
  - Blynk.Air    - Over The Air firmware updates
  - Device state indication using a physical LED
  - Credentials reset using a physical Button
 *************************************************************/


// Fill-in information from your Blynk Template here
#define BLYNK_TEMPLATE_ID   "TMPL3bTnPK_y"
#define BLYNK_TEMPLATE_NAME "ZoneCommand"
 
#define BLYNK_FIRMWARE_VERSION        "0.7.7"
 
#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG
 
#define APP_DEBUG
 
// Uncomment your board, or configure a custom board in Settings.h
//#define USE_WROVER_BOARD
//#define USE_TTGO_T7
 
#include "BlynkEdgent.h"
 
//Widget Setups
 
//WidgetLED led1(V4);
//WidgetLED led2(V5);
//WidgetLED led3(V6);
//WidgetLED led4(V7);
//WidgetLED led5(V8);
//WidgetLED led6(V9);
//WidgetLED led7(V10);
//WidgetLED led8(V55);
//WidgetLED led9(V56);
//WidgetLED led10(V57);
//WidgetLED led11(V58);
const int ledArray[] = {V7,V8,V9,V10};
const int soakLedArray[] = {V55,V56,V57,V58};
const int sensorData[] = {V0,V1,V2,V3};
const int triggerDisplay[] = {V11,V12,V13,V14};
const int soakTimeArray[] = {V27,V28,V29,V30};
const int countdownArray[] = {V35,V36,V37,V38};
const int pumpTimeArray[] = {V47,V48,V49,V50};
int ledStatus[] = {0,0,0,0};
int soakLedStatus[] = {0,0,0,0};
int zoneState[4]; 
 
// Set Pin Assignments For Output Pins
 
// MOSFETS:
const int output[] = {4,16,17,18};
//Motor Drivers:
//U4 Motor Driver:
const int mtrOutA1 = 22;
const int mtrOutA2 = 23;
//U7 Motor Driver:
const int mtrOutB1 = 19;
const int mtrOutB2 = 21;
 
// PWM Settings for Motor Drivers
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 255;
 
 
 
//Set Pin Assignments for Input Pins
 
//Analog Inputs:
 
//Moisture Sensors:
const int sensPin[] = {35,34,39,36};
 
//Water Level Sensors:
const int wtrLvlTop = 26;
const int wtrLvlBtm = 27;
 
 
//Zone Loop Controls
 
int zoneNumber = 0;
int blynkZone = 0;
int triggerLow[] = {3600,3600,3600,3600};
int triggerHigh[] = {2700,2700,2700,2700};
int triggerSpread[] = {700,700,700,700};
bool systemActive = true;
bool zoneActive[] = {true, true, true, true};
bool zoneManual[] = {false,false,false,false};
bool zoneAuto[] = {true,true,true,true};
bool zonePump[] = {false,false,false,false};
bool zoneManualPump[] = {false,false,false,false};
bool zoneSoak[] = {false,false,false,false};
int waterStatus[] = {0,0,0,0};
 
 
// Sensor Data:
int sensor[] = {0, 0, 0, 0}; // Sensor reading values array
int topWtrLvl = 0;
int btmWtrLvl = 0;
int lowWater = 3500;
int highWater = 3500;
 
// Timer Variables
 
const int sensorReadDelay[] = {5000, 5000, 5000, 5000}; // delay between sensor readings in millis
unsigned long lastSensorReadTime[] = {0, 0, 0, 0}; // the last time a sensor was read
int manualDayTimer[] = {86400000, 86400000, 86400000, 86400000}; // delay for x times a day manual watering mode
unsigned long lastManualDayTimer[] = {0,0,0,0,};
unsigned long pumpCountDown[] = {0,0,0,0};
unsigned long pumpTimer[] = {60000, 60000, 60000, 60000}; //Pump timers in array
unsigned long soakTimer[] = {43200000, 43200000, 43200000, 43200000}; //soak timers in array
unsigned long lastPumpTimer[] = {0, 0, 0, 0}; // last value of Pump timers in array
unsigned long lastSoakTimer[] = {0, 0, 0, 0}; // last value of soak timers in array
 
BlynkTimer timer;
 
void setup()
{
  Serial.begin(115200);
  delay(100);
  
  // Set OUT Pins to Output
  pinMode(output[0], OUTPUT);
  pinMode(output[1], OUTPUT);
  pinMode(output[2], OUTPUT);
  pinMode(output[3], OUTPUT);
  pinMode(mtrOutA1, OUTPUT);
  pinMode(mtrOutA2, OUTPUT);
  pinMode(mtrOutB1, OUTPUT);
  pinMode(mtrOutB2, OUTPUT);
 
  // Set PWM
 
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(mtrOutA2, pwmChannel);
  ledcAttachPin(mtrOutB2, pwmChannel);
 
  // Set Sensor Pins to Input
  pinMode(sensPin[0], INPUT);
  pinMode(sensPin[1], INPUT);
  pinMode(sensPin[2], INPUT);
  pinMode(sensPin[3], INPUT);
 
  // Ensure Mosfets are pulled low
  digitalWrite(output[0], LOW);
  digitalWrite(output[1], LOW);
  digitalWrite(output[2], LOW);
  digitalWrite(output[3], LOW);
 
 
  // set Water Level Pins to Input
  pinMode(wtrLvlTop, INPUT);
  pinMode(wtrLvlBtm, INPUT);
 
  //Blynk Timers
  BlynkEdgent.begin();
  timer.setInterval(250, blynkLoop);

 
}
 
BLYNK_CONNECTED() {
    Blynk.syncAll();
} 
 
void zoneLoop() {
  for (zoneNumber = 0; zoneNumber < 3; zoneNumber++) {
    if (systemActive) {
      zoneControl();
      zoneControlManual();
      zoneControlPump();
      zoneSoakCycle();
    }
    else {
      zonePump[zoneNumber] = false;
      sensor[zoneNumber] = 0;
      digitalWrite(output[zoneNumber], LOW);
    }
    waterLevel();
  }
  for (zoneNumber = 3; zoneNumber > 0; zoneNumber = 0) {
    if (systemActive) {
      zoneControl();
      zoneControlManual();
      zoneControlPump();
      zoneSoakCycle();
    }
    else {
      zonePump[zoneNumber] = false;
      sensor[zoneNumber] = 0;
      digitalWrite(output[zoneNumber], LOW);
    }
    waterLevel();
  }
}
 
void blynkLoop(){
  for (blynkZone = 0; blynkZone < 3; blynkZone++) {
    blynkData();
  }
  for (blynkZone = 3; blynkZone > 0; blynkZone = 0) {
    blynkData();
  }
}
 
void blynkData(){
  if (systemActive) {
    Blynk.virtualWrite(V5, LOW);
  }
  else {
    Blynk.virtualWrite(V5, HIGH);
  }
  Blynk.virtualWrite(sensorData[blynkZone], sensor[blynkZone]);
  Blynk.virtualWrite(triggerDisplay[blynkZone], triggerLow[blynkZone]);
  Blynk.virtualWrite(pumpTimeArray[blynkZone], (pumpTimer[blynkZone] / 60000));
  Blynk.virtualWrite(soakTimeArray[blynkZone], (soakTimer[blynkZone] / 3600000));
  if (zonePump[blynkZone]) {
    Blynk.virtualWrite(ledArray[blynkZone], HIGH);
  }
  else {
    Blynk.virtualWrite(ledArray[blynkZone], LOW);
  }
  if (zoneSoak[blynkZone]) {
    Blynk.virtualWrite(soakLedArray[blynkZone], HIGH);
  }
  else {
    Blynk.virtualWrite(soakLedArray[blynkZone], LOW);
  }
  if (zoneManual[blynkZone]) {
    pumpCountDown[blynkZone] = ((unsigned long)(millis() - lastManualDayTimer[blynkZone])) - manualDayTimer[blynkZone];
    pumpCountDown[blynkZone] = pumpCountDown[blynkZone] * -1;
    Blynk.virtualWrite(countdownArray[blynkZone], pumpCountDown[blynkZone] / 3600000);
  }
  else {
    Blynk.virtualWrite(countdownArray[blynkZone], 0);
  }
}
 
 
void waterLevel()
{
  int btmWtrLvl = digitalRead(wtrLvlBtm);
 
  if (btmWtrLvl == 1) {
    systemActive = false;
  }
  else if (btmWtrLvl == 0) {
    systemActive = true;
  }
}
 
 
void zoneSoakCycle()
{
  if (zoneSoak[zoneNumber]) {
    if ((unsigned long)(millis() - lastSoakTimer[zoneNumber]) >= soakTimer[zoneNumber])
    {
      zoneSoak[zoneNumber] = false;
      zoneActive[zoneNumber] = true;
      lastManualDayTimer[zoneNumber] = millis();
    }
  } 
}
 
 
void zoneControlManual() {
  if (zoneManual[zoneNumber]) {
    if (!zonePump[zoneNumber] && !zoneManualPump[zoneNumber]) {
      digitalWrite(output[zoneNumber], LOW);
      sensor[zoneNumber] = 0;
      if ((unsigned long)(millis() - lastManualDayTimer[zoneNumber]) > manualDayTimer[zoneNumber]) {
          zoneManualPump[zoneNumber] = true;
          digitalWrite(output[zoneNumber], HIGH);
          lastPumpTimer[zoneNumber] = millis();
          lastManualDayTimer[zoneNumber] = millis();
          zoneManual[zoneNumber] = false;
      }
    }
  }
}
 
 
void zoneControl() {
  if (zoneAuto[zoneNumber]) {
    if (zoneActive[zoneNumber] && (!zonePump[zoneNumber] && !zoneManualPump[zoneNumber])) {
      sensor[zoneNumber] = analogRead(sensPin[zoneNumber]);
      digitalWrite(output[zoneNumber], LOW);   
      if ((unsigned long)(millis() - lastSensorReadTime[zoneNumber]) >= sensorReadDelay[zoneNumber]) {
        lastSensorReadTime[zoneNumber] = millis();
 
        if (sensor[zoneNumber] >= triggerLow[zoneNumber]) {
          zonePump[zoneNumber] = true;
          digitalWrite(output[zoneNumber], HIGH);
          lastPumpTimer[zoneNumber] = millis();
          zoneActive[zoneNumber] = false;
        } 
      }
    }    
  }
}
 
 
void zoneControlPump() {    
    if (zonePump[zoneNumber]) {
      if ((unsigned long)(millis() - lastPumpTimer[zoneNumber]) >= pumpTimer[zoneNumber]) {
        digitalWrite(output[zoneNumber], LOW);
        zoneSoak[zoneNumber] = true;
        lastSoakTimer[zoneNumber] = millis();
        lastSensorReadTime[zoneNumber] = millis();
        zonePump[zoneNumber] = false;
        zoneManualPump[zoneNumber] = false;
        lastManualDayTimer[zoneNumber] = millis();
      }
    }
    else if (zoneManualPump[zoneNumber]) {
      if ((unsigned long)(millis() - lastPumpTimer[zoneNumber]) >= pumpTimer[zoneNumber]) {
        digitalWrite(output[zoneNumber], LOW);
        zoneManual[zoneNumber] = true;
        zonePump[zoneNumber] = false;
        zoneManualPump[zoneNumber] = false;
        lastManualDayTimer[zoneNumber] = millis();
      }
    }
    else if (!zoneManualPump[zoneNumber] && !zonePump[zoneNumber]) {
      digitalWrite(output[zoneNumber], LOW);
    }
}    
 
 
 
BLYNK_WRITE(V15)
{
  triggerLow[0] = param.asInt();
  triggerHigh[0] = triggerLow[0] - triggerSpread[0];
}
 
BLYNK_WRITE(V16)
{
  triggerLow[1] = param.asInt();
  triggerHigh[1] = triggerLow[1] - triggerSpread[1];
}
 
BLYNK_WRITE(V17)
{
  triggerLow[2] = param.asInt();
  triggerHigh[2] = triggerLow[2] - triggerSpread[2];
}
 
BLYNK_WRITE(V18)
{
  triggerLow[3] = param.asInt();
  triggerHigh[3] = triggerLow[3] - triggerSpread[3];
}
 
BLYNK_WRITE(V19)
{
  zoneState[0] = param.asInt();
  if (zoneState[0] == 0) {
    zoneActive[0] = false;
    zoneManual[0] = true;
    lastPumpTimer[0] = millis();
    lastManualDayTimer[0] = millis();
    digitalWrite(output[0], LOW);
    ledStatus[0] = 0;
    Blynk.virtualWrite(ledArray[0], LOW);
    Blynk.virtualWrite(soakLedArray[0], LOW);
  }
  else {
    zoneActive[0] = true;
    zoneSoak[0] = false;
    zoneManual[0] = false;
    lastPumpTimer[0] = millis();
    lastManualDayTimer[0] = millis();
  }
}
 
BLYNK_WRITE(V20)
{
  zoneState[1] = param.asInt();
  if (zoneState[1] == 0) {
    zoneActive[1] = false;
    zoneManual[1] = true;
    lastPumpTimer[1] = millis();
    lastManualDayTimer[1] = millis();
    digitalWrite(output[1], LOW);
    ledStatus[1] = 0;
    Blynk.virtualWrite(ledArray[1], LOW);
    Blynk.virtualWrite(soakLedArray[1], LOW);
  }
  else {
    zoneActive[1] = true;
    zoneSoak[1] = false;
    zoneManual[1] = false;
    lastPumpTimer[1] = millis();
    lastManualDayTimer[1] = millis();
  }
}
 
BLYNK_WRITE(V21)
{
  zoneState[2] = param.asInt();
  if (zoneState[2] == 0){
    zoneActive[2] = false;
    zoneManual[2] = true;
    lastPumpTimer[2] = millis();
    lastManualDayTimer[2] = millis();
    digitalWrite(output[2], LOW);
    ledStatus[2] = 0;
    Blynk.virtualWrite(ledArray[2], LOW);
    Blynk.virtualWrite(soakLedArray[2], LOW);
  }
  else {
    zoneActive[2] = true;
    zoneSoak[2] = false;
    zoneManual[2] = false;
    lastPumpTimer[2] = millis();
    lastManualDayTimer[2] = millis();
  }
}
 
BLYNK_WRITE(V22)
{
  zoneState[3] = param.asInt();
  if (zoneState[3] == 0){
    zoneActive[3] = false;
    zoneManual[3] = true;
    lastPumpTimer[3] = millis();
    lastManualDayTimer[3] = millis();
    digitalWrite(output[3], LOW);
    ledStatus[3] = 0;
    Blynk.virtualWrite(ledArray[3], LOW);
    Blynk.virtualWrite(soakLedArray[3], LOW);
  }
  else {
    zoneActive[3] = true;
    zoneSoak[3] = false;
    zoneManual[3] = false;
    lastPumpTimer[3] = millis();
    lastManualDayTimer[3] = millis();
  }
}
 
 
BLYNK_WRITE(V31)
{
  if (zoneSoak[0]) {
    zoneSoak[0] = false;
    zoneActive[0] = true;
  }
  zonePump[0] = true;
  digitalWrite(output[0], HIGH);
  lastPumpTimer[0] = millis();
  zoneActive[0] = false;
  lastPumpTimer[0] = millis();
}
 
BLYNK_WRITE(V32)
{
  if (zoneSoak[1]) {
    zoneSoak[1] = false;
    zoneActive[1] = true;
  }
  zonePump[1] = true;
  digitalWrite(output[1], HIGH);
  lastPumpTimer[1] = millis();
  zoneActive[1] = false;
  lastPumpTimer[1] = millis();
}
 
BLYNK_WRITE(V33)
{
  if (zoneSoak[2]) {
    zoneSoak[2] = false;
    zoneActive[2] = true;
  }
  zonePump[2] = true;
  digitalWrite(output[2], HIGH);
  lastPumpTimer[2] = millis();
  zoneActive[2] = false;
  lastPumpTimer[2] = millis();
}
 
BLYNK_WRITE(V34)
{
  if (zoneSoak[3]) {
    zoneSoak[3] = false;
    zoneActive[3] = true;
  }
  zonePump[3] = true;
  digitalWrite(output[3], HIGH);
  lastPumpTimer[3] = millis();
  zoneActive[3] = false;
  lastPumpTimer[3] = millis();
}
 
BLYNK_WRITE(V39)
{
  manualDayTimer[0] = param.asInt();
}
 
BLYNK_WRITE(V40)
{
  manualDayTimer[1] = param.asInt();
}
 
BLYNK_WRITE(V41)
{
  manualDayTimer[2] = param.asInt();
}
 
BLYNK_WRITE(V42)
{
  manualDayTimer[3] = param.asInt();
}

BLYNK_WRITE(V23)
{
  soakTimer[0] = (param.asInt() * 3600000);
}
 
BLYNK_WRITE(V24)
{
  soakTimer[1] = (param.asInt() * 3600000);
}
 
BLYNK_WRITE(V25)
{
  soakTimer[2] = (param.asInt() * 3600000);
}
 
BLYNK_WRITE(V26)
{
  soakTimer[3] = (param.asInt() * 3600000);
}
 
BLYNK_WRITE(V43)
{
  pumpTimer[0] = (param.asInt() * 60000);
}
 
BLYNK_WRITE(V44)
{
  pumpTimer[1] = (param.asInt() * 60000);
}
 
BLYNK_WRITE(V45)
{
  pumpTimer[2] = (param.asInt() * 60000);
}
 
BLYNK_WRITE(V46)
{
  pumpTimer[3] = (param.asInt() * 60000);
}
 
BLYNK_WRITE(V51)
{
  if (param.asInt() == 0) {
    zoneActive[0] = false;
    zonePump[0] = false;
    zoneSoak[0] = false;
    digitalWrite(output[0], LOW);
    ledStatus[0] = 0;
    sensor[0] = 0;
    Blynk.virtualWrite(ledArray[0], LOW);
    Blynk.virtualWrite(soakLedArray[0], LOW);
  }
  else {
    zoneActive[0] = true;
  }
}
 
BLYNK_WRITE(V52)
{
  if (param.asInt() == 0) {
    zoneActive[1] = false;
    zonePump[1] = false;
    zoneSoak[1] = false;
    digitalWrite(output[1], LOW);
    ledStatus[1] = 0;
    sensor[1] = 0;
    Blynk.virtualWrite(ledArray[1], LOW);
    Blynk.virtualWrite(soakLedArray[1], LOW);
  }
  else {
    zoneActive[1] = true;
  }
}
 
BLYNK_WRITE(V53)
{
  if (param.asInt() == 0) {
    zoneActive[2] = false;
    zonePump[2] = false;
    zoneSoak[2] = false;
    digitalWrite(output[2], LOW);
    ledStatus[2] = 0;
    sensor[2] = 0;
    Blynk.virtualWrite(ledArray[2], LOW);
    Blynk.virtualWrite(soakLedArray[2], LOW);
  }
  else {
    zoneActive[2] = true;
  }
}
 
BLYNK_WRITE(V54)
{
  if (param.asInt() == 0) {
    zoneActive[3] = false;
    zonePump[3] = false;
    zoneSoak[3] = false;
    digitalWrite(output[3], LOW);
    ledStatus[3] = 0;
    sensor[3] = 0;
    Blynk.virtualWrite(ledArray[3], LOW);
    Blynk.virtualWrite(soakLedArray[3], LOW);
  }
  else {
    zoneActive[3] = true;
  }
}
 
void loop() {
  BlynkEdgent.run();
  timer.run();
  zoneLoop();
}