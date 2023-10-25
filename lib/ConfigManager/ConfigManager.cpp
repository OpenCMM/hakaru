#include "ConfigManager.h"

Preferences preferences;
WebServer server(80);

const char *apSSID = "opencmm";
const char *apPassword = "opencmm1sensor";

const char *ssidKey = "wifi_ssid";
const char *passwordKey = "wifi_password";

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
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.print('.');
    }
    Serial.println("Successfully connected to WiFi.");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("No Wi-Fi credentials found.");
  }
}

void handleRoot()
{
  String html = "<!DOCTYPE html><html><body>";
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");

  if (n == 0)
  {
    Serial.println("no networks found");
    html += "<h2>No Wi-Fi Networks Found</h2>";
    html += "<p>Refresh the page to scan again.</p>";
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    delay(10);
    html += "<p>Select SSID</p>";

    for (int i = 0; i < n; i++)
    {
      html += "<p><a href='/connect?ssid=" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</a></p>";
    }
  }
  html += "</body></html>";

  Serial.println("");
  WiFi.scanDelete();

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
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to the WiFi network");
  server.close();

  // Redirect to a success page or any other action you want
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "credentials saved");
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