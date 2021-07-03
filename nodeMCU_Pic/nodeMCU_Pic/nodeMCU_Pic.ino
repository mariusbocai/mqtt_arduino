/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <stdlib.h>

// Update these with values suitable for your network.
//const char* ssid = "Tenda_828F60";
//const char* password = "headtable315";

//const char* ssid = "GardeNet";
//const char* password = "SolariileMaAn";
const char* ssid = "GardeNet1";
const char* password = "SolariileMaAn1";
//const char* ssid = "Tenda_2EC6E0";
//const char* password = "gamechair955";
const char* mqtt_server = "192.168.0.79";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long stopTime = 0;
unsigned long onTime = 0;
unsigned char pumpState;
int pumpPin = 4;
int pumpPin1 = 0;
const int analogInPin = A0;

unsigned short Pic_getCurrent(void)
{
  unsigned short sensorValue, maxValue;
  int i;

  sensorValue = maxValue = 0;
  for (i=0; i<100;i++)
  {
    sensorValue = analogRead(analogInPin);
    if (sensorValue>maxValue)
    {
      maxValue = sensorValue;
    }
    delay(1);
  }
  return maxValue;
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

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
  /*Check if there is a pump request*/
  if (strcmp(topic,"/Pump/Control")==0) {
    /*Yep, it's a request*/
    if ((char)payload[0] == '1')
    {
      /*Start countdown timer*/
      onTime = millis();
      stopTime = onTime + 1800000;
      if(stopTime < onTime)
      {
        /*Counter Overflow, stop pump as countdown timer is broken*/
        digitalWrite(pumpPin, HIGH);
        digitalWrite(pumpPin1, HIGH);
        client.publish("/Pump/Status", "0Pompa Oprita");
        Serial.println("0Pompa Oprita");
        onTime = 0;
        stopTime = 0;
        pumpState = 0;
      }
      else
      {
        /*Switch pump on*/
        digitalWrite(pumpPin, LOW);
        digitalWrite(pumpPin1, LOW);
        client.publish("/Pump/Status", "1Pompa Pornita");
        Serial.println("1Pompa Pornita");
        pumpState = 1;
      }
    }
    else
    {
      digitalWrite(pumpPin, HIGH);
      digitalWrite(pumpPin1, HIGH);
      client.publish("/Pump/Status", "0Pompa Oprita");
      Serial.println("0Pompa Oprita");
      onTime = 0;
      stopTime = 0;
      pumpState = 0;
    }
  }
  else if(strcmp(topic,"/Pump/Query")==0)
  {
    /*Just a status update requested*/
    if (pumpState == 0)
    {
      client.publish("/Pump/Status", "0Pompa Oprita");
      Serial.println("0Pompa Oprita");
    }
    else
    {
      client.publish("/Pump/Status", "1Pompa Pornita");
      Serial.println("1Pompa Pornita");
    }
  }
  else if(strcmp(topic,"/Pump/QueryC")==0)
  {
    unsigned short Current_n;
    char cstr[4];
    Current_n = Pic_getCurrent();
    itoa(Current_n, cstr, 10);
    client.publish("/Pump/StatusC", cstr);
  }
}

void reconnect() {
  /*For safety reasons, switch pump off in case there is no MQTT connection available*/
  digitalWrite(pumpPin, HIGH); //set pump OFF
  digitalWrite(pumpPin1, HIGH);
  pumpState = 0;
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/Pump/Status", "Modul Control pompa pornit");
      // ... and resubscribe
      client.subscribe("/Pump/Control");
      client.subscribe("/Pump/Query");
      client.subscribe("/Pump/QueryC");
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
  pinMode(pumpPin, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(pumpPin1, OUTPUT);
  digitalWrite(pumpPin, HIGH); //set pump OFF
  digitalWrite(pumpPin1, HIGH);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  onTime = 0;
  stopTime = 0;
  pumpState = 0;
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if (WiFi.status() != WL_CONNECTED)
  {
    /*If there is no WiFi available, switch off the pump*/
    digitalWrite(pumpPin, HIGH); //set pump OFF
    digitalWrite(pumpPin1, HIGH);
    onTime = 0;
    stopTime = 0;
    pumpState = 0;
    setup_wifi();
  }
  if(pumpState == 1)
  {
    unsigned long now = millis();
    if (now > stopTime)
    {
      /*Pump has been running for 30 minutes, it's time to stop now*/
      digitalWrite(pumpPin, HIGH); //set pump OFF
      digitalWrite(pumpPin1, HIGH);
      pumpState = 0;
      onTime = 0;
      stopTime = 0;
    }
  }
  /*take a break for 1 second*/
  //delay(1000);
}