#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

int mockGetSensorData();
void ledBlink();

const char* ssid = "<ssid>";
const char* password =  "<password>";
const float slowResponseTime = 10.0;

WebSocketsServer webSocket(81);

bool streaming = false;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      streaming = false; // Stop streaming data when a client disconnects
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
        esp_deep_sleep_start();
      }
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);

  // connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  ledBlink();
  Serial.println("Connected to the WiFi network");

  // print the IP address
  Serial.println(WiFi.localIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();

  // If streaming is enabled, get sensor data and send it to the client
  if (streaming) {
    int randomNum = mockGetSensorData();
    String randomNumStr = String(randomNum);
    Serial.println(randomNumStr);
    webSocket.broadcastTXT(randomNumStr.c_str(), randomNumStr.length());
    delay(1000);
  }
}

int mockGetSensorData() {
  // Generate a random number here
  int randomNum = random(0, 1000);
  // Serial.println(randomNum);

  // response time
  // fast: 1.5ms
  // standard: 5ms
  // slow: 10ms
  delay(slowResponseTime);
  return randomNum;
}

// LED blink
void ledBlink() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
}