#include "ETI2C.h"
#include <Arduino.h>

// #define DEBUG

void ETI2C::begin(uint8_t *target, uint8_t target_size, TwoWire *i2c_bus) {
  _i2c_bus = i2c_bus;
  _target_buffer = target;
  _target_buffer_size = target_size;
  _rx_buffer = (uint8_t*) malloc(target_size);
  _rx_buffer_position = 0;
  _expected_packet_size = 0;
  _state = 0;
}

bool ETI2C::receiveData() {
  while(_i2c_bus->available()) {
    uint8_t c = _i2c_bus->read();
    switch(_state) {
      case 0: 
        if (c == 0x06) {
          #ifdef DEBUG
            Serial.println("Start character found");
          #endif
          _state = 1;
        }
        break;
      case 1:
        if (c == 0x85) {
          _state = 2;
          #ifdef DEBUG
            Serial.println("Ready to read data");
          #endif
        } else {
          #ifdef DEBUG
            Serial.println("Wrong start sequence");
          #endif
          _state = 0;
        }
        break;
      case 2:
        if (c > _target_buffer_size) {
          #ifdef DEBUG
            Serial.print("Invalid buffer size, got ");
            Serial.print(c);
            Serial.print(", expected <= ");
            Serial.println(_target_buffer_size);
          #endif
          _state = 0;
        } else {
          _expected_packet_size = c;
          _rx_buffer_position = 0;
          _state = 3;
        }
        break;
      case 3: {
        _rx_buffer[_rx_buffer_position++] = c;
        if (_rx_buffer_position == _expected_packet_size) {
          #ifdef DEBUG
            Serial.println("Buffer full");
          #endif
          _state = 4;
        }
        break;
      }
      case 4: {
        uint8_t checksum = _expected_packet_size;
        for(int i = 0; i < _expected_packet_size; i++) {
          checksum ^= _rx_buffer[i];
        }
        #ifdef DEBUG
          Serial.print("Calculated checksum ");
          Serial.print(checksum);
          Serial.print(", received ");
          Serial.println(c);
        #endif

        _state = 0;

        if (checksum == c) {
          memset(_target_buffer, 0, _target_buffer_size);
          memcpy(_target_buffer, _rx_buffer, _expected_packet_size);
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

