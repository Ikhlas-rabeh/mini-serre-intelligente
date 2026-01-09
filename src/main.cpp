#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTTYPE    DHT22
#include <WiFi.h>
#include <AsyncMqttClient.h>
AsyncMqttClient mqttClient;

const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;

const char* ssid = "Wokwi-GUEST";  
const char* password = "";          

const int lampe = 19;
const int eau = 16;
const int potentio = 35;
const int dhtPin = 17;
const int LDR = 32;

DHT_Unified dht(dhtPin, DHTTYPE);

void onMqttConnect(bool sessionPresent) {
    Serial.println("MQTT connection established!");
    Serial.println("ESP32 is online and ready to publish data.");
    mqttClient.publish("mini-serre/status", 1, true, "online");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  //wifi setup
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
  //mqtt setup
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setWill("mini-serre/status", 1, true, "offline"); //last will & testament
  mqttClient.onConnect(onMqttConnect);
  mqttClient.connect();
  //pin config
  pinMode(lampe, OUTPUT);
  pinMode(eau, OUTPUT);
  pinMode(potentio, INPUT);
  pinMode(LDR, INPUT);
  dht.begin();
}

void loop() {
static unsigned long lastPublish = 0;
const unsigned long interval = 5000;
//loop for reading and publishing data
if (millis() - lastPublish >= interval) { 
  lastPublish = millis();
  //reading and converting potentio values
  int hum_sol = analogRead(potentio); 
  int hum_percent = map(hum_sol, 0, 4095, 0, 100);
  //reading luminosity value
  int luminosite = analogRead(LDR);
  //get temp and hum and verify if there is an error
  sensors_event_t tempEvent, humEvent;
  dht.temperature().getEvent(&tempEvent);
  dht.humidity().getEvent(&humEvent);
 if (isnan(tempEvent.temperature) || isnan(humEvent.relative_humidity)) {
    Serial.println("DHT22 error reading data.");
    return;
 }
 if (mqttClient.connected()) {
  //print to console
  Serial.print("Soil Humidity Percentage: " + String(hum_percent) + "%");
  Serial.print("Temperature: " + String(tempEvent.temperature, 2) + "°C\t");
  Serial.print("Humidity: " + String(humEvent.relative_humidity, 1) + "%\t");
  Serial.println("Luminosity: " + String(luminosite) + "\t");
  //publish to broker
  mqttClient.publish("mini-serre/soil", 0, false, String(hum_percent).c_str());
  mqttClient.publish("mini-serre/temperature", 0, false, String(tempEvent.temperature).c_str());
  mqttClient.publish("mini-serre/humidity", 0, false, String(humEvent.relative_humidity).c_str());
  mqttClient.publish("mini-serre/light", 0, false, String(luminosite).c_str());
  //watering manipulation
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
  //lamp manipulation
  if (luminosite < 2000) {
    digitalWrite(lampe, HIGH);
    Serial.println("It's dark, turning on the lamp.");
  } else {
    digitalWrite(lampe, LOW);
    Serial.println("Light level is sufficient.");
  }
  //temp and hum alerts
  if (tempEvent.temperature > 40) {
    Serial.println("Warning: High temperature detected!");
    mqttClient.publish("mini-serre/alert", 1, true, "Warning! High temperature detected!");
  }
  if (humEvent.relative_humidity > 80) {
    Serial.println("Warning: High humidity detected!");
    mqttClient.publish("mini-serre/alert", 1, true, "Warning! High Humidity level detected!");
  }
}
}
}