#include <Wire.h>
#include <Servo.h>

#define I2C_ADDRESS       9

#define RED_LED           13
#define GREEN_LED         12

#define LEFT_MOTOR        6
#define RIGHT_MOTOR       5

#define MAX_MOVEMENT_TIME 2000

//#define DEBUG

struct RECEIVE_DATA_STRUCTURE{
  int8_t x;
  int8_t y;
};

RECEIVE_DATA_STRUCTURE speedData;

Servo leftMotor;
Servo rightMotor;
volatile long lastMovementCommand;

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  leftMotor.attach(LEFT_MOTOR);
  rightMotor.attach(RIGHT_MOTOR);

  digitalWrite(GREEN_LED, HIGH);
 // digitalWrite(RED_LED, HIGH);

  Wire.begin(I2C_ADDRESS);
//  ET.begin(details(speedData), &Wire);
  Wire.onReceive(receive);

  Serial.begin(9600);
  Serial.println("Ready");
}

void set_motor_speed(int left, int right) {
  left = min(127, max(-128, left));
  right = min(127, max(-128, right));

  int servoLeft = left != 0 ? map(left, -128, 127, 50, 130) : 90;
  int servoRight = right != 0 ? map(right, -128, 127, 50, 130) : 90;

  leftMotor.write(servoLeft);
  rightMotor.write(servoRight);
}

void motors_set_movement(int nJoyX, int nJoyY) {
  // Differential Steering Joystick Algorithm 
  // ======================================== 
  // by Calvin Hass 
  // http://www.impulseadventure.com/elec/ 
  
  // 
  // Converts a single dual-axis joystick into a differential 
  // drive motor control, with support for both drive, turn 
  // and pivot operations. 
  
  // 
  // INPUTS 
  nJoyX = min(127, max(-128, nJoyX)); // Joystick X input (-128..+127) 
  nJoyY = min(127, max(-128, nJoyY)); // Joystick Y input (-128..+127) 
  
  // OUTPUTS 
  int nMotMixL; // Motor (left) mixed output (-128..+127) 
  int nMotMixR; // Motor (right) mixed output (-128..+127) 
  
  // CONFIG 
  // - fPivYLimt : The threshold at which the pivot action starts 
  // This threshold is measured in units on the Y-axis 
  // away from the X-axis (Y=0). A greater value will assign 
  // more of the joystick's range to pivot actions. 
  // Allowable range: (0..+127) 
  float fPivYLimit = 32.0; 
  
  // TEMP VARIABLES 
  float nMotPremixL; // Motor (left) premixed output (-128..+127) 
  float nMotPremixR; // Motor (right) premixed output (-128..+127) 
  int nPivSpeed; // Pivot Speed (-128..+127) 
  float fPivScale; // Balance scale b/w drive and pivot ( 0..1 ) 
  
  // Calculate Drive Turn output due to Joystick X input 
  if (nJoyY >= 0) { 
    // Forward 
    nMotPremixL = (nJoyX>=0)? 127.0 : (127.0 + nJoyX); 
    nMotPremixR = (nJoyX>=0)? (127.0 - nJoyX) : 127.0; 
  } else { 
    // Reverse 
    nMotPremixL = (nJoyX>=0)? (127.0 - nJoyX) : 127.0; 
    nMotPremixR = (nJoyX>=0)? 127.0 : (127.0 + nJoyX); 
  } 
  
  // Scale Drive output due to Joystick Y input (throttle) 
  nMotPremixL = nMotPremixL * nJoyY/128.0; 
  nMotPremixR = nMotPremixR * nJoyY/128.0; 
  
  // Now calculate pivot amount 
  // - Strength of pivot (nPivSpeed) based on Joystick X input 
  // - Blending of pivot vs drive (fPivScale) based on Joystick Y input 
  nPivSpeed = nJoyX; 
  fPivScale = (abs(nJoyY)>fPivYLimit)? 0.0 : (1.0 - abs(nJoyY)/fPivYLimit); 
  
  // Calculate final mix of Drive and Pivot 
  nMotMixL = (1.0-fPivScale)*nMotPremixL + fPivScale*( nPivSpeed); 
  nMotMixR = (1.0-fPivScale)*nMotPremixR + fPivScale*(-nPivSpeed);

  set_motor_speed(nMotMixL, nMotMixR);
}

void loop() {
  long current = millis();

  if (readI2C()) {
    lastMovementCommand = millis();
    digitalWrite(RED_LED, LOW);
//    Serial.print("x: ");
//    Serial.print(speedData.x);
//    Serial.print(" y: ");
//    Serial.println(speedData.y);
    motors_set_movement(speedData.x, speedData.y);
  }

  if ((millis() - lastMovementCommand) > MAX_MOVEMENT_TIME) {
    set_motor_speed(0, 0);
    digitalWrite(RED_LED, HIGH);
  }
}

volatile uint8_t i2cBuffer[2];
volatile int8_t i2cBufferPosition;
volatile uint8_t i2cState;

void receive(int numBytes) {}

bool readI2C() {
  while(Wire.available()) {
    uint8_t c = Wire.read();
    switch(i2cState) {
      case 0: 
        if (c == 0x06) i2cState = 1;
        break;
      case 1:
        if (c == 0x85) {
          i2cState = 2;
        } else {
          #ifdef DEBUG
            Serial.println("Wrong start sequence");
          #endif
          i2cState = 0;
        }
        break;
      case 2:
        if (c != sizeof(i2cBuffer)) {
          #ifdef DEBUG
            Serial.print("Invalid buffer size, got ");
            Serial.print(c);
            Serial.print(", expected ");
            Serial.println(sizeof(i2cBuffer));
          #endif
          i2cState = 0;
        } else {
          i2cBufferPosition = 0;
          i2cState = 3;
        }
        break;
      case 3: {
        i2cBuffer[i2cBufferPosition++] = c;
        if (i2cBufferPosition == sizeof(i2cBuffer)) {
          #ifdef DEBUG
            Serial.println("Buffer full");
          #endif
          i2cState = 4;
        }
        break;
      }
      case 4: {
        uint8_t checksum = sizeof(i2cBuffer);
        for(int i = 0; i < sizeof(i2cBuffer); i++) {
          checksum ^= i2cBuffer[i];
        }
        #ifdef DEBUG
          Serial.print("Calculated checksum ");
          Serial.print(checksum);
          Serial.print(", received ");
          Serial.println(c);
        #endif

        i2cState = 0;

        if (checksum == c) {
          memcpy(&speedData, i2cBuffer, sizeof(i2cBuffer));
          return true;
        } else {
          #ifdef DEBUG
            Serial.println("Invalid checksum");
          #endif
        }
        break;
      }
    }
  }
  return false;
}
