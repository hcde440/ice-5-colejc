//ICE#5 - this code sends sensor information as JSON to MQTT, to be collected by another device.

#include <ESP8266WiFi.h>    //Requisite Libraries . . .
#include "Wire.h"           //
#include <PubSubClient.h>   //
#include <ArduinoJson.h>    //
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MPL115A2.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DHTPIN 2 //digital pin connected to the DHT sensor
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);

//start MPL sensor
Adafruit_MPL115A2 mpl115a2;

#define wifi_ssid "University of Washington"   //You have seen this before
#define wifi_password "" //

#define mqtt_server "mediatedspaces.net"  //this is its address, unique to the server
#define mqtt_user "hcdeiot"               //this is its server login, unique to the server
#define mqtt_password "esp8266"           //this is it server password, unique to the server

WiFiClient espClient;             //blah blah blah, espClient
PubSubClient mqtt(espClient);     //blah blah blah, tie PubSub (mqtt) client to WiFi client

char mac[6]; //A MAC address is a 'truly' unique ID for each device, lets use that as our 'truly' unique user ID!!!
char message[201]; //201, as last character in the array is the NULL character, denoting the end of the array
unsigned long currentMillis, timerOne, timerTwo, timerThree; //we are using these to hold the values of our timers

//This method connnects to a wifi network
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");  //get the unique MAC address to use as MQTT client ID, a 'truly' unique ID.
  Serial.println(WiFi.macAddress());  //.macAddress returns a byte array 6 bytes representing the MAC address
}                                     //5C:CF:7F:F0:B0:C1 for example

//Set up the wifi, connect to the server, and initialize sensors
void setup() {
  Serial.begin(115200);
  setup_wifi();
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback); //register the callback function
  timerOne = timerTwo = timerThree = millis();

  dht.begin();
  mpl115a2.begin();
}

//Monitor the connection to MQTT server, if down, reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_user, mqtt_password)) { //<<---using MAC as client ID, always unique!!!
      Serial.println("connected");
      mqtt.subscribe("fromJon/+"); //we are subscribing to 'theTopic' and all subtopics below that topic
    } else {                        //please change 'theTopic' to reflect your topic you are subscribing to
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//Read information from sensors, format it as JSON, and output to MQTT
void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop(); //this keeps the mqtt connection 'active'
  
  //Here we just send a regular c-string which is not formatted JSON, or json-ified.
  if (millis() - timerOne > 10000) {
    //Here we would read a sensor, perhaps, storing the value in a temporary variable

    //STARTING HERE, ADD DHT/MPL SENSING AND SEND TO MQTT
    float presNum = 0, tempNum = 0;    
    presNum = mpl115a2.getPressure();
    tempNum = mpl115a2.getTemperature();
    sensors_event_t event;
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
    }
    else {
      //CREATE THE MESSAGE TO SEND TO MQTT
      //turn numbers into strings
      char temp[10];
      char pres[10];
      char hum[10];
      dtostrf(presNum, 5, 2, pres);
      dtostrf(tempNum, 5, 2, temp);
      dtostrf(event.relative_humidity, 5, 2, hum);

      //send message
      sprintf(message, "{\"temp\":\"%s%\", \"pres\":\"%s%\", \"hum\":\"%s%\"}", temp, pres, hum); // %d is used for an int
      mqtt.publish("fromJon/words", message);
    }
    timerOne = millis();
  }
}//end Loop

//format JSON data and print it to the serial monitor
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic); //'topic' refers to the incoming topic name, the 1st argument of the callback function
  Serial.println("] ");

  DynamicJsonBuffer  jsonBuffer; //blah blah blah a DJB
  JsonObject& root = jsonBuffer.parseObject(payload); //parse it!

  if (!root.success()) { //well?
    Serial.println("parseObject() failed, are you sure this message is JSON formatted.");
    return;
  }
  
  root.printTo(Serial); //print out the parsed message
  Serial.println(); //give us some space on the serial monitor read out
}
