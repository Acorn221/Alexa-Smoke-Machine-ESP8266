#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "fauxmoESP.h"

#define WIFI_SSID "SSID"
#define WIFI_PASS "PASSWORD"

fauxmoESP fauxmo;

// -----------------------------------------------------------------------------

#define SERIAL_BAUDRATE     115200

#define relayPin            0

#define name             "Smoke"

int remainingTime = 0;
bool onState = false;
static unsigned long relay = 0;
int val = 0;

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

void wifiSetup() {

  // Set WIFI module to STA mode
  WiFi.mode(WIFI_STA);

  // Connect
  Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Wait
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Connected!
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

}

int addT(int x) {
  x = map(x, 0, 255, 0, 100);
  if (x >= 95) { // This would mean that someone has just turned it on, so we don't want to blast them with all the smoke
    return 15000;
  } else if (x < 95 && x > 50) { // this will blast them with smoke, just to make it easier
    return 75000;
  } else if (x <= 50 && x > 20) { // this will do a moderate blast
    return 7500;
  } else if (x <= 20 ) { // this is a quick blast
    return 2500;
  }
}

void setup() {

  // Init serial port and clean garbage
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println();
  Serial.println();

  // LEDs
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);

  // Wifi
  wifiSetup();
  fauxmo.setPort(80); // This is required for gen3 devices
  fauxmo.enable(true);
  fauxmo.addDevice(name); // adds smoke as a device

  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    remainingTime = addT(value);
    digitalWrite(relayPin, state ? LOW : HIGH);
    relay = millis();
    onState = state;
    val = value;
    if (state) {
      Serial.printf("\nRemaining Time:%d, State: %d", remainingTime, state);
    } else {
      Serial.print("\nRequested Off");
    }

  });

}


void loop() {
  fauxmo.handle();
  // interupts are not needed, they could be used but that would make this code less adaptable
  if (remainingTime > 0 && onState) {
    if (millis() - relay >= remainingTime) {
      digitalWrite(relayPin, HIGH);
      //Serial.printf("\nOff Relay: %d, Mills-relay=%d, RemainingTime: %d", relay, millis() - relay, remainingTime);
      Serial.printf("\nOff");
      fauxmo.setState(name, false, val);
      remainingTime = 0;
    }
  }

}
