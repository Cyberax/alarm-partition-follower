#include "ledhelper.h"

enum SystmeState {
  IDLE,
  BOTH_ARMED,
  HOME_ARMED,
  GARAGE_ARMING,
  GARAGE_DISARMING,
  EXCEPTION  
};

SystmeState state;
unsigned long lastStateEvent;
unsigned long lastDoorOpenTime;

#define HOME_ARMED_PIN A1
#define GARAGE_ARMED_PIN A2
#define GARAGE_DOOR_OPEN_PIN A3

#define KEYSWITCH_ZONE_PIN 1

#define GARAGE_ARMING_DELAY 120000
#define GARAGE_ARMING_TIMEOUT 10000

#define STATE_IDLE_PIN 6
#define STATE_ARMED_PIN 7
#define STATE_GARAGE_ARMING_DISARMING_PIN 8
#define STATE_EXCEPTION_PIN 9

LED idlePin(STATE_IDLE_PIN, 500, 500);
LED armedPin(STATE_ARMED_PIN, 500, 500);
LED garageArmingDisarmingPin(STATE_GARAGE_ARMING_DISARMING_PIN, 500, 500);
LED exceptionPin(STATE_EXCEPTION_PIN, 500, 500);

void setup() {
  state = IDLE;
  lastDoorOpenTime = lastStateEvent = millis() - 600000;

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

void transition(SystmeState newState) {
  state = newState;
  lastStateEvent = millis();
}

void updateLedState() {
    // Update the LEDs
  idlePin.setLedState(state == IDLE ? LED::ON : LED::OFF);  
  if (state == HOME_ARMED || state == GARAGE_ARMING) {
    armedPin.setLedState(LED::BLINKING);
  } else if (state == BOTH_ARMED || state == GARAGE_DISARMING) {
    armedPin.setLedState(LED::ON);
  } else {
    armedPin.setLedState(LED::OFF);
  }
  garageArmingDisarmingPin.setLedState((state == GARAGE_ARMING || state == GARAGE_DISARMING) ? LED::BLINKING : LED::OFF);
  exceptionPin.setLedState(state == EXCEPTION ? LED::BLINKING : LED::OFF);
  idlePin.update();
  armedPin.update();
  garageArmingDisarmingPin.update();
  exceptionPin.update();
}

void loop() {
  // Look around for the state changes
  bool is_door_open = analogRead(GARAGE_DOOR_OPEN_PIN) < 500;
  bool is_home_armed = analogRead(HOME_ARMED_PIN) < 500;
  bool is_garage_armed = analogRead(GARAGE_ARMED_PIN) < 500;

  if (is_door_open) {
    lastDoorOpenTime = millis();
  }
  bool door_was_open_recently = (millis() - lastDoorOpenTime) <= GARAGE_ARMING_DELAY;

  unsigned long timediff = millis() - lastStateEvent;

  // Alarm state machine
  switch(state) {
    case IDLE:
      if (is_home_armed && is_garage_armed) {
        Serial.println("IDLE: Both partitions are armed, transitioning to BOTH_ARMED");
        transition(BOTH_ARMED);
      } else if (is_home_armed) {
        Serial.println("IDLE: Home partition is armed, transitioning to HOME_ARMED");
        transition(HOME_ARMED);
      } else if (is_garage_armed) {
        // Disarm the garage
        Serial.println("IDLE: Garage partition is armed, disarming");
        actuateKeySwitch();
        transition(GARAGE_DISARMING);
      }
    break;
    case BOTH_ARMED:
      if (is_garage_armed) {
        if (door_was_open_recently) {
          // Disarm the garage
          Serial.println("BOTH_ARMED: Garage door was open recently, disarming the garage partition");
          actuateKeySwitch();
          transition(GARAGE_DISARMING);
        } else if (!is_home_armed) {
          // Disarm the garage
          Serial.println("BOTH_ARMED: Home partition is no longer armed, disarming the garage partition");
          actuateKeySwitch();
          transition(GARAGE_DISARMING);
        }
      } else {
        if (is_home_armed) {
          Serial.println("BOTH_ARMED: Garage partition is no longer armed, but home is armed, transitioning to HOME_ARMED");
          transition(HOME_ARMED);
        } else {
          Serial.println("BOTH_ARMED: No partition is armed, transitioning to IDLE");
          transition(IDLE);
        }
      }
    break;
    case HOME_ARMED:
      if (!is_home_armed) {
          Serial.println("HOME_ARMED: No partition is armed, transitioning to IDLE");
          transition(IDLE);
      } else {
        if (is_garage_armed) {
          Serial.println("HOME_ARMED: Both partitions are armed, transitioning to BOTH_ARMED");
          transition(BOTH_ARMED);
        } else {
          // Is it time to arm the garage?
          if (!door_was_open_recently) {
            Serial.println("HOME_ARMED: Garage arming delay expired, arming it. Transitioning to GARAGE_ARMING");
            actuateKeySwitch();
            transition(GARAGE_ARMING);
          }
        }
      }
    break;
    case GARAGE_ARMING:
      if (is_garage_armed && is_home_armed) {
        Serial.println("GARAGE_ARMING: Garage is now armed, transitioning to BOTH_ARMED");
        transition(BOTH_ARMED);
      } else if (is_home_armed) {
        if (timediff > GARAGE_ARMING_TIMEOUT) {
          Serial.println("GARAGE_ARMING: Failed to arm the garage, transitioning to EXCEPTION");
          transition(EXCEPTION);
        }
        if (door_was_open_recently) {
          Serial.println("GARAGE_ARMING: Door was open during the arming delay, transitioning to HOME_ARMED");
          transition(HOME_ARMED);
        }
      } else {
        Serial.println("GARAGE_ARMING: Home is no longer armed, transitioning to IDLE");
        transition(IDLE);
      }
    break;

    case GARAGE_DISARMING:
      if (is_garage_armed) {
        if (timediff > GARAGE_ARMING_TIMEOUT) {
          Serial.println("GARAGE_DISARMING: Failed to disarm the garage, transitioning to EXCEPTION");
          transition(EXCEPTION);
        }
      } else {
        Serial.println("GARAGE_DISARMING: Garage is no longer armed, transitioning to IDLE");
        transition(IDLE);
      }
    break;

    case EXCEPTION:
      if (!is_home_armed && !is_garage_armed) {
        Serial.println("EXCEPTION: Both partitions are disarmed, returning to IDLE");
        transition(IDLE);
      }
    break;
  }

  updateLedState();
  delay(100);
}
