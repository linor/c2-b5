BTS7960 motor = BTS7960(MOTOR_L_ENABLE, MOTOR_R_ENABLE, MOTOR_L_PWM, MOTOR_R_PWM);
BTS7960::Direction currentMotorDirection;
int currentMotorSpeed = 0;
int targetMotorSpeed = 0;
long lastSpeedUpdate = 0;

void setup_motor() {
  motor.enable();
}

void stop_motors() {
  motor.stop();
  currentMotorSpeed = 0;
  targetMotorSpeed = 0;
}

void set_motor_speed(BTS7960::Direction dir, int speed, int ramp) {
  if ((currentMotorSpeed != 0) && (dir != currentMotorDirection)) {
    Serial.println("Can't reverse motor direction without going through stop");
    if (!ramp) currentMotorSpeed = 0;
    targetMotorSpeed = 0;
    return;
  }

  currentMotorDirection = dir;
  targetMotorSpeed = speed;
  if (!ramp) currentMotorSpeed = speed;
}

void update_motor() {
  long diff = millis() - lastSpeedUpdate;
  while(diff > RAMP_TIME_MILLIS) {
    diff -= RAMP_TIME_MILLIS;

    if (currentMotorSpeed < targetMotorSpeed) {
      currentMotorSpeed = min(currentMotorSpeed + RAMP_UP_SPEED, targetMotorSpeed);
    } else if (currentMotorSpeed > targetMotorSpeed) {
      currentMotorSpeed = max(currentMotorSpeed - RAMP_DOWN_SPEED, targetMotorSpeed);
    }
  }
  lastSpeedUpdate = millis() - diff;

  motor.rotate(currentMotorDirection, max(0, min(255, currentMotorSpeed)));
}

