/*
  FSPage.ino - Example PageBuilder with HTML page stored SPIFFS
  Copyright (c) 2017 Hieromon Ikasamo. All rights reserved.
  This software is released under the MIT License.
  http://opensource.org/licenses/mit-license.php

  This is a sample sketch that connects to an existing WiFi router already
  connected Internet and makes ESP8266 join the network.
  After writing this sketch, probably you can access 192.168.4.1 from
  your smartphone and it will be listed the WiFi AP that you can connect.
  You can select and connect the listed SSID.
  It is using Web page transition in the dialogue procedure for connecting
  to SSID, and its HTML source is stored in SPIFFS file.

  Before executing this sketch, you need to upload the data folder to SPIFFS
  of ESP8266 by the tool as "ESP8266 Sketch Data Upload" in Tools menu in
  Arduino IDE.
*/
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#define WIFI_EVENT_STA_CONNECTED      WIFI_EVENT_STAMODE_CONNECTED
#define WIFI_EVENT_STA_DISCONNECTED   WIFI_EVENT_STAMODE_DISCONNECTED
#define WIFI_EVENT_AP_STACONNECTED    WIFI_EVENT_SOFTAPMODE_STACONNECTED
#define WIFI_EVENT_AP_STADISCONNECTED WIFI_EVENT_SOFTAPMODE_STADISCONNECTED
#define WIFI_EVENT_ALL                WIFI_EVENT_ANY
#define WIFI_AUTH_OPEN                ENC_TYPE_NONE
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#define WIFI_EVENT_STA_CONNECTED      SYSTEM_EVENT_STA_CONNECTED
#define WIFI_EVENT_STA_DISCONNECTED   SYSTEM_EVENT_STA_DISCONNECTED
#define WIFI_EVENT_AP_STACONNECTED    SYSTEM_EVENT_AP_STACONNECTED
#define WIFI_EVENT_AP_STADISCONNECTED SYSTEM_EVENT_AP_STADISCONNECTED
#define WIFI_EVENT_ALL                SYSTEM_EVENT_MAX
#endif
#include "PageBuilder.h"

#define AP_NAME "esp-ap"
#define AP_PASS "12345678"

#if defined(ARDUINO_ARCH_ESP8266)
ESP8266WebServer server;
#elif defined(ARDUINO_ARCH_ESP32)
WebServer server;
#endif

bool    CONNECT_REQ;
String  CONN_SSID;
String  CONN_PSK;
String  REDIRECT_URI;
String  CURRENT_HOST;

// Page URIs
#define URI_ROOT    "/"
#define URI_JOIN    "/join"
#define URI_REQ     "/request"
#define URI_RESULT  "/result"
#define URI_WELCOME "/welcome"
#define URI_FAILED  "/failed"

// Connection request redirector
String reqConnect(PageArgument&);
PageElement REQ_ELM("{{REQ}}", { {"REQ", reqConnect} });
PageBuilder REQ_PAGE(URI_REQ, {REQ_ELM});

// Connection result redirector
String resConnect(PageArgument&);
PageElement CONNRES_ELM("{{CONN}}", { {"CONN", resConnect} });
PageBuilder CONNRES_PAGE(URI_RESULT, {CONNRES_ELM});

// This callback function would be invoked from the root page and scans nearby
// WiFi-AP to make a connectable list.
String listSSID(PageArgument& args) {
  String s_ssid_list = "";
  int8_t nn = WiFi.scanNetworks(false, true);
  for (uint8_t i = 0; i < nn; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0)
      ssid = "?";
    s_ssid_list += "<div class=\"ssid_list\"><a href=\"" + String(URI_JOIN) + String("?ssid=");
    s_ssid_list += ssid == "?" ? "%3F" : ssid;
    s_ssid_list += "&psk_type=" + String(WiFi.encryptionType(i)) + "\">" + ssid + "</a>";
    s_ssid_list += String(toWiFiQuality(WiFi.RSSI(i))) + "%";
    if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN)
      s_ssid_list += "<span class=\"img_lock\" />";
    s_ssid_list += "</div>";
  }
  CONNECT_REQ = false;
  return s_ssid_list;
}

// Convert RSSI dBm to signal strength
unsigned int toWiFiQuality(int32_t rssi) {
  unsigned int  qu;
  if (rssi <= -100)
    qu = 0;
  else if (rssi >= -50)
    qu = 100;
  else
    qu = 2 * (rssi + 100);
  return qu;
}

// Accepting connection request.
// Flag the occurrence of the connection request and response a redirect to
// the connection result page.
String reqConnect(PageArgument& args) {
  if (CONN_SSID == "?")
    CONN_SSID = args.arg("ssid");
  CONN_PSK = args.arg("psk");

  // Leave from the AP currently.
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
    while (WiFi.status() == WL_CONNECTED);
  }

  // Available upon connection request.
  CONNECT_REQ = true;
  REDIRECT_URI = "";
  CURRENT_HOST = "";

  // Redirect http request to connection result page.
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + String(URI_RESULT), true);
  server.send(302, "text/plain", "");
  REQ_PAGE.cancel();
  return "";
}

