#include "BTS7960.h"

BTS7960::BTS7960(int leftEnable, int rightEnable, int leftPWMPin, int rightPWMPin) {
  _leftEnable = leftEnable;
  _rightEnable = rightEnable;
  _leftPWM = leftPWMPin;
  _rightPWM = rightPWMPin;
  _enabled = false;

  pinMode(_leftEnable, OUTPUT);
  pinMode(_rightEnable, OUTPUT);
  pinMode(_leftPWM, OUTPUT);
  pinMode(_rightPWM, OUTPUT);

  digitalWrite(_leftEnable, LOW);
  digitalWrite(_rightEnable, LOW);

  analogWrite(_leftPWM, 0);
  analogWrite(_rightPWM, 0);
}

void BTS7960::enable() {
  digitalWrite(_leftEnable, HIGH);
  digitalWrite(_rightEnable, HIGH);

  _enabled = true;
}

void BTS7960::disable() {
  digitalWrite(_leftEnable, LOW);
  digitalWrite(_rightEnable, LOW);

  analogWrite(_leftPWM, 0);
  analogWrite(_rightPWM, 0);

  _speed = 0;
  _enabled = false;
}

void BTS7960::rotate(Direction dir, int speed) {
  _speed = speed;
  _direction = dir;
  
  if (dir == BTS7960::Left) {
    analogWrite(_rightPWM, 0);
    analogWrite(_leftPWM, speed);
  } else {
    analogWrite(_leftPWM, 0);
    analogWrite(_rightPWM, speed);
  }
}

void BTS7960::stop() {
  analogWrite(_leftPWM, 0);
  analogWrite(_rightPWM, 0);

  _speed = 0;
}

