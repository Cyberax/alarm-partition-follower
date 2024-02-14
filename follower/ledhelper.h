#pragma once

class LED
{
	int ledPin;
	unsigned long onTimeMillis;
	unsigned long offTimeMillis;
  bool isLedOn;

	unsigned long lastStateChangeMillis;  
public:
  enum LedState {
    OFF, ON, BLINKING
  };
  LedState state;

  LED(int pin, unsigned long on, unsigned long off) : ledPin(pin), onTimeMillis(on), offTimeMillis(off) {
  	pinMode(ledPin, OUTPUT);     	  	
	  isLedOn = false;
	  lastStateChangeMillis = millis();
    state = OFF;
  }

  void setLedState(LedState state) {
    this->state = state;
  }

  void update() {
    if (state == LedState::OFF) {
      digitalWrite(ledPin, LOW);
    } else if (state == LedState::ON) {
      digitalWrite(ledPin, HIGH);
    } else if (state == LedState::BLINKING) {
      unsigned long now = millis();

      if (isLedOn && (now - lastStateChangeMillis) >= onTimeMillis) {
        isLedOn = false;
        lastStateChangeMillis = now;
        digitalWrite(ledPin, LOW);
      } else if (!isLedOn && (now - lastStateChangeMillis) >= offTimeMillis) {
        isLedOn = true;
        lastStateChangeMillis = now;
        digitalWrite(ledPin, HIGH);
      }
    }
  }
};
