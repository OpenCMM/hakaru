#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ConfigManager.h>
#include <Sensor.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

void ledBlink();

const int touchPin = 4; // GPIO4 as the touch-sensitive pin
int sensorData = 0;
int interval = 1000; // 1 second
int threshold = 100;
const char* hostname = "opencmm";

WebSocketsServer webSocket(81);

bool streaming = false;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    // streaming = false; // Stop streaming data when a client disconnects
    break;
  case WStype_TEXT:
  {
    const uint8_t size = JSON_OBJECT_SIZE(3);
    StaticJsonDocument<size> json;
    DeserializationError err = deserializeJson(json, payload);
    if (err)
    {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
    }
    const char *command = json["command"];

    if (strcmp(command, "start") == 0)
    {
      streaming = true; // Start streaming data when "startStreaming" message is received
      const int _interval = json["interval"];
      if (isIntervalValid(_interval))
      {
        interval = _interval;
        Serial.println("Interval: " + String(interval));
      }

      const int _threshold = json["threshold"];
      if (isThresholdValid(_threshold))
      {
        threshold = _threshold;
        Serial.println("Threshold: " + String(threshold));
      }
      Serial.println("Streaming started");
    }
    else if (strcmp(command, "stop") == 0)
    {
      streaming = false; // Stop streaming data when "stopStreaming" message is received
      Serial.println("Streaming stopped");
    }
    else if (strcmp(command, "deepSleep") == 0)
    {
      streaming = false;
      Serial.println("Going to sleep now");
      delay(1000);
      // turn off sensor
      switchSensor(false);
      delay(1000);
      esp_deep_sleep_start();
    }
  }

  break;
  default:
    break;
  }
}

void touchCallback()
{
  // wake up from deep sleep
  Serial.println("Touch detected");
  // turn on sensor
  switchSensor(true);
  ledBlink();
}

void setup()
{
  Serial.begin(115200);
  MDNS.begin(hostname);
  checkWifiInfo();
  setupSensor();

  touchAttachInterrupt(touchPin, touchCallback, 40); // Attach touch interrupt

  // Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();

  ledBlink();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    webSocket.loop();

    // If streaming is enabled, get sensor data and send it to the client
    if (streaming)
    {
      int currentData = getSensorData();
      // if the difference is greater than threshold, send data
      if (abs(currentData - sensorData) < threshold)
      {
        return;
      }
      sensorData = currentData;
      String dataStr = String(currentData);
      Serial.println(dataStr);
      webSocket.broadcastTXT(dataStr.c_str(), dataStr.length());
      delay(interval);
    }
  } else {
    runServer();
    delay(1000);
    if (!checkIfScanCompleted()) {
      scanNetworks();
    }
  }
}

// LED blink
void ledBlink()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
}
