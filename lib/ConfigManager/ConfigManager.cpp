#include "ConfigManager.h"

Preferences preferences;
WebServer server(80);

const char *apSSID = "opencmm";
const char *apPassword = "opencmm1sensor";

const char *ssidKey = "wifi_ssid";
const char *passwordKey = "wifi_password";

boolean scanCompleted = false;
String scanResults = "";

IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void checkWifiInfo()
{
  preferences.begin("credentials", false);

  // Check if Wi-Fi credentials are already stored
  if (preferences.getBool("configured", false))
  {
    // Wi-Fi credentials exist, so connect to Wi-Fi
    connectToWiFi();
  }
  else
  {
    // Wi-Fi credentials don't exist, start in AP mode
    startAPServer();
  }
}

void runServer()
{
  server.handleClient();
}

void startAPServer()
{
  // Start in AP mode
  WiFi.softAP(apSSID, apPassword);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);

  // Define the web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/connect", HTTP_GET, handleConnect);
  server.on("/config", HTTP_POST, handleConfig);

  server.begin();
}

void connectToWiFi()
{
  String ssid = preferences.getString(ssidKey, "");
  String password = preferences.getString(passwordKey, "");

  delay(3000);

  if (ssid != "" && password != "")
  {
    Serial.println("ssid: " + ssid);

    // Attempt to connect to Wi-Fi
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting to WiFi ..");

    int i = 0;
    while (WiFi.status() != WL_CONNECTED && i++ < 5) {
      delay(1000);
      Serial.print('.');
    }
  } else {
    Serial.println("No Wi-Fi credentials found.");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Successfully connected to WiFi.");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Could not connect to WiFi.");
    startAPServer();
  }
}

void handleRoot()
{
  String html = "";
  if (scanCompleted) { 
      html += scanResults;
      scanResults = "";  // Clear the results
      scanCompleted = false;
  } else {
      html += "<!DOCTYPE html><html><body>";
      html += "<h2>Scanning for Wi-Fi Networks...</h2>";
      html += "</body></html>";
  }

  Serial.println("Sending Wi-Fi scan results");
  server.send(200, "text/html", html);
}

void handleConnect()
{
  String ssid = server.arg("ssid");
  Serial.println("ssid: " + ssid);
  String html = "<!DOCTYPE html><html><body>";
  html += "<h2>Enter Wi-Fi Credentials</h2>";
  html += "<p>SSID: " + ssid + "</p>";
  html += "<form method='post' action='/config'>";
  html += "<input type='hidden' name='ssid' value='" + ssid + "'>";
  html += "Password: <input type='password' name='password'><br>";
  html += "<input type='submit' value='Submit'>";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleConfig()
{
  String ssid = server.arg("ssid");
  String password = server.arg("password");

  saveWiFiCredentials(ssid, password);

  // Switch to Station mode and connect to the user's Wi-Fi network
  WiFi.softAPdisconnect();
  WiFi.begin(ssid.c_str(), password.c_str());
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 5) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi.");
    startAPServer();
    return;
  } else {
    Serial.println("Connected to the WiFi network");
    server.close();
  }
}

void saveWiFiCredentials(String ssid, String password)
{
  // Store Wi-Fi credentials in NVS
  preferences.putString(ssidKey, ssid);
  preferences.putString(passwordKey, password);
  preferences.putBool("configured", true);
  Serial.println("Network Credentials Saved using Preferences");
  preferences.end();
}

bool isIntervalValid(int interval)
{
  return interval >= 2 && interval <= 1000;
}

bool isThresholdValid(int threshold)
{
  return threshold >= 1 && threshold <= 20000;
}

void scanNetworks() {
  int n = WiFi.scanNetworks();
  scanResults = "<!DOCTYPE html><html><body><h2>Select Wi-Fi Network</h2>";
  for (int i = 0; i < n; i++) {
    scanResults += "<p><a href='/connect?ssid=" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</a></p>";
  }
  scanResults += "</body></html>";
  scanCompleted = true;
  WiFi.scanDelete();
}

bool checkIfScanCompleted() {
  return scanCompleted;
}