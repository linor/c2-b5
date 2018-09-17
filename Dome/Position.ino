int home_position_found = 0;
volatile long position_in_steps = 0;

void pci_setup(byte pin)
{
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

ISR(PCINT1_vect) {
  if (!home_position_found) {
    if (!digitalRead(HALL_SENSOR_0)) {
      home_position_found = 1;
      position_in_steps = HOME_0_STEPS;
    } else if (!digitalRead(HALL_SENSOR_90)) {
      home_position_found = 2;
      position_in_steps = HOME_90_STEPS;
    } else if (!digitalRead(HALL_SENSOR_180)) {
      home_position_found = 3;
      position_in_steps = HOME_180_STEPS;
    } else if (!digitalRead(HALL_SENSOR_270)) {
      home_position_found = 4;
      position_in_steps = HOME_270_STEPS;
    }
  }

}

ISR(PCINT2_vect) {
  if (!home_position_found) return;

  if (digitalRead(COUNTER_1)) {
    if (digitalRead(COUNTER_2)) {
      position_in_steps++;
    } else {
      position_in_steps--;
    }
  }

  while(position_in_steps > COUNTER_STEPS) position_in_steps -= COUNTER_STEPS;
  while(position_in_steps < 0) position_in_steps += COUNTER_STEPS;
}

void setup_position_control() {
    Serial.println("Setting up position control");
     
    pci_setup(COUNTER_1);
    //pci_setup(COUNTER_2);
    pci_setup(HALL_SENSOR_0);
    pci_setup(HALL_SENSOR_90);
    pci_setup(HALL_SENSOR_180);
    pci_setup(HALL_SENSOR_270);
}

int last_home_position = home_position_found;
void position_loop() {
  if (home_position_found != last_home_position) {
    last_home_position = home_position_found;
    switch(last_home_position) {
      case 1: Serial.println("Home position found at 0 degrees"); break;
      case 2: Serial.println("Home position found at 90 degrees"); break;
      case 3: Serial.println("Home position found at 180 degrees"); break;
      case 4: Serial.println("Home position found at 270 degrees"); break;
    }
  }
}

