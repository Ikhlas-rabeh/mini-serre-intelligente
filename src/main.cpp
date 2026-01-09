#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTTYPE    DHT22
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char* ssid = "Wokwi-GUEST";  
const char* password = "";          

const int lampe = 19;
const int eau = 16;
const int potentio = 35;
const int dhtPin = 17;
const int LDR = 32;

DHT_Unified dht(17, DHT22);

WiFiClientSecure espClient;        
PubSubClient client(espClient); 

const char* mqtt_server = "601c0121dc1841cf81e5c9dfb83600d9.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;

const char* mqtt_user = "ikhlas_iot";
const char* mqtt_pass = "Robotics_esp32";

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "ESP32_" + String(random(0xffff), HEX); // unique ID
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe("mini-serre/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  espClient.setInsecure(); // Accept HiveMQ Cloud TLS certificate
  client.setServer(mqtt_server, mqtt_port);

  pinMode(lampe, OUTPUT);
  pinMode(eau, OUTPUT);

  pinMode(potentio, INPUT);
  pinMode(LDR, INPUT);
  dht.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // maintain MQTT connection

  delay(5000);

  int hum_sol = analogRead(potentio); 
  int hum_percent = map(hum_sol, 0, 4095, 0, 100);
  Serial.print("Soil Humidity Percentage: " + String(hum_percent) + "%");

  sensors_event_t tempEvent, humEvent;
  dht.temperature().getEvent(&tempEvent);
  dht.humidity().getEvent(&humEvent);
  Serial.print("Temperature: " + String(tempEvent.temperature, 2) + "°C\t");
  Serial.print("Humidity: " + String(humEvent.relative_humidity, 1) + "%\t");

  int luminosite = analogRead(LDR);
  Serial.println("Luminosity: " + String(luminosite) + "\t");

  // Publishing corrected topics (no spaces)
  client.publish("mini-serre/soil", String(hum_percent).c_str());
  client.publish("mini-serre/temperature", String(tempEvent.temperature, 2).c_str());
  client.publish("mini-serre/humidity", String(humEvent.relative_humidity, 1).c_str());
  client.publish("mini-serre/light", String(luminosite).c_str());

  if (hum_percent < 40) {
    digitalWrite(eau, HIGH);
    Serial.println("Watering the plant...");
  } else if (hum_percent > 70) {
    digitalWrite(eau, LOW);
    Serial.println("Soil is too wet, stopping watering.");
  } else {
    digitalWrite(eau, LOW);
    Serial.println("Soil moisture is sufficient.");
  }

  if (luminosite < 2000) {
    digitalWrite(lampe, HIGH);
    Serial.println("It's dark, turning on the lamp.");
  } else {
    digitalWrite(lampe, LOW);
    Serial.println("Light level is sufficient.");
  }

  if (tempEvent.temperature > 40) {
    Serial.println("Warning: High temperature detected!");
  }
  if (humEvent.relative_humidity > 80) {
    Serial.println("Warning: High humidity detected!");
  }
}
