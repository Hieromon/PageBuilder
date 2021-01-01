/*
  WebLED.ino, Example for the PageBuilder library.
  Copyright (c) 2017, 2020, Hieromon Ikasamo
  https://github.com/Hieromon/PageBuilder
  This software is released under the MIT License.
  https://opensource.org/licenses/MIT

  This example demonstrates the typical behavior of PageBuilder. It also
  represents a basic structural HTML definition with the PageElement.
*/

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif
#include <PageBuilder.h>
#include "WebLED.h"   // Only the LED lighting icon

// Web page structure is described as follows.
// It contains two tokens as {{STYLE}} and {{LEDIO}} also 'led'
//  parameter for GET method.
static const char _PAGE_LED[] PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
  <title>{{ARCH}} LED Control</title>
  <style type="text/css">
  {{STYLE}}
  </style>
</head>
<body>
  <p>{{ARCH}} LED Control</p>
  <div class="one">
  <p><a class="button" href="/?led=on">ON</a></p>
  <p><a class="button" href="/?led=off">OFF</a></p>
  </div>
  <div class="img">
  <img src="{{LEDIO}}"/>
  </div>
</body>
</html>
)";

// A style description can be separated from the page structure.
// It expands from {{STYLE}} caused by declaration of 'Button' PageElement.
static const char _STYLE_BUTTON[] PROGMEM = R"(
body {-webkit-appearance:none;}
p {
  font-family:'Arial',sans-serif;
  font-weight:bold;
  text-align:center;
}
.button {
  display:block;
  width:150px;
  margin:10px auto;
  padding:7px 13px;
  text-align:center;
  background:#668ad8;
  font-size:20px;
  color:#ffffff;
  white-space:nowrap;
  box-sizing:border-box;
  -webkit-box-sizing:border-box;
  -moz-box-sizing:border-box;
}
.button:active {
  font-weight:bold;
  vertical-align:top;
  padding:8px 13px 6px;
}
.one a {text-decoration:none;}
.img {text-align:center;}
)";

// ONBOARD_LED is WiFi connection indicator.
// LED_BUILTIN is controlled by the web page.
#ifndef LED_BUILTIN
#define ONBOARD_LED 16    // Different pin assignment by each module
#elif defined(LED_BUILTIN_AUX)
#define ONBOARD_LED LED_BUILTIN_AUX
#else
#define ONBOARD_LED LED_BUILTIN
#endif

// Get an architecture of compiled
String getArch(PageArgument& args) {
#if defined(ARDUINO_ARCH_ESP8266)
  return "ESP8266";
#elif defined(ARDUINO_ARCH_ESP32)
  return "ESP32";
#endif
}

// This function is the logic for LED_BUILTIN on/off.
// It is called from the occurrence of the 'LEDIO' token by PageElement
//  'Button' declaration as following code.
String ledIO(PageArgument& args) {
  String ledImage = "";

  // Blinks LED_BUILTIN according to value of the http request parameter 'led'.
  if (args.hasArg("led")) {
    if (args.arg("led") == "on")
      digitalWrite(LED_BUILTIN, LOW);
    else if (args.arg("led") == "off")
      digitalWrite(LED_BUILTIN, HIGH);
  }
  delay(10);

  // Feedback the lighting icon depending on actual port value which
  //  indepent from the http request parameter.
  if (digitalRead(LED_BUILTIN) == LOW)
    ledImage = FPSTR(_PNG_LED);
  return ledImage;
}

// Page construction
PageElement Button(FPSTR(_PAGE_LED), {
  {"STYLE", [](PageArgument& arg) { return String(FPSTR(_STYLE_BUTTON)); }},
  {"ARCH", getArch },
  {"LEDIO", ledIO }
});
PageBuilder LEDPage("/", {Button});

#if defined(ARDUINO_ARCH_ESP8266)
ESP8266WebServer  Server;
#elif defined(ARDUINO_ARCH_ESP32)
WebServer  Server;
#endif

const char* SSID = "********";  // Modify for your available WiFi AP
const char* PSK  = "********";  // Modify for your available WiFi AP

const char* username = "admin";
const char* password = "espadmin";

#define BAUDRATE 115200

void setup() {
  delay(1000);  // for stable the module.
  Serial.begin(BAUDRATE);

  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSK);
  do {
    delay(500);
  } while (WiFi.waitForConnectResult() != WL_CONNECTED);
  Serial.println("Connected to " + String(SSID));

  LEDPage.authentication(username, password, DIGEST_AUTH, "WebLED");
  LEDPage.transferEncoding(PageBuilder::TransferEncoding_t::ByteStream);
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
