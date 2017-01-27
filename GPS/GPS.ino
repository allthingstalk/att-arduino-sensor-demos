/*
   Copyright 2015-2016 AllThingsTalk

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

This sketch is an example sketch to deploy the Grove GPS module (113020003) to the
AllThingsTalk IoT developer cloud

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
#define gpsId 2                // Digital sensor is connected to pin D2 on grove shield

SoftwareSerial SoftSerial(2, 3);
unsigned char buffer[64];                   // Buffer array for data receive over serial port
int count=0;

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

  Device.AddAsset(gpsId, "GPS module", "GPS", false, "{\"type\": \"object\",\"properties\": {\"lat\": {\"type\": \"number\", \"unit\": \"°\"},\"lng\": {\"type\": \"number\", \"unit\": \"°\"},\"alt\": {\"type\": \"number\", \"unit\": \"°\"},\"time\": {\"type\": \"integer\", \"unit\": \"epoc time\"}},\"required\": [\"lat\",\"lng\",\"alt\",\"time\" ]}");   // Create the sensor asset for your device

  while(!Device.Subscribe(pubSub))                                              // make certain that we can receive message from the iot platform (activate mqtt)
    Serial.println("retrying");

  //pinMode(gpsId, INPUT);                               // Initialize the digital pin as an input.
  SoftSerial.begin(9600);                 // the SoftSerial baud rate
  Serial.println("GPS module is ready for use!");
}


float longitude;
float latitude;
float altitude;
float timestamp;

void loop() 
{
  if(readCoordinates() == true) SendValue();
  Device.Process();
  delay(2000);
}

void SendValue()
{
    Serial.print("sending gps data");
	  String data;
	  data = "{\"lat\":" + String(latitude, 3) + ",\"lng\":" + String(longitude, 3) + ",\"alt\":" + String(altitude, 3) + ",\"time\":" + String((int)timestamp) + "}";
    Device.Send(data, gpsId);
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

bool readCoordinates()
{
  bool foundGPGGA = false;                          // Sensor can return multiple types of data, need to capture lines that start with $GPGGA
    if (SoftSerial.available())                     // If date is coming from software serial port ==> data is coming from SoftSerial shield
    {
        while(SoftSerial.available())               // Reading data into char array
        {
            buffer[count++]=SoftSerial.read();      // Writing data into array
            if(count == 64)break;
        }
        //use the following line to see the raw data coming from the gps
        //Serial.println((char*)buffer);
        foundGPGGA = count > 60 && ExtractValues();  // If we have less then 60 characters, then we have bogus input, so don't try to parse it or process the values
        clearBufferArray();                          // Call clearBufferArray function to clear the stored data from the array
        count = 0;                                   // Set counter of while loop to zero
    }
    return foundGPGGA;
}

bool ExtractValues()
{
  unsigned char start = count;
  while(buffer[start] != '$')        // Find the start of the GPS data -> multiple $GPGGA can appear in 1 line, if so, need to take the last one.
  {
    if(start == 0) break;            // It's an unsigned char, so we can't check on <= 0
    start--;
  }
  start++;                           // Remove the '$', don't need to compare with that.
  if(start + 4 < 64 && buffer[start] == 'G' && buffer[start+1] == 'P' && buffer[start+2] == 'G' && buffer[start+3] == 'G' && buffer[start+4] == 'A')      //we found the correct line, so extract the values.
  {
    start+=6;
    timestamp = ExtractValue(start);
    latitude = ConvertDegrees(ExtractValue(start) / 100);
    start = Skip(start);    
    longitude = ConvertDegrees(ExtractValue(start)  / 100);
    start = Skip(start);
    start = Skip(start);
    start = Skip(start);
    start = Skip(start);
    altitude = ExtractValue(start);
    return true;
  }
  else
    return false;
}

float ExtractValue(unsigned char& start)
{
  unsigned char end = start + 1;
  while(end < count && buffer[end] != ',')      // Find the start of the GPS data -> multiple $GPGGA can appear in 1 line, if so, need to take the last one.
    end++;
  buffer[end] = 0;                              // End the string, so we can create a string object from the sub string -> easy to convert to float.
  float result = 0.0;
  if(end != start + 1)                          // If we only found a ',', then there is no value.
    result = String((const char*)(buffer + start)).toFloat();
  start = end + 1;
  return result;
}

float ConvertDegrees(float input)
{
  float fractional = input - (int)input;
  return (int)input + (fractional / 60.0) * 100.0;
}

unsigned char Skip(unsigned char start)
{
  unsigned char end = start + 1;
  while(end < count && buffer[end] != ',')      // Find the start of the GPS data -> multiple $GPGGA can appear in 1 line, if so, need to take the last one.
    end++;
  return end+1;
}

void clearBufferArray()                         // Function to clear buffer array
{
  for (int i=0; i<count; i++)
  {
    buffer[i] = NULL;                           // Clear all index of array with command NULL
  }
}

