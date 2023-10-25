#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

void checkWifiInfo();
void runServer();
void startAPServer();
void saveWiFiCredentials(String ssid, String password);
void handleRoot();
void handleConfig();
void connectToWiFi();
