/*
    Project: Water Spy - Water Consumption Monitoring
    TX LoRa module
    - Acquires the Water sensor hall-effect sensor and transmits the number of
      detected pules using the LoRa modulatio. For more info check the project 
      webpage at marcoroda.com https://marcoroda.com/2020/08/20/WATER-SPY.html
    
    author: Marco Roda, marcoroda.com
    date:   August 2020
*/

#include <SPI.h>  // to comm with the LoRa modules Ra-01
#include <LoRa.h> // LoRa library 

#define ss 15
#define rst 16
#define dio0 2

// Number of LoRa packets transmitted
unsigned long counter = 0;
// Water Sensor Pin
const int waterSensor = 5;
// Stores nbr of pulses detected by the Water Sensor
unsigned long nbrPulses = 0;

// ISR routine for the water sensor output
ICACHE_RAM_ATTR void detectsMovement() {
  nbrPulses++;
  Serial.println("Number of Pulses detected = ");
  Serial.print(nbrPulses);
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  while (!Serial);
  Serial.println("LoRa Sender");
  // LoRa Setup
  LoRa.setPins(ss, rst, dio0);
    if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    delay(100);
    while (1);
  }
  
  // Water Sensor interrupt config : INPUT_PULLUP
  pinMode(waterSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(waterSensor), detectsMovement, RISING);
}
 
void loop() {
  Serial.print("Sending packet: ");
  Serial.println(counter);
  
  // send nbr of detected pulses
  LoRa.beginPacket();
  LoRa.println(nbrPulses);
  LoRa.endPacket();
 
  counter++;
  // Sends out a new packet every 3 seconds
  delay(3000);
}
