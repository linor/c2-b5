// EasyTransferI2C inspired library
#include <Wire.h>

class ETI2C {
  public:
    void begin(uint8_t *target, uint8_t target_size, TwoWire *i2c_bus);
    bool receiveData();
  private:
    TwoWire *_i2c_bus;
    uint8_t *_target_buffer;
    uint8_t _target_buffer_size;
    uint8_t *_rx_buffer;
    uint8_t _rx_buffer_position;
    uint8_t _expected_packet_size;
    uint8_t _state;
};

