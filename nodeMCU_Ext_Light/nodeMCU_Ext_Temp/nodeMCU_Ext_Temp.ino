#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <stdlib.h>


#define LightOnDelayTime 600000

const char* ssid = "GardeNet2";
const char* password = "WazzupDoc";
#error "No Pass set"
const char* mqtt_server = "192.168.0.79";

WiFiClient espClient;
PubSubClient client(espClient);

/** Initialize DHT sensor */
#define DHTPIN 14     // Digital pin connected to the DHT sensor 
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

DHT_Unified dht(DHTPIN, DHTTYPE);
sensor_t sensor;
sensors_event_t event;

int lightPin = 4;
int lightPinRed = 5;
unsigned long lightSwitchOnTime = 0;
unsigned char lightState;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(BUILTIN_LED, LOW);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  /*Check if there is an Ext Light request*/
  if (strcmp(topic,"/Home/ExtLight/Query")==0) {
    /*Yep, it's a request*/
    if ((char)payload[0] == '1')
    {
      char result[8];
      /*Get temperature*/
      dht.temperature().getEvent(&event);
      if(isnan(event.temperature))
      {
        Serial.println(F("Error reading temperature!"));
      }
      else
      {
        dtostrf(event.temperature, 5, 1, result);
        client.publish("/Home/Temperature/ExtTemp", result);
      }
      dht.humidity().getEvent(&event);
      if(isnan(event.relative_humidity))
      {
        Serial.println(F("Error reading humidity!"));
      }
      else
      {
        dtostrf(event.relative_humidity, 4, 1, result);
        client.publish("/Home/Humidity/ExtHumi", result);
      }
      /* get ext light status*/
      if(lightState == 1) {
         client.publish("/Home/ExtLight/Status", "1Bec Aprins");
      }
      else {
          client.publish("/Home/ExtLight/Status", "0Bec Stins");
      }
    }
  }
  else if (strcmp(topic,"/Home/ExtLight/Control")==0) {
      if ((char)payload[0] == '1') {
        /*Switch Light on*/
        digitalWrite(lightPin, LOW);
        digitalWrite(lightPinRed, LOW);
        client.publish("/Home/ExtLight/Status", "1Bec Aprins");
        Serial.println("Bec Ext aprins");
        lightState = 1;
     }
     else if ((char)payload[0] == '0') {
        /*Switch Light off*/
        digitalWrite(lightPin, HIGH);
        digitalWrite(lightPinRed, HIGH);
        client.publish("/Home/ExtLight/Status", "0Bec Stins");
        Serial.println("Bec Ext stins");
        lightState = 0;
     }
}
}

void reconnect() {

  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected and subscribed");
      client.subscribe("/Home/ExtLight/Query");
      client.subscribe("/Home/ExtLight/Status");
      client.subscribe("/Home/ExtLight/Control");
      client.subscribe("/Home/Temperature/ExtTemp");
      client.subscribe("/Home/Humidity/ExtHumi");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(lightPin, OUTPUT);
  pinMode(lightPinRed, OUTPUT);
  dht.begin();
  lightState = 1;
  //Start with the Light switched on
  digitalWrite(lightPin, LOW);
  digitalWrite(lightPinRed, LOW);
  lightSwitchOnTime = millis();
  Serial.begin(9600); // Starts the serial communication
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  
  if(WiFi.isConnected() == false)
  {
     setup_wifi();
  }
  else
  {
     if (!client.connected()) {
        reconnect();
     }
     client.loop();
     if((lightState == 1)&&(millis() > ((unsigned long)(lightSwitchOnTime + LightOnDelayTime))))
     {
      /*After 10 minutes, shut down the light*/
      lightState = 0;
      digitalWrite(lightPin, HIGH);
      digitalWrite(lightPinRed, HIGH);
     }
  }
}
