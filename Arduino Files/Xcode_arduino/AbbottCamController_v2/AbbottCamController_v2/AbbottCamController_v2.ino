// 
// AbbottCamController_v2 
//
// Description of the project
// Developed with [embedXcode](http://embedXcode.weebly.com)
// 
// Author	 	Blair Neal
// 				Blair Neal
//
// Date			3/13/14 10:48 AM
// Version		<#version#>
// 
// Copyright	© Blair Neal, 2014
// License		<#license#>
//
// See			ReadMe.txt for references
//

// Core library for code-sense
#if defined(WIRING) // Wiring specific
#include "Wiring.h"
#elif defined(MAPLE_IDE) // Maple specific
#include "WProgram.h"   
#elif defined(MICRODUINO) // Microduino specific
#include "Arduino.h"
#elif defined(MPIDE) // chipKIT specific
#include "WProgram.h"
#elif defined(DIGISPARK) // Digispark specific
#include "Arduino.h"
#elif defined(ENERGIA) // LaunchPad MSP430, Stellaris and Tiva, Experimeter Board FR5739 specific
#include "Energia.h"
#elif defined(TEENSYDUINO) // Teensy specific
#include "Arduino.h"
#elif defined(ARDUINO) // Arduino 1.0 and 1.5 specific
#include "Arduino.h"
#else // error
#error Platform not defined
#endif

// Include application, user and local libraries


/*
 Abbott Camera Controller
 
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
unsigned long previousMillis = 0;
unsigned long startAirTime = 0;
unsigned long endAirTime = 0;
unsigned long totalAirTime = 0;


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
boolean fireCameras = false;

int a = 0;
int b = 0;


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
    
    //Check for incoming serial byte
    char tempBytes[4];
    // see if there's incoming serial data:
    if (Serial.available() > 0) {
        
        Serial.readBytes(tempBytes, 4);
        
        if(tempBytes[0] == 'X' || tempBytes[0] == 'x' ){ //Check for special string
            if(tempBytes[1] == '1' && tempBytes[2] == '1' && tempBytes[3] == '1') { //Prime Jump Trigger - X111
                Serial.println("Sensor Primed. Ready to Jump");
                readyToJump = true;
                baseLineLoadCell = analogRead(loadCellInput);
            }
            if(tempBytes[1] == '2' && tempBytes[2] == '2' && tempBytes[3] == '2') { //Fire Cameras immediately (without priming/load sensor) - X222
                Serial.println("Firing Cameras!");
                //fireCameras = true;
                cameraTriggerWithDelays();
            }
            if(tempBytes[1] == '3' && tempBytes[2] == '3' && tempBytes[3] == '3') { //Get Duration - X333
                Serial.print('C');
                Serial.println(cameraTiming);
            }
            if(tempBytes[1] == '4' && tempBytes[2] == '4' && tempBytes[3] == '4') { //get jump time
                Serial.print('J');
                Serial.println(totalAirTime);
            }
            if(tempBytes[1] == '5' && tempBytes[2] == '5' && tempBytes[3] == '5') { //get threshold - X555
                Serial.print('L');
                Serial.println(thresWeightAdd);
            }
            if(tempBytes[1] == '6' && tempBytes[2] == '6' && tempBytes[3] == '6') { //get current load cell value - X666
                rawLoadCell = analogRead(loadCellInput);
                Serial.println((int)rawLoadCell);
            }
        }
        
        if(tempBytes[0] == 'Q' || tempBytes[0] == 'q'){
            int x = tempBytes[1] - '0'; //char to int conversion
            int y = tempBytes[2] - '0';
            int z = tempBytes[3] - '0';
            cameraTiming = 100*x + 10*y + 1 *z; //Set camera timing interval - send Q050 or Q999
            Serial.println(cameraTiming);
        }
        
        if(tempBytes[0] == 'K' || tempBytes[0] == 'k'){
            thresWeightAdd = 100*tempBytes[1] + 10*tempBytes[2] + 1*tempBytes[3]; //Set camera timing interval - send K050 or K999
        }
        Serial.flush();
    }
    
    //Detect takeoff and toggle variable
    
    
    if(justJumped == false){ //only read if its not currently triggered
        rawLoadCell = analogRead(loadCellInput);
        if(rawLoadCell > (thresWeightMulti * baseLineLoadCell) + thresWeightAdd){
            startAirTime = millis();
            Serial.print("----------------Takeoff Time: ");
            Serial.println(startAirTime);
            justJumped = true;
        }
    }
    
    //Detect landing while also removing false positives
    if(justJumped == true && (millis() - startAirTime > 100 )){
        rawLoadCell = analogRead(loadCellInput);
        if(rawLoadCell >= baseLineLoadCell && rawLoadCell <800){
            endAirTime = millis();
            justJumped = false;
            
            Serial.print("------------------Landing Time: ");
            Serial.println(endAirTime);
            
            Serial.print("------------------Time in Air: ");
            totalAirTime = endAirTime-startAirTime;
            
            Serial.println(totalAirTime);
            
        }
    }
    
    //always loop through camera triggering function
    if(justJumped == true || fireCameras == true){
        cameraTrigger();
    }
    
}

void cameraTrigger(){
    //Run the camera firing
    if(millis() - previousMillis > cameraTiming){ //only fire every interval, but run the loop otherwise
    previousMillis = millis();
    a++;
    if(a < 9){ //fire off the first set of cameras
        if(a ==1){
            //do nothing
        }else{
            Wire.beginTransmission(32);
            Wire.write(18); //toggle this bit to be 1 or on - 0,2,4,8,16,32,64,128
            Wire.write((int)pow(2,a)); //0b00000001 = 0, 0b0000000
            Wire.endTransmission();
        }
    }else{ //now fire the rest
        b++;
        if(b > 5){
            a = 0;
            b = 0;
            justJumped = false;
            delay(100);
            allCamerasOff(); //make sure to set all bytes to 0 after firing
        }
        if(b == 1){
            //do nothing
        } else {
            Wire.beginTransmission(32);
            Wire.write(19);
            Wire.write((int)pow(2,b));
            Wire.endTransmission();
        }
    }
}
    
}

void cameraTriggerWithDelays()
{
    for (byte a=0; a < 9; a++){
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
    
    for(byte b=0; b < 5; b++){
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
    delay(100);
    allCamerasOff();
    
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
