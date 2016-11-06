/*

 Code based on https://github.com/DennisSc/easyIoT-ESPduino/blob/master/sketches/ds18b20.ino
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <stdio.h>

#include "config.h"

#define REPORT_INTERVAL 60 // in sec
#define ONE_WIRE_BUS 2  // DS18B20 pin
#define DOORSWITCH 0

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

float oldTemp;
int buttonState;
int doorOpen;

void setup(void)
{
  Serial.begin(9600);

  WiFi.begin(WIFI_SSID, WIFI_PW);
  
  Serial.print("Wifi connecting.");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  Serial.flush();
  oldTemp = -1;
  buttonState = 0;
  doorOpen = 0; 
}


void loop(void)
{ 
  while (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT Broker...");
    // (clientID, username, password, willTopic, willQoS, willRetain, willMessage)
    if (mqttClient.connect("vacuum1", MQTT_USER, MQTT_PASS, "mumalab/fridge/temp/status", 1, true, "0")) {
      Serial.println("MQTT connection successfull.");
      mqttClient.publish("mumalab/fridge/temp/status","1");
      
    } else {
      Serial.println("MQTT connection failed.");
    }
    
    delay(500);
  }

  while (mqttClient.connected()) {
     float temp;
      do {
        DS18B20.requestTemperatures(); 
        temp = DS18B20.getTempCByIndex(0);
        Serial.print("Temperature: ");
        Serial.println(temp);
      } while (temp == 85.0 || temp == (-127.0));
      
      if (temp != oldTemp)
      {
        char buf[10];
        char str_temp[6];
        
        /* 4 is mininum width, 2 is precision; float value is copied onto str_temp*/
        dtostrf(temp, 4, 2, str_temp);
        sprintf(buf, "%s", str_temp);
        mqttClient.publish("mumalab/fridge/temp/value", buf );
        oldTemp = temp;
      }
                
      int cnt = REPORT_INTERVAL;
      
      while(cnt--) {
        delay(500);
        buttonState = digitalRead(DOORSWITCH);
        if (doorOpen != buttonState) {
          Serial.print("Door: ");
          if (buttonState == HIGH) {
            mqttClient.publish("mumalab/fridge/door", "open" );    
            Serial.println("open");      
          } else {
            mqttClient.publish("mumalab/fridge/door", "closed" ); 
            Serial.println("closed"); 
          }
        doorOpen = buttonState; 
        }
        
        mqttClient.loop();
      }
        
  }
  mqttClient.loop();
  delay(1000);
        
} 




