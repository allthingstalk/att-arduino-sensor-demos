/*
   Copyright 2015-2016 AllThingsTalk

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

This sketch is an example sketch to deploy the Grove gas sensor (MQ5) (101020055) or Grove
gas sensor (MQ2) (101020056) to the AllThingsTalk IoT developer cloud.

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
#define gasId 0                // Analog sensor is connected to pin A0 on grove shield

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
EthernetClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);

void setup()
{
  Serial.begin(57600);                                 // Init serial link for debugging
  while(!Serial && millis() < 1000);                   // Make sure you see all output on the monitor. After 1 sec, it will skip this step, so that the board can also work without being connected to a pc
  Serial.println("Start");
  
  byte mac[] = {0x90, 0xA2, 0xDA, 0x0D, 0xE1, 0x3E};                                // Adapt to your Arduino MAC Address  
  if (Ethernet.begin(mac) == 0)                                                     // Initialize the Ethernet connection:
  { 
    Serial.println(F("nodchpend"));
    while(true);                                                                //we failed to connect, halt execution here. 
  }
  
  //while(!Device.Connect(&ethClient, httpServer))                                // connect the device with the IOT platform.
  //  Serial.println("retry");

  //Device.AddAsset(gasId, "Gas sensor(MQ5)", "gas", false, "number");   // Create the sensor asset for your device

  while(!Device.Subscribe(pubSub))                                              // make certain that we can receive message from the iot platform (activate mqtt)
    Serial.println("retry");
    
  pinMode(gasId, INPUT);                               // Initialize the digital pin as an input.
  Serial.println("ready");

}

float voltage = 5.0;
void loop()
{
  float sensor_volt; 
  float RS_air; //  Get the value of RS via in a clear air
  float R0;  // Get the value of R0 via in H2
  float sensorValue = 0.0;
 
  for(int x = 0 ; x < 100 ; x++)
  {
    sensorValue = sensorValue + analogRead(A0);
  }
  sensorValue = sensorValue/100.0;
 
  sensor_volt = sensorValue/1024*voltage;
  RS_air = (voltage-sensor_volt)/sensor_volt; // omit *RL
  R0 = RS_air/6.5; // The ratio of RS/R0 is 6.5 in a clear air from Graph (Found using WebPlotDigitizer)
 
  Serial.print("volt= ");
  Serial.print(sensor_volt);
  Serial.println("V");
 
  Serial.print("R0= ");
  Serial.println(R0);
  delay(1000);

  Device.Send(String(R0), gasId);
  
  Device.Process(); 
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  
}

