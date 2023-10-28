#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

void checkWifiInfo();
void runServer();
void startAPServer();
void saveWiFiCredentials(String ssid, String password);
void handleRoot();
void handleConfig();
void handleConnect();
void connectToWiFi();
bool isIntervalValid(int interval);
bool isThresholdValid(int threshold);
void scanNetworks();
bool checkIfScanCompleted();
void resetWifiCredentialsWithWs();