#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

int mockGetSensorData();
void ledBlink();

const char* ssid = "<ssid>";
const char* password =  "<password>";
const float slowResponseTime = 10.0;

WebSocketsServer webSocket(81);

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
}

void loop() {
  webSocket.loop();

  // Generate a random number here
  int randomNum = mockGetSensorData();

  // broadcast sensor data to all connected clients
  String randomNumStr = String(randomNum);
  webSocket.broadcastTXT(randomNumStr.c_str(), randomNumStr.length());
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