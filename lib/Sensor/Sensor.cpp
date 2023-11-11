#include <Sensor.h>

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

const int sensorControlPins[3] = {5, 18, 19};

void setupSensor() {
  Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");

  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }

  for (int i = 0; i < 3; i++) {
    pinMode(sensorControlPins[i], OUTPUT);
  }
  switchSensor(true);
}


// Switch sensor
void switchSensor(bool on) {
  for (int i = 0; i < 3; i++) {
    digitalWrite(sensorControlPins[i], on ? HIGH : LOW);
  }
}

std::pair<int16_t, std::pair<time_t, int>> getSensorData() {
  int16_t adc0;
  float volts0;

  adc0 = ads.readADC_SingleEnded(0);

  // volts0 = ads.computeVolts(adc0);

  // Serial.println("-----------------------------------------------------------");
  // Serial.print("AIN0: "); Serial.print(adc0); Serial.print("  "); Serial.print(volts0); Serial.println("V");

  // response time
  // fast: 1.5ms
  // standard: 5ms
  // slow: 10ms
  time_t now = time(nullptr);
  unsigned long millisSinceStart = millis();
  return std::make_pair(adc0, std::make_pair(now, millisSinceStart % 1000));
}
