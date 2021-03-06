/* 
Camera Controller

Initial Code to trigger twelve cameras in sequence just before a user is
about to jump.  Currently the code using just a load cell sensor to sense or
weight increases once it's taken a sample of a user's initial weight.  If the
sensor detects a significant enough increase in the weight of the person, it
can be assumed that the user is exerting force down on the sensor in preparation 
for a jump.

Created by Ezer Longinus
Date March 10 2014
Version 0.1

*/

#include "Wire.h"

byte zeroByte = 0x00;
/*------------------  CAMERA TIMING --------------------------*/
/* Adjust this number to configure the 
timing between cameras triggering */
int cameraTiming = 500;
/*------------------------------------------------------------*/


/*--------------- LOAD CELL WEIGHT ADJUSTMENTS ---------------*/
/* Change the multiplier if you are noticing jumping force is more
dependent on weight.  For instance if for every 10lbs people exert an
extra 1lb of force before they jump.  If you don't notice anything, keep
it at "1" */
int thresWeightMulti = 1;
/* Adjust this number if you are noticing jumps not being activated, or
triggering cameras too early.
*/
int thresWeightAdd = 100;
/*------------------------------------------------------------*/

unsigned long takeOffTime = 0;
unsigned long landingTime = 0;


// Analog Input of the load cell
int loadCellInput = A6;
// The initial reading we get to test against all future readings before a user jumps.
float baseLineLoadCell = 0;
// The values we're looking at while on the load sensor.
float rawLoadCell = 0;

// Digital Output for the Ultrasound Sensors.
// This is so you can pulse them on and off if needed.
// They're default state will be on.
int pulseSensor1 = 2;
int pulseSensor2 = 3;
int pulseSensor3 = 4;
int pulseSensor4 = 5;
// Analog Input of the Ultrasound Sensor
int ultraSensor1 = A0;
int ultraSensor2 = A1;
int ultraSensor3 = A2;
int ultraSensor4 = A3;


// Switch statement to prepare to start reading the sensor(s)
boolean readyToJump = false;
boolean justJumped = false;

// Incase you want to use it.
byte incomingByte;

void setup()
{
  Serial.begin(9600);
   
  Wire.begin(); // wake up I2C bus
   
  // set I/O pins to outputs
  Wire.beginTransmission(0x20);
  Wire.write(zeroByte); // IODIRA register
  Wire.write(zeroByte); // set all of port A to outputs
  Wire.endTransmission();
   
  Wire.beginTransmission(0x20);
  Wire.write(0x01); // IODIRB register
  Wire.write(zeroByte); // set all of port B to outputs
  Wire.endTransmission();
  
  pinMode(pulseSensor1, OUTPUT);
  pinMode(pulseSensor2, OUTPUT);
  pinMode(pulseSensor3, OUTPUT);
  pinMode(pulseSensor4, OUTPUT);
  
  digitalWrite(pulseSensor1, HIGH);
  digitalWrite(pulseSensor2, HIGH);
  digitalWrite(pulseSensor3, HIGH);
  digitalWrite(pulseSensor4, HIGH);
  
  //Initialize all cameras to be off 
  allCamerasOff();
}

void loop()
{
  analogRead(loadCellInput);
  if(Serial.available() > 0){
    incomingByte = Serial.read();
    Serial.println("Sensor Primed. Ready to Jump");
    readyToJump = true;
    baseLineLoadCell = analogRead(loadCellInput);
  }
  //Serial.println(analogRead(loadCellInput));
  
  if(readyToJump){
    rawLoadCell = analogRead(loadCellInput);
    Serial.print(baseLineLoadCell);
    Serial.print(" ");
    Serial.println(rawLoadCell);
    if(rawLoadCell > (thresWeightMulti * baseLineLoadCell) + thresWeightAdd){
      
     Serial.print("----------------Takeoff Time: ");
     takeOffTime = millis();
     Serial.println(takeOffTime);
     justJumped = true;
        
     cameraTrigger();
     readyToJump = false;
    }
  }
  
  if(justJumped && !readyToJump){
    //not currently working because of delays in camera loop
    Serial.println("Just Jumped");
    rawLoadCell = analogRead(loadCellInput);
    Serial.print(baseLineLoadCell);
    Serial.print(" ");
    Serial.println(rawLoadCell);
  }
  
 // if(justJumped && rawLoadCell  (thresWeightMulti * baseLineLoadCell) + thresWeightAdd){
     //not sure if this is the best logic here to detect a landing
  if(justJumped && rawLoadCell == 800){
         rawLoadCell = analogRead(loadCellInput);
         Serial.print("LANDED------------------------------");
         Serial.print(baseLineLoadCell);
    Serial.print(" ");
    Serial.println(rawLoadCell);
    
    
     Serial.print("Landing Time: ");
    landingTime = millis();
    Serial.println(landingTime);
    Serial.print("Time in Air: ");
    landingTime = millis();
    Serial.println(landingTime-takeOffTime);
    
    justJumped = false;
    delay(1000);
    allCamerasOff();
  }
}

void cameraTrigger()
{
 for (byte a=0; a < 9; a++)
 {
   if(a == 1){
     continue;
   } else {
     Wire.beginTransmission(32);
     Wire.write(18); // GPIOA
     Wire.write((int)pow(2,a)); // port A 0 = 0, 1 = Null/skipped, 2 = 4, 3 = 9, 4 = 16, 5 = 25, 6 = 36, 7 = 49, 8 = 64
     Wire.endTransmission();
     delay(cameraTiming);
   }
 }
 for(byte b=0; b < 5; b++)
 {
   if(b == 1){
     continue;
   } else {
     Wire.beginTransmission(32);
     Wire.write(19);
     Wire.write((int)pow(2,b)); 
     Wire.endTransmission();
     delay(cameraTiming);
   }
  }
  
  //-------Experimental backwards traversal
  //doesnt work for some reason?
//   for(byte b=4; b >=0; b--)
// {
//   if(b == 1){
//     continue;
//   } else {
//     Wire.beginTransmission(32);
//     Wire.write(19);
//     Wire.write((int)pow(2,b)); 
//     Wire.endTransmission();
//     delay(cameraTiming);
//   }
//  }
//   for (byte a=8; a >=0; a--)
// {
//   if(a == 1){
//     continue;
//   } else {
//     Wire.beginTransmission(32);
//     Wire.write(18); // GPIOA
//     Wire.write((int)pow(2,a)); // port A 0 = 0, 1 = Null/skipped, 2 = 4, 3 = 9, 4 = 16, 5 = 25, 6 = 36, 7 = 49, 8 = 64
//     Wire.endTransmission();
//     delay(cameraTiming);
//   }
// }
 
 
}

void allCamerasOff()
{
 for (byte a=0; a < 9; a++)
 {
 Wire.beginTransmission(0x20);
 Wire.write(0x12); // GPIOA
 Wire.write(zeroByte); // port A
 Wire.endTransmission();
 Wire.beginTransmission(0x20);
 }
  for (byte b=0; b < 5; b++)
 {
 Wire.beginTransmission(0x20);
 Wire.write(0x13); // GPIOA
 Wire.write(zeroByte); // port A
 Wire.endTransmission();
 Wire.beginTransmission(0x20);
 }
}
