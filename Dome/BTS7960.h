#ifndef BTS8960_h
#define BTS7960_h

#include "Arduino.h"

class BTS7960 {
  public:
    enum Direction
    {
      Left = 0,
      Right = 1
    }; 
    
    BTS7960(int leftEnable, int rightEnable, int leftPWMPin, int rightPWMPin);
    void enable();
    void disable();
    void rotate(Direction dir, int speed);
    void stop();
  private:
    Direction _direction;
    int _speed;
    bool _enabled;
    int _leftEnable;
    int _rightEnable;
    int _leftPWM;
    int _rightPWM;
};

#endif
