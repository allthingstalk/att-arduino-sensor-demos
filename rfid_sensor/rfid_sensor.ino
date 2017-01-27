/*
   Copyright 2015-2017 AllThingsTalk

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

This sketch is an example sketch to deploy the Grove RFID reader (113020002) to the
AllThingsTalk IoT developer cloud.

*/

#include <Ethernet2.h>
#include <EthernetClient.h>

#include <PubSubClient.h>
#include <ATT_IOT.h>                            //AllThingsTalk IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include "keys.h"              					// Keep all your personal account information in a seperate file
#include <SoftwareSerial.h>

ATTDevice Device(DEVICEID, CLIENTID, CLIENTKEY);            //create the object that provides the connection to the cloud to manager the device.
#define httpServer "api.AllThingsTalk.io"                  // HTTP API Server host                  
#define mqttServer httpServer                				// MQTT Server Address 


// Define PIN numbers for assets
// For digital and analog sensors, we recommend to use the physical pin id as the asset id
// For other sensors (I2C and UART), you can select any unique number as the asset id
#define readerId 0                // Analog sensor is connected to pin A0 on grove shield
SoftwareSerial SoftSerial(2, 3);
char buffer[64];                  // Buffer array for data receive over serial port of the RFID device
int count=0;                      // Counter for buffer array 

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
EthernetClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);

void setup()
{
  Serial.begin(57600);                                 // Init serial link for debugging
  while(!Serial && millis() < 1000);                   // Make sure you see all output on the monitor. After 1 sec, it will skip this step, so that the board can also work without being connected to a pc  Serial.println("Starting sketch");
  SoftSerial.begin(9600);

  byte mac[] = {0x90, 0xA2, 0xDA, 0x0D, 0xE1, 0x3E};                                // Adapt to your Arduino MAC Address  
  if (Ethernet.begin(mac) == 0)                                                     // Initialize the Ethernet connection:
  { 
    Serial.println(F("DHCP failed,end"));
    while(true);                                                                //we failed to connect, halt execution here. 
  }
  
  while(!Device.Connect(&ethClient, httpServer))                                // connect the device with the IOT platform.
    Serial.println("retrying");

  Device.AddAsset(readerId, "RFID reader", "reader", false, "string");   // Create the sensor asset for your device

  while(!Device.Subscribe(pubSub))                                              // make certain that we can receive message from the iot platform (activate mqtt)
    Serial.println("retrying");

  Serial.println("RFID reader is ready for use!");
}

void clearBufferArray()                 // Function to clear buffer array
{
  for (int i=0; i<sizeof(buffer);i++)
     buffer[i]=NULL;                        // Clear all indices of array with command NULL
  count = 0;                                // Set counter of while loop to zero      
}


void loop()
{
  if (SoftSerial.available())                 // Data is coming from software serial port comes from SoftSerial shield
  {
    bool foundStart = false;
	  while(SoftSerial.available())          // reading RFID data into char array 
    {
      char val = SoftSerial.read();
      if(val == 2)                              // 2 indicates start of text
        foundStart = true;
      else if(foundStart == true){
        if(val == 3 || count == 64) break;      //3 indicates end of text.
        buffer[count++]= val;     // writing RFID data into array
      }
      delay(10);                              // Delay because the Genuino 101 seems to be to fast for the serial port routine
    }
    buffer[count++] = '\0';                   // Needs a terminating String char.
    Serial.print("RFID tag: ");Serial.println(buffer);
    Device.Send(buffer, readerId);    
    clearBufferArray();                       // Call clearBufferArray function to clear the stored data from the array
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


