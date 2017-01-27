/*
   Copyright 2015-2017 AllThingsTalk

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

This sketch is an example sketch to deploy the Grove Sunlight sensor (101020089) to the
AllThingsTalk IoT developer cloud.

*/


#include <Ethernet2.h>
#include <EthernetClient.h>

#include <PubSubClient.h>
#include <ATT_IOT.h>                            //AllThingsTalk IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include "keys.h"              					// Keep all your personal account information in a seperate file
#include "SI114X.h"

ATTDevice Device(DEVICEID, CLIENTID, CLIENTKEY);            //create the object that provides the connection to the cloud to manager the device.
#define httpServer "api.AllThingsTalk.io"                  // HTTP API Server host                  
#define mqttServer httpServer                				// MQTT Server Address 


// Define PIN numbers for assets
// For digital and analog sensors, we recommend to use the physical pin id as the asset id
// For other sensors (I2C and UART), you can select any unique number as the asset id
SI114X SI1145 = SI114X();
#define visLightId 0
#define irLightId 1
#define uvLightId 2

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
EthernetClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);

void setup()
{
  Serial.begin(57600);                                 // Init serial link for debugging
  while(!Serial && millis() < 1000);                   // Make sure you see all output on the monitor. After 1 sec, it will skip this step, so that the board can also work without being connected to a pc  Serial.println("Starting sketch");
  
  byte mac[] = {0x90, 0xA2, 0xDA, 0x0D, 0xE1, 0x3E};                                // Adapt to your Arduino MAC Address  
  if (Ethernet.begin(mac) == 0)                                                     // Initialize the Ethernet connection:
  { 
    Serial.println(F("DHCP failed,end"));
    while(true);                                                                //we failed to connect, halt execution here. 
  }
  
  while(!Device.Connect(&ethClient, httpServer))                                // connect the device with the IOT platform.
    Serial.println("retrying");

  // Create the sensor assets for your device
  Device.AddAsset(visLightId, "visual light", "visual light", false, "{\"type\": \"integer\", \"minimum\": 0, \"unit\": \"lm\"}");   // Create the Sensor asset for your device
  Device.AddAsset(irLightId, "infra red", "infra red light", false, "{\"type\": \"integer\", \"minimum\": 0, \"unit\": \"lm\"}");
  Device.AddAsset(uvLightId, "UV", "ultra violet light", false, "{\"type\": \"number\", \"minimum\": 0, \"unit\": \"UV index\"}");


  while(!Device.Subscribe(pubSub))                                              // make certain that we can receive message from the iot platform (activate mqtt)
    Serial.println("retrying");

  //pinMode(sunlightId, INPUT);                       // Initialize the digital pin as an input.

  Serial.println("Beginning Si1145!");
  while (!SI1145.Begin()) {
    Serial.println("Si1145 is not ready!");
    delay(1000);
  }
  Serial.println("Si1145 sunlight sensor is ready for use!");
}

void loop()
{
  Serial.println("//--------------------------------------//");
  int visValue = SI1145.ReadVisible();
  Serial.print("Vis: "); 
  Serial.println(visValue);
  Device.Send(String(visValue), visLightId);
  
  int irValue = SI1145.ReadIR();
  Serial.print("IR: "); 
  Serial.println(irValue);
  Device.Send(String(irValue), irLightId);
  
  //the real UV value must be div 100 from the reg value , datasheet for more information.
  float uvValue = (float)SI1145.ReadUV()/100;
  Serial.print("UV: ");  
  Serial.println(uvValue);
  Device.Send(String(uvValue), uvLightId);
  
  Device.Process(); 
  delay(1000);
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