// Performing AP connection procedure.
void connWiFi() {
  // Starts connecting the ap as WIFI_STA
  Serial.print("Connect to " + CONN_SSID + "/" + CONN_PSK + " ");
  WiFi.begin(CONN_SSID.c_str(), CONN_PSK.c_str());
  unsigned long tm = millis();
  do {
    delay(500);
    Serial.print('.');
    if (millis() - tm > 30000)
      break;
  } while (WiFi.status() != WL_CONNECTED);
  Serial.print(String(WiFi.status()) + ":");

  // Dynamically switch the result page URL according to the connection result.
  // If it connected to the AP, redirect localIP () as the host address.
  // Connection failed, redirect to a failed page addressed on softAPIP().
  if (WiFi.status() == WL_CONNECTED) {
    // Connection established, makes a response page into a new IP address.
    CURRENT_HOST = WiFi.localIP().toString();
    REDIRECT_URI = URI_WELCOME;
    Serial.println("OK");
  }
  else {
    // Connection refused, a fails page keeps on softAPIP.
    CURRENT_HOST = WiFi.softAPIP().toString();
    REDIRECT_URI = URI_FAILED;
    Serial.println("Failed");
  }
}

// Responds redirect destination according to the connection result of success
// or failure to the AP.
String resConnect(PageArgument& args) {
  String redirect = String("http://") + CURRENT_HOST + REDIRECT_URI;
  Serial.println("Redirect: " + redirect);
  server.sendHeader("Location", redirect, true);
  server.sendHeader("Connection", "keep-alive");
  server.send(302, "text/plain", "");
  server.client().stop();
  CONNRES_PAGE.cancel();

  Serial.println();
  WiFi.printDiag(Serial);
  return "";
}

// WiFi event determination and echo back.
void broadcastEvent(WiFiEvent_t event) {
  static const char eventText_APSTA_CONN[] PROGMEM = "SoftAP station connected.";
  static const char eventText_APSTA_DISC[] PROGMEM = "SoftAP station disconnected.";
  static const char eventText_STA_CONN[] PROGMEM = "WiFi AP connected.";
  static const char eventText_STA_DISC[] PROGMEM = "WiFi AP disconnected.";
  const char* eventText;

  switch (event) {
    case WIFI_EVENT_AP_STACONNECTED:
      eventText = eventText_APSTA_CONN;
      break;
    case WIFI_EVENT_AP_STADISCONNECTED:
      eventText = eventText_APSTA_DISC;
      break;
    case WIFI_EVENT_STA_CONNECTED:
      eventText = eventText_STA_CONN;
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      eventText = eventText_STA_DISC;
      break;
    default:
      eventText = nullptr;
  }
  if (eventText)
    Serial.println(String("[event] ") + String(FPSTR(eventText)));
}

// Get an architecture of compiled
String getArch(PageArgument& args) {
#if defined(ARDUINO_ARCH_ESP8266)
  return "ESP8266";
#elif defined(ARDUINO_ARCH_ESP32)
  return "ESP32";
#endif
}

// HTML page declarations.
// root page
PageElement SSID_ELM("file:/root.htm", {
  { "SSID_LIST", listSSID },
  { "URI_ROOT", [](PageArgument& args) { return URI_ROOT; }}
});
PageBuilder SSID_PAGE(URI_ROOT, { SSID_ELM });

// SSID & Password entry page
PageElement ENTRY_ELM("file:/entry.htm", {
  { "ESP_ARCH", getArch },
  { "ENTRY", [](PageArgument& args) { CONN_SSID = args.arg("ssid"); return "AP"; } },
  { "URI_REQ", [](PageArgument& args) { return URI_REQ; } },
  { "SSID", [](PageArgument& args) { return CONN_SSID == "?" ? "placeholder=\"SSID\"" : String("value=\"" + CONN_SSID + "\" readonly"); } },
  { "PSK", [](PageArgument& args) { return args.arg("psk_type") != "7" ? "<input type=\"text\" name=\"psk\" placeholder=\"Password\" />" : ""; } }
});
PageBuilder ENTRY_PAGE(URI_JOIN, {ENTRY_ELM});

// Connection successful page
PageElement WELCOME_ELM("file:/connect.htm", {
  { "ESP-AP", [](PageArgument& args) { return AP_NAME; } },
  { "SSID", [](PageArgument& args) { return WiFi.SSID(); } },
  { "IP", [](PageArgument& args) { return WiFi.localIP().toString(); } },
  { "GATEWAY", [](PageArgument& args) { return WiFi.gatewayIP().toString(); } },
  { "SUBNET", [](PageArgument& args) { return  WiFi.subnetMask().toString(); } }
});
PageBuilder WELCOME_PAGE(URI_WELCOME, {WELCOME_ELM});

// Connection failed page
PageElement FAILED_ELM("file:/failed.htm", {
  { "SSID", [](PageArgument& args) { return CONN_SSID; } },
  { "RESULT", [](PageArgument& args) { return String(WiFi.status()); } }
});
PageBuilder FAILED_PAGE(URI_FAILED, {FAILED_ELM});

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();

  // Prepare HTML page 
  SSID_PAGE.insert(server);
  ENTRY_PAGE.insert(server);
  REQ_PAGE.insert(server);
  CONNRES_PAGE.insert(server);
  WELCOME_PAGE.insert(server);
  FAILED_PAGE.insert(server);

  // Start WiFi soft AP.
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_NAME, AP_PASS);
  while (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
    yield();
    delay(100);
  }
  Serial.print(AP_NAME " started. IP:");
  Serial.println(WiFi.softAPIP());

  // Start http server.
  SPIFFS.begin();
  server.begin();

  // Turn on WiFi event handling.
  WiFi.onEvent(broadcastEvent, WIFI_EVENT_ALL);
}

void loop() {
  server.handleClient();

  if (CONNECT_REQ) {
    connWiFi();
    CONNECT_REQ = false;
  }
}
