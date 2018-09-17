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

#define I2C_ADDRESS       8

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

#define MAX_ROTATION_TIME 20000

#define RAMP_UP_SPEED     2
#define RAMP_DOWN_SPEED   3
#define RAMP_TIME_MILLIS  5

byte action[10];
byte *actionPtr = action;
long lastCommandTime = 0;
byte currentCommand = 0;

void setup() {
  Wire.begin(I2C_ADDRESS);                
  Wire.onReceive(receiveEvent); 

  Serial.begin(9600);
  Serial.println("Dome Controller");
  Serial.println("(c) Krijn Schaap");

  setup_motor();
  setup_position_control();
}

void update_action() {
  int used = actionPtr - action;

  if (used > 1) {
    switch(action[0]) {
      case 1:
        if (used < 3) return;

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
    actionPtr = action;
  }  
}

void loop() {
  position_loop();
  update_action();
  update_motor();

  if (currentCommand == 1) {
    if ((millis() - lastCommandTime) > MAX_ROTATION_TIME) {
      stop_motors();
      Serial.println("Emergency stop");
      currentCommand = 0;
    }
  }
}

void receiveEvent(int howMany) {
  int used = actionPtr - action;
  if ((sizeof(action) - used) < howMany) {
    while(Wire.available()) {
      Wire.read();
      Serial.println("Disposing");    
    }
    return;
  }
  
  Wire.readBytes(actionPtr, howMany);
  actionPtr = actionPtr + howMany;
}

