/**
 *  Dome Control
 *  Krijn Schaap, 2018
 *  
 *  I2C protocol <cmd><data>
 *  
 *  All off: 
 *  cmd 0 
 *  
 *  Rotate at speed:
 *  cmd 1
 *  data <direction><speed>
 *  direction 0 - left
 *            1 - right
 *  speed 0 - 100
 *  
 *  Rotate to angle
 *  cmd 2
 *  data <direction><angle>
 *  direction 0 - left
 *            1 - right
 *            2 - shortest
 *  angle, 2 bytes, degrees with one decimal * 10
 */
#include <Wire.h>
#include "BTS7960.h"
#include "ETI2C.h"

#define I2C_ADDRESS       8
#define I2C_MAX_COMMAND_DURATION 50

#define HALL_SENSOR_0     A0
#define HALL_SENSOR_90    A1
#define HALL_SENSOR_180   A2
#define HALL_SENSOR_270   A3

#define COUNTER_1         2
#define COUNTER_2         3
#define COUNTER_STEPS     1600

#define HOME_0_STEPS      0
#define HOME_90_STEPS     400
#define HOME_180_STEPS    800
#define HOME_270_STEPS    1200

#define MOTOR_R_PWM       10
#define MOTOR_L_PWM       9
#define MOTOR_L_ENABLE    8
#define MOTOR_R_ENABLE    11

#define MAX_ROTATION_TIME 2000

#define RAMP_UP_SPEED     2
#define RAMP_DOWN_SPEED   3
#define RAMP_TIME_MILLIS  5

byte action[10];
byte *actionPtr = action;
long lastCommandTime = 0;
byte currentCommand = 0;

ETI2C eti2c;

void setup() {
  Wire.begin(I2C_ADDRESS);    
  Wire.onReceive(receiveEvent);
  eti2c.begin((uint8_t *)action, sizeof(action), &Wire);            

  Serial.begin(9600);
  Serial.println("Dome Controller");
  Serial.println("(c) Krijn Schaap");

  setup_motor();
  setup_position_control();
}

void update_action() {
  switch(action[0]) {
    case 1:
      currentCommand = 1;
      unsigned int speed = map(action[2], 0, 100, 0, 255);

      if (action[1] == 0) {
        set_motor_speed(BTS7960::Left, speed, 1);
      } else {
        set_motor_speed(BTS7960::Right, speed, 1);
      }
      break; 
  }

  lastCommandTime = millis();
  memset(action, 0, sizeof(action));
}

void loop() {
  long current = millis();
  
  position_loop();
  update_motor();

  if (eti2c.receiveData()) {
    update_action();
  }
  
  if (currentCommand == 1) {
    if ((current - lastCommandTime) > MAX_ROTATION_TIME) {
      stop_motors();
      Serial.println("Emergency stop");
      currentCommand = 0;
    }
  }
}

void receiveEvent(int howMany) {
}

