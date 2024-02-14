#include "ledhelper.h"

#define HOME_ARMED_PIN A1
#define GARAGE_ARMED_PIN A2
#define GARAGE_DOOR_OPEN_PIN A3

#define KEYSWITCH_ZONE_PIN 1

#define GARAGE_ARMING_DELAY 120000
#define GARAGE_ARMING_TIMEOUT 10000

#define STATE_HOME_ARMED_PIN 6
#define STATE_GARAGE_ARMED_PIN 7
#define DOOR_OPENED_RECENTLY_PIN 8
#define ONLINE_INDICATOR_PIN 9

unsigned long lastDoorOpenTime;
unsigned long garageDoorArmingTime;
unsigned long garageDoorDisarmingTime;

unsigned long homeArmingTime;
bool wasHomeArmedBefore;

LED homeArmedPin(STATE_HOME_ARMED_PIN, 500, 500);
LED garageArmedPin(STATE_GARAGE_ARMED_PIN, 500, 500);
LED doorOpenedRecentlyPin(DOOR_OPENED_RECENTLY_PIN, 500, 500);
//LED systemOnlinePin(ONLINE_INDICATOR_PIN, 2500, 2500);

void setup() {
  lastDoorOpenTime = millis() - 600000;
  garageDoorArmingTime = millis() - 600000;
  wasHomeArmedBefore = false;

  pinMode(HOME_ARMED_PIN, INPUT_PULLUP);
  pinMode(GARAGE_ARMED_PIN, INPUT_PULLUP);
  pinMode(GARAGE_DOOR_OPEN_PIN, INPUT_PULLUP);
}

void actuateKeySwitch() {
  Serial.println("Actuating the keyswitch");
  digitalWrite(KEYSWITCH_ZONE_PIN, 1);
  delay(1000);
  digitalWrite(KEYSWITCH_ZONE_PIN, 0);
  Serial.println("Keyswitch actuated");
}

void loop() {
  // Look around for the state changes
  bool isDoorOpen = analogRead(GARAGE_DOOR_OPEN_PIN) < 500;
  bool isHomeArmed = analogRead(HOME_ARMED_PIN) < 500;
  bool isGarageArmed = analogRead(GARAGE_ARMED_PIN) < 500;

  if (isHomeArmed && !wasHomeArmedBefore) {
    wasHomeArmedBefore = true;
    homeArmingTime = millis();
  }
  if (!isHomeArmed) {
    wasHomeArmedBefore = false;
  }

  if (isDoorOpen) {
    lastDoorOpenTime = millis();
  }
  bool doorWasOpenRecently = (millis() - lastDoorOpenTime) <= GARAGE_ARMING_DELAY;
  bool homeWasArmedRecently = (millis() - homeArmingTime) <= GARAGE_ARMING_DELAY;  

  // We arm the garage if the door has been closed for a while, and the home is armed,
  // and the garage is NOT armed
  if (!doorWasOpenRecently && isHomeArmed && !homeWasArmedRecently && !isGarageArmed) {
    if (millis() - garageDoorArmingTime > GARAGE_ARMING_TIMEOUT) {
      Serial.println("Garage is not armed, and the home is armed. Arming the garage partition.");
      actuateKeySwitch();
      garageDoorArmingTime = millis();
      // Allow the garage to be disarmed immediately
      garageDoorDisarmingTime = millis() - GARAGE_ARMING_TIMEOUT;
    }
  }

  if (isGarageArmed && (doorWasOpenRecently || !isHomeArmed)) {
    // Make sure we don't spam the keyswitch
    if (millis() - garageDoorDisarmingTime > GARAGE_ARMING_TIMEOUT) {
      Serial.println("Garage is armed. Disarming the garage partition.");
      actuateKeySwitch();
      garageDoorDisarmingTime = millis();
    }
  }

  // Update LEDs
  if (isHomeArmed) {
    homeArmedPin.setLedState(homeWasArmedRecently ? LED::BLINKING : LED::ON);
  } else {
    homeArmedPin.setLedState(LED::OFF);
  }
  garageArmedPin.setLedState(isGarageArmed ? LED::ON : LED::OFF);
  doorOpenedRecentlyPin.setLedState(doorWasOpenRecently ? LED::BLINKING : LED::OFF);
  //systemOnlinePin.setLedState(LED::BLINKING);

  homeArmedPin.update();
  garageArmedPin.update();
  doorOpenedRecentlyPin.update();
  //systemOnlinePin.update();
  delay(100);
}
