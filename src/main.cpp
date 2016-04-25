#include <Arduino.h>

#include <FS.h>

#include <ESP8266WiFi.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define TXPWR 1
#define MODE PHY_MODE_11N

#define HIDDEN false
#define CHANNEL 2

#define ESSID "Lector de microchip"
#define PASS "00000000"
#define BSSID {0xD0, 0x6D, 0x06, 0xD0, 0x6D, 0x06}

AsyncWebServer webServer(80);
String code = "NULL";
File codes, header;

extern "C" {
  #include "user_interface.h"
  void __run_user_rf_pre_init(void) {
    uint8_t mac[] = BSSID;
    system_phy_set_max_tpw(0);
    wifi_set_phy_mode(PHY_MODE_11N);
    wifi_set_macaddr(SOFTAP_IF, &mac[0]);
  }
}

void setup() {
  Serial.begin(9600);
  ArduinoOTA.begin();

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(10, 10, 10, 10),
                    IPAddress(10, 10, 10, 10),
                    IPAddress(255, 255, 255, 0));
  WiFi.softAP(ESSID, PASS, CHANNEL, HIDDEN);

  if (!SPIFFS.begin()) { Serial.println("SPIFFS error"); return; }

  webServer.onNotFound([](AsyncWebServerRequest *request) { request->send(SPIFFS, "/index.html"); });
  webServer.on("/s", [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", code);
    code = "NULL";
  });
  webServer.on("/c", [](AsyncWebServerRequest *request) {
    SPIFFS.remove("/codes.txt");
    request->send(200, "text/plain", "OK");
  });
  webServer.on("/memory.txt", [](AsyncWebServerRequest *request) {
    if(SPIFFS.exists("/codes.txt")) {
      request->send(SPIFFS, "/codes.txt", String(), true);
    } else {
      request->send(200, "text/plain", " ");
    }
  });
  webServer.on("/history.html", [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    codes = SPIFFS.open("/codes.txt", "r+");
    header = SPIFFS.open("/history.header", "r+");
    while (header.available() > 0) { response->write(header.read()); }
    while (codes.available() > 0) { response->write(codes.read()); }
    codes.flush(); codes.close(); header.flush(); header.close();
    response->println("</code></pre></body></html>");
    request->send(response);
  });
  webServer.serveStatic("", SPIFFS, "");
  webServer.begin();
}


void loop() {
  ArduinoOTA.handle();
  if(Serial.available() > 1) {
    code  = Serial.readStringUntil('\n');
    codes = SPIFFS.open("/codes.txt", "a+b");
    codes.println(code);
    codes.flush();
    codes.close();
  }
}
