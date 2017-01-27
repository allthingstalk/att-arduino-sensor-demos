/*
   Copyright 2015-2017 AllThingsTalk

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

This sketch is an example sketch to deploy the Grove Relay (103020005) to the
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
#define relayId 2                // Digital actuator is connected to pin D2 on grove shield

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

  Device.AddAsset(relayId, "Relay", "relay", true, "boolean");   // Create the actuator asset for your device

  while(!Device.Subscribe(pubSub))                                              // make certain that we can receive message from the iot platform (activate mqtt)
    Serial.println("retrying");

  pinMode(relayId, OUTPUT);                            // Initialize the digital pin as an input.
  Device.Send("false", relayId);
  Serial.println("Relay is ready for use!");
}

void loop()
{
  Device.Process();
}

// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  String msgString; 
  {                                                           //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
    char message_buff[length + 1];                        //need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation
    strncpy(message_buff, (char*)payload, length);        //copy over the data
    message_buff[length] = '\0';                      //make certain that it ends with a null            
    msgString = String(message_buff);
  }
  Serial.print("Payload: ");                            //show some debugging
  Serial.println(msgString);
  Serial.print("topic: ");
  Serial.println(topic);
  
  int pinNr = Device.GetPinNr(topic, strlen(topic));
  if(pinNr == relayId)
  {
    msgString.toLowerCase();                  //to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)	  
	if(msgString == "true")
      digitalWrite(relayId, HIGH);
    else    
      digitalWrite(relayId, LOW);
    Device.Send(msgString, relayId);              // Send the value back for confirmation
  }

}
