#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ConfigManager.h>
#include <Sensor.h>

void ledBlink();

const int touchPin = 4; // GPIO4 as the touch-sensitive pin
int sensorData = 0;
int interval = 1000; // 1 second
int threshold = 100;

WebSocketsServer webSocket(81);

bool streaming = false;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      // streaming = false; // Stop streaming data when a client disconnects
      break;
    case WStype_TEXT:
      if (strcmp((char*)payload, "startStreaming") == 0) {
        streaming = true; // Start streaming data when "startStreaming" message is received
        Serial.println("Streaming started");
      } else if (strcmp((char*)payload, "stopStreaming") == 0) {
        streaming = false; // Stop streaming data when "stopStreaming" message is received
        Serial.println("Streaming stopped");
      } else if (strcmp((char*)payload, "deepSleep") == 0) {
        streaming = false;
        Serial.println("Going to sleep now");
        delay(1000);
        // turn off sensor
        switchSensor(false);
        delay(1000);
        esp_deep_sleep_start();
      }
      break;
    default:
      break;
  }
}

void touchCallback() {
  // wake up from deep sleep
  Serial.println("Touch detected");
  // turn on sensor
  switchSensor(true);
  ledBlink();
}

void setup() {
  Serial.begin(115200);
  checkWifiInfo();

  setupSensor();

  touchAttachInterrupt(touchPin, touchCallback, 40); // Attach touch interrupt

  //Configure Touchpad as wakeup source
  esp_sleep_enable_touchpad_wakeup();

  ledBlink();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    webSocket.loop();

    // If streaming is enabled, get sensor data and send it to the client
    if (streaming) {
      int currentData = getSensorData();
      // if the difference is greater than threshold, send data
      if (abs(currentData - sensorData) < threshold) {
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
  }
}

// LED blink
void ledBlink() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
}