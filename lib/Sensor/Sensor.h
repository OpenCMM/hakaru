#include <Adafruit_ADS1X15.h>
#include <Arduino.h>
#include <time.h>

std::pair<int16_t, std::pair<time_t, int>> getSensorData();
void switchSensor(bool on);
void setupSensor();