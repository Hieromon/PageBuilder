#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "PageBuilder.h"
#include "WebLED.h"

#define ONBOARD_LED 2

static const char _PAGE_LED[] PROGMEM = {
"<!DOCTYPE html>"
"<html>"
"<head>"
  "<meta charset=\"UTF-8\">"
  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  "<title>ESP8266 LED Control</title>"
  "<style type=\"text/css\">"
  "{{STYLE}}"
  "</style>"
"</head>"
"<body>"
  "<div class=\"one\">"
  "<p><a id=\"button\" href=\"/?led=on\">ON</a></p>"
  "<p><a id=\"button\" href=\"/?led=off\">OFF</a></p>"
  "</div>"
  "<div class=\"img\">"
  "{{LEDIO}}"
  "</div>"
"</body>"
"</html>"
};

static const char _STYLE_BUTTON[] PROGMEM = {
"body { -webkit-appearance:none; }"
"#button {"
  "display:block;"
  "width:150px;"
  "margin:10px auto;"
  "padding:7px 13px;"
  "text-align:center;"
  "background:#668ad8;"
  "font-size:20px;"
  "font-family:'Arial',sans-serif;"
  "color:#ffffff;"
  "white-space:nowrap;"
  "box-sizing:border-box;"
  "-webkit-box-sizing:border-box;"
  "-moz-box-sizing:border-box;"
"}"
"#button:active {"
  "font-weight:bold;"
  "vertical-align:top;"
  "padding:8px 13px 6px;"
"}"
".one a { text-decoration:none; }"
".img { text-align:center; }"
};

String ledIO(PageArgument& args) {
  String ledImage = "";
  if (args.hasArg("led")) {
    if (args.arg("led") == "on") {
      digitalWrite(BUILTIN_LED, LOW);
    }
    else if (args.arg("led") == "off")
      digitalWrite(BUILTIN_LED, HIGH);
  }
  delay(10);
  if (digitalRead(BUILTIN_LED) == LOW)
      ledImage = String("<img src=\"") + FPSTR(_PNG_LED) + String("\"/>");
  return ledImage;
}

PageElement Button(_PAGE_LED, {
  {"STYLE", [](PageArgument& arg) { return String(_STYLE_BUTTON); }},
  {"LEDIO", ledIO }
});
PageBuilder LEDPage("/", {Button});

ESP8266WebServer  Server;

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, HIGH);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin("SHAP-G","A0309T0312#");
  do {
    delay(500);
  } while (WiFi.waitForConnectResult() != WL_CONNECTED);
  LEDPage.insert(Server);
  Server.begin();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED)
    digitalWrite(ONBOARD_LED, LOW);
  else
    digitalWrite(ONBOARD_LED, HIGH);
  Server.handleClient();
}
