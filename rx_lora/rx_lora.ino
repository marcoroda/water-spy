/*
    Project: Water Spy - Water Consumption Monitoring
    RX LoRa module
    - Receives the LoRa payload sent out by the TX module.
    - Publishes BME280 and water sensor data to a Node-Red dashboard application 
    through MQTT. For more info check the project webpage at marcoroda.com 
    https://marcoroda.com/2020/08/20/WATER-SPY.html
    
    author: Marco Roda, marcoroda.com
    date:   August 2020
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <SPI.h>
#include <LoRa.h>

// Replace with your WiFi network credentials
const char* WIFI_SSID     = "";
const char* WIFI_PASSWORD = "";

// Raspberri Pi Mosquitto MQTT Broker
#define MQTT_HOST IPAddress(0, 0, 0, 0) // replace with your Broker's IP address
#define MQTT_PORT 1883

// MQTT Topics
#define MQTT_PUB_TEMP  "esp/bme280/temperature"
#define MQTT_PUB_HUM   "esp/bme280/humidity"
#define MQTT_PUB_PRES  "esp/bme280/pressure"
#define MQTT_PUB_WATER "esp/bme280/water"

// Got from water sensor calibration
#define PULSES_PER_LITRES 320

#define ss 15
#define rst 16
#define dio0 4

// BME280 Instance for I2C interface
Adafruit_BME280 bme;

// Variables to hold BME280sensor readings
float temp;
float hum;
float pres;

// Valriables to hold nbr of pulses received (LoRa payload)
String LoRaData;
unsigned long pulsesRX   = 0;
unsigned long pulsesMQTT = 0;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Water Spy - RX module Receiver");

  //setup LoRa RX module
  LoRa.setPins(ss, rst, dio0);
  // 433 MHz is used for TX-ing
  while (!LoRa.begin(433E6)) {
    Serial.println(".");
    delay(500);
  }
  
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");

  // Initialize BME280 sensor 
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  
  // Connect to WiFi local network
  connectToWifi();
}

void loop() {
  unsigned long currentMillis = millis();
  // Every X number of seconds (interval = 10 seconds) 
  // it publishes a new MQTT message
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;

    // New BME280 sensor readings
    temp = bme.readTemperature();
    hum = bme.readHumidity();
    pres = bme.readPressure()/100.0F;
    
    // Publish an MQTT message on topic esp/bme280/temperature
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temp).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_TEMP, packetIdPub1);
    Serial.printf("Message: %.2f \n", temp);

    // Publish an MQTT message on topic esp/bme280/humidity
    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(hum).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_HUM, packetIdPub2);
    Serial.printf("Message: %.2f \n", hum);

    // Publish an MQTT message on topic esp/bme280/pressure
    uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_PRES, 1, true, String(pres).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_PRES, packetIdPub3);
    Serial.printf("Message: %.3f \n", pres);

    // Publish an MQTT message on topic esp/bme280/water
    uint16_t packetIdPub4 = mqttClient.publish(MQTT_PUB_WATER, 1, true, String(pulsesMQTT).c_str());                     
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_WATER, packetIdPub4);
    Serial.printf("Message: %.3u \n", pulsesMQTT);
    
    
  }

  // try to parse LoRa packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet '");
    
    // read packet
    while (LoRa.available()) {
      LoRaData = LoRa.readString();
      Serial.print(LoRaData); 
    }
    
    pulsesRX = LoRaData.toInt();
    if (pulsesRX > 0) {
      pulsesMQTT = pulsesRX / PULSES_PER_LITRES; // calibration
    }

    // print RSSI of received packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
 
}
