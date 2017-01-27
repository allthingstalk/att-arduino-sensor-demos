/*
   Copyright 2015-2017 AllThingsTalk

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

This sketch is an example sketch to deploy the Grove Rotary angle sensor (101020017) to the
AllThingsTalk IoT developer cloud.

*/

#include <Ethernet2.h>
#include <EthernetClient.h>

#include <PubSubClient.h>
#include <ATT_IOT.h>                            //AllThingsTalk IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include "keys.h"              					// Keep all your personal account information in a seperate file

ATTDevice Device(DEVICEID, CLIENTID, CLIENTKEY);            //create the object that provides the connection to the cloud to manager the device.
#define httpServer "api.AllThingsTalk.io"                  // HTTP API Server host                  
#define mqttServer httpServer                				// MQTT Server Address 


// Define PIN numbers for assets
// For digital and analog sensors, we recommend to use the physical pin id as the asset id
// For other sensors (I2C and UART), you can select any unique number as the asset id
#define rotaryId 2                // Analog sensor is connected to pin A2 on grove shield
#define rotaryColor 3             // Extra asset to show colours in the SmartLiving dashboard

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
EthernetClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);

void setup()
{
  Serial.begin(57600);                                 // Init serial link for debugging
  while(!Serial && millis() < 1000);                   // Make sure you see all output on the monitor. After 1 sec, it will skip this step, so that the board can also work without being connected to a pc
  Serial.println("Starting sketch");
  
  byte mac[] = {0x90, 0xA2, 0xDA, 0x0D, 0xE1, 0x3E};                                // Adapt to your Arduino MAC Address  
  if (Ethernet.begin(mac) == 0)                                                     // Initialize the Ethernet connection:
  { 
    Serial.println(F("DHCP failed,end"));
    while(true);                                                                //we failed to connect, halt execution here. 
  }
  
  while(!Device.Connect(&ethClient, httpServer))                                // connect the device with the IOT platform.
    Serial.println("retrying");

  Device.AddAsset(rotaryId, "Rotary angle sensor", "rotary", false, "integer");   // Create the sensor asset for your device
  Device.AddAsset(rotaryColor, "Colour", "Hot-Cold", false, "string");

  while(!Device.Subscribe(pubSub))                                              // make certain that we can receive message from the iot platform (activate mqtt)
    Serial.println("retrying");

  pinMode(rotaryId, INPUT);                            // Initialize the digital pin as an input.
  Serial.println("Rotary angle sensor is ready for use!");
}

int sensorValAvg = 0;
int sensorReadAvg;
int sampleRate = 50;

unsigned int red, blue;

void loop()
{
  // Calculate average of several values to compensate for sensor wobbling
  sensorReadAvg = 0;
  for(int i=0; i<sampleRate; i++) {
    sensorReadAvg += analogRead(rotaryId);
  }
  sensorReadAvg /= sampleRate;

  // Verify if average value has change
  if (abs(sensorValAvg - sensorReadAvg) > 1)
  {
    sensorValAvg = sensorReadAvg;
    if(sensorValAvg>1020) sensorValAvg = 1020;     // Cap at 1020 (instead of the 1023 max) as we want to map to colours and 1020 = 4x 255

    // Set rgb value for red and blue
    blue = 255 - sensorValAvg / 4;              // 255,0,0 -> 128,0,128 -> 0,0,255
    red = sensorValAvg / 4;

    // Compose string representing the colour in hexadecimal
    String redstr = red <= 15 ? "0" + String(red,HEX) : String(red,HEX);
    String bluestr = blue <= 15 ? "0" + String(blue,HEX) : String(blue,HEX);
    String color = "#" + redstr + "00" + bluestr;

    // Send value for both assets to the platform
    Device.Send(String(sensorValAvg), rotaryId);
    Device.Send(color, rotaryColor);
  }
  Device.Process();
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  char message_buff[length + 1];                        //need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation
  strncpy(message_buff, (char*)payload, length);        //copy over the data
  message_buff[length] = '\0';                      //make certain that it ends with a null           
	  
  Serial.print("Payload: ");                            //show some debugging
  Serial.println(message_buff);
  Serial.print("topic: ");
  Serial.println(topic);  
}

