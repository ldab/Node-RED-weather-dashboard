#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "SparkFunHTU21D.h"
#include <PubSubClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Ticker.h>

#define WIFI_AP "XXXXXXXXXXXXXXX"
#define WIFI_PASSWORD "XXXXXXXXXXXXXX"

HTU21D myHumidity;

const char* mqtt_server = "192.168.178.21";  //Your MQTT IP address
WiFiClient wifiClient;
PubSubClient client(wifiClient);

float humd = 0;
float temp = 0;
void ReadSensors();

Ticker ReadSensor(ReadSensors, 20000);

void ReadSensors(){
  client.connect("ESP");
  humd = myHumidity.readHumidity();
  temp = myHumidity.readTemperature();
  Serial.print(humd);
  Serial.println("%");
  Serial.print(temp);
  Serial.println("degC");
  String tempString = String(temp);
  String humdString = String(humd);
  char tempChar[100];
  tempString.toCharArray( tempChar, 100 );
  char humChar[100];
  humdString.toCharArray( humChar, 100 );
  client.publish( "home/humi",humChar);
  client.publish( "home/temperature",tempChar);
}

void callback(char* topic, byte* payload, unsigned int length) {   // handle message arrived
  String message = String();
  for (unsigned int i = 0; i < length; i++) {
    char input_char = (char)payload[i];
    message += input_char;
  }

  if (message == "ota") { // When we receive ota notification from MQTT, start the update
    ESPhttpUpdate.update("http://whatever.domain/tester.bin");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    if ( WiFi.status() != WL_CONNECTED) {
      WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    // Attempt to connect (clientId, username, password)
    if ( client.connect("ESP8266Device", "USERNAME", "PASSWORD") ) {    //MQTT USERNAME AND PASSWORD
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}

void setup() {
    Serial.begin(19200);
    ReadSensor.start();

    myHumidity.begin();

    WiFi.mode(WIFI_OFF);
    delay(500);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    WiFi.printDiag(Serial); //Wi-Fi diagnostic information
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }

    //pinMode(2, OUTPUT);
    //digitalWrite(2, HIGH); //LED OFF

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA End");
    Serial.println("Rebooting...");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
  if ( !client.connected() ) {
  reconnect();
}
  ReadSensor.update();
  client.loop();
}
