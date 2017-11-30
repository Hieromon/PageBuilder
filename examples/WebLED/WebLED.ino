#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "PageBuilder.h"
#include "WebLED.h"   // Only the LED lighting icon

// Web page structure is described as follows.
// It contains two tokens as {{STYLE}} and {{LEDIO}} also 'led'
//  parameter for GET method.
static const char _PAGE_LED[] PROGMEM = {
"<!DOCTYPE html>"
"<html>"
"<head>"
  "<meta charset=\"UTF-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  "<title>ESP8266 LED Control</title>"
  "<style type=\"text/css\">"
  "{{STYLE}}"
  "</style>"
"</head>"
"<body>"
  "<p>ESP8266 LED Control</p>"
  "<div class=\"one\">"
  "<p><a class=\"button\" href=\"/?led=on\">ON</a></p>"
  "<p><a class=\"button\" href=\"/?led=off\">OFF</a></p>"
  "</div>"
  "<div class=\"img\">"
  "<img src=\"{{LEDIO}}\"/>"
  "</div>"
"</body>"
"</html>"
};

// A style description can be separated from the page structure.
// It expands from {{STYLE}} caused by declaration of 'Button' PageElement.
static const char _STYLE_BUTTON[] PROGMEM = {
"body {-webkit-appearance:none;}"
"p {"
  "font-family:'Arial',sans-serif;"
  "font-weight:bold;"
  "text-align:center;"
"}"
".button {"
  "display:block;"
  "width:150px;"
  "margin:10px auto;"
  "padding:7px 13px;"
  "text-align:center;"
  "background:#668ad8;"
  "font-size:20px;"
  "color:#ffffff;"
  "white-space:nowrap;"
  "box-sizing:border-box;"
  "-webkit-box-sizing:border-box;"
  "-moz-box-sizing:border-box;"
"}"
".button:active {"
  "font-weight:bold;"
  "vertical-align:top;"
  "padding:8px 13px 6px;"
"}"
".one a {text-decoration:none;}"
".img {text-align:center;}"
};

// ONBOARD_LED is WiFi connection indicator.
// BUILTIN_LED is controled by the web page.
#define ONBOARD_LED 2     // Different pin assignment by each module

// This function is the logic for BUILTIN_LED on/off.
// It is called from the occurrence of the 'LEDIO' token by PageElement
//  'Button' declaration as following code.
String ledIO(PageArgument& args) {
  String ledImage = "";

  // Blinks BUILTIN_LED according to value of the http request parameter 'led'.
  if (args.hasArg("led")) {
    if (args.arg("led") == "on")
      digitalWrite(BUILTIN_LED, LOW);
    else if (args.arg("led") == "off")
      digitalWrite(BUILTIN_LED, HIGH);
  }
  delay(10);

  // Feedback the lighting icon depending on actual port value which
  //  indepent from the http request parameter.
  if (digitalRead(BUILTIN_LED) == LOW)
      ledImage = FPSTR(_PNG_LED);
  return ledImage;
}

// Page construction
PageElement Button(_PAGE_LED, {
  {"STYLE", [](PageArgument& arg) { return String(_STYLE_BUTTON); }},
  {"LEDIO", ledIO }
});
PageBuilder LEDPage("/", {Button});

ESP8266WebServer  Server;
const char* SSID = "********";  // Modify for your available WiFi AP
const char* PSK  = "********";  // Modify for your available WiFi AP

#define BAUDRATE 115200

void setup() {
  delay(1000);  // for stable the module.
  Serial.begin(BAUDRATE);

  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, HIGH);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSK);
  do {
    delay(500);
  } while (WiFi.waitForConnectResult() != WL_CONNECTED);
  Serial.println("Connected to " + String(SSID));

  LEDPage.insert(Server);
  Server.begin();

  Serial.print("Web server http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED)
    digitalWrite(ONBOARD_LED, LOW);
  else
    digitalWrite(ONBOARD_LED, HIGH);

  Server.handleClient();
}
