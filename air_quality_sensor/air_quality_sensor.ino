/*
   Copyright 2015-2016 AllThingsTalk

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

This sketch is an example sketch to deploy the Grove Air quality sensor v1.3 (101020078) to the
AllThingsTalk IoT developer cloud.

*/


#include <Ethernet2.h>
#include <EthernetClient.h>

#include <PubSubClient.h>
#include <ATT_IOT.h>                            //AllThingsTalk IoT library
#include <SPI.h>    
#include "keys.h"              // Keep all your personal account information in a seperate file
#include "AirQuality2.h"

ATTDevice Device(DEVICEID, CLIENTID, CLIENTKEY);
#define httpServer "api.AllThingsTalk.io"                  // HTTP API Server host                  
#define mqttServer httpServer                				// MQTT Server Address 

// Define PIN numbers for assets
// For digital and analog sensors, we recommend to use the physical pin id as the asset id
// For other sensors (I2C and UART), you can select any unique number as the asset id
#define airId 2                // Analog sensor is connected to pin A2 on grove shield
#define textId 3               // Extra asset to display labeled values
AirQuality2 airqualitysensor;

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
EthernetClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);

void setup()
{
  pinMode(airId, INPUT);                               // Initialize the digital pin as an input.
  Serial.begin(57600);                                 // Init serial link for debugging
  while(!Serial) ;                                     // This line makes sure you see all output on the monitor. REMOVE THIS LINE if you want your IoT board to run without monitor !
  Serial.println("Starting sketch");
  Serial1.begin(115200);                               // Init serial link for WiFi module
  while(!Serial1);

  byte mac[] = {  0x90, 0xA2, 0xDA, 0x0D, 0x8D, 0x3D };         // Adapt to your Arduino MAC Address 
  if (Ethernet.begin(mac) == 0)                                 // Initialize the Ethernet connection:
  { 
    Serial.println(F("DHCP failed,end"));
    while(true);                                                //we failed to connect, halt execution here. 
  }
  delay(1000);                                                  //give the Ethernet shield a second to initialize:
  while(!Device.Connect(&ethClient, httpServer))                // connect the device with the IOT platform.
    Serial.println("retrying");

  Device.AddAsset(airId, "Air quality sensor v1.3", "air", false, "{\"type\": \"integer\",\"minimum\":0,\"maximum\":1023}");   // Create the sensor asset for your device
  Device.AddAsset(textId, "Air quality", "label", false, "string");

  while(!Device.Subscribe(pubSub))                              // make certain that we can receive message from the iot platform (activate mqtt)
    Serial.println("retrying");

  airqualitysensor.init(airId);
  Serial.println("Air quality sensor v1.3 is ready for use!");
}

int sensorVal = -1;                                    // Set initial value to -1
int evaluateVal = -1;

void loop() 
{
  int sensorRead = airqualitysensor.getRawData();
  Serial.println(String(sensorRead));
  
  if (sensorVal != sensorRead ) 
  {
    Serial.print("Air quality: ");
    Serial.print(sensorVal);
    Serial.println("(raw)");
    sensorVal = sensorRead;
    
    Device.Send(String(sensorVal), airId);
  }
   
  sensorRead = airqualitysensor.evaluate();
  if (evaluateVal != sensorRead ) 
  {
    String text;
    evaluateVal = sensorRead;
    if(evaluateVal == 0)
      text = "Good air quality";
    else if(evaluateVal == 1)
      text = "Low pollution";
    else if(evaluateVal == 2)
      text = "High pollution";
    if(evaluateVal == 3)
      text = "Very high pollution";
    Serial.println("0|"+text);
    
    Device.Send(text, textId);
  }
  
  Device.Process();
  delay(2000);
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


