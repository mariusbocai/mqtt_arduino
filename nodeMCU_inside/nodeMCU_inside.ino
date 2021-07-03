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
#include <DHTesp.h>

/** Initialize DHT sensor */
DHTesp dht;
/** Task handle for the light value read task */
//TaskHandle_t tempTaskHandle = NULL;
/** Pin number for DHT11 data pin */
int dhtPin = 2;

//Enable this if the board uses a relay to power the humudity sensor only when the measurement is made
#define Use_Power_relay 1

#if Use_Power_relay
#define Sleep_time 600000
#else
#define Sleep_time 2000
#endif
// Update these with values suitable for your network.

//const char* ssid = "HorsDePrix";
//const char* password = "atefisawesome";
//const char* mqtt_server = "192.168.2.10";

//const char* ssid = "Tenda_20AE40";
//const char* password = "abovehigh379";

//const char* ssid = "Tenda_828F60";
//const char* password = "headtable315";

const char* ssid = "Tenda_2EC6E0";
const char* password = "gamechair955";

const char* mqtt_server = "192.168.1.79";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
const int sensor_pin = A0;  /* Connect Soil moisture analog sensor pin to A0 of NodeMCU */
const int relay_pin = 0;

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
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
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
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      //client.subscribe("inTopic");
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
  pinMode(relay_pin, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  // Initialize temperature sensor
  Serial.println("Setup sensor");
  dht.setup(dhtPin, DHTesp::DHT11);
  Serial.println("Setup done");
  lastMsg = 0;
  //client.setCallback(callback);
}

void loop() {
  uint8 moisture_percentage, temp_air, humi_air;
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (WiFi.status() != WL_CONNECTED)
  {
    setup_wifi();
  }

  unsigned long now = millis();
  if ((now - lastMsg) > Sleep_time) {
    lastMsg = now;
    ++value;
    /*Read temperature and Humidity*/
    // Reading temperature and humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    Serial.print("Reading sensor");
    TempAndHumidity lastValues = dht.getTempAndHumidity();
    temp_air = (uint8)lastValues.temperature;
    humi_air = (uint8)lastValues.humidity;
    Serial.print("Temperature: ");
    Serial.println(temp_air);
    Serial.print("Humidity: ");
    Serial.println(humi_air);
    /*Calculate humidity Value based on analog input*/
    #if Use_Power_relay
    //digitalWrite(relay_pin, HIGH);
    //delay(5000);
    #endif
   // moisture_percentage = (uint8)( 100.00 - ( (analogRead(sensor_pin)/1023.00) * 100.00 ) );
    #if Use_Power_relay
    //digitalWrite(relay_pin, LOW);
    #endif
   
    Serial.print("Publish message: ");
  if ((humi_air==0) || (humi_air==255) || (temp_air == 255)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
       client.publish("/Camera/Temperatura_aer", String(temp_air).c_str());
       client.publish("/Camera/Umiditate_aer", String(humi_air).c_str());
    }
  }
}
