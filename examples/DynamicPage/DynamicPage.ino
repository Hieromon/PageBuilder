/*
  DynamicPage.ino, Example for the PageBuilder library.
  Copyright (c) 2017, Hieromon Ikasamo
  https://github.com/Hieromon/PageBuilder
  This software is released under the MIT License.
  https://opensource.org/licenses/MIT
*/

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif
#include <PageBuilder.h>

// Modify according to your Wi-Fi environment.
#define SSID  "wifissid"
#define PSK   "wifipassword"

#if defined(ARDUINO_ARCH_ESP8266)
ESP8266WebServer  server;
#elif defined(ARDUINO_ARCH_ESP32)
WebServer  server;
#endif

PageElement  elm;
PageBuilder  page;
String  currentUri;

// The root page content builder
String rootPage(PageArgument& args) {
  return String(F("This is root page."));
}

// The hello page content builder
String helloPage(PageArgument& args) {
  return String(F("This is hello page."));
}

// This function creates dynamic web page by each request.
// It is called twice at one time URI request that caused by the structure
// of ESP8266WebServer class.
bool handleAcs(HTTPMethod method, String uri) {
  if (uri == currentUri) {
    // Page is already prepared.
    return true;
  } else {
    currentUri = uri;
    page.clearElements();         // Discards the remains of PageElement.
    page.addElement(elm);         // Register PageElement for current access.

    Serial.println("Request:" + uri);

    if (uri == "/") {             // for the / page
      page.setUri(uri.c_str());
      elm.setMold(F(
        "<html>"
        "<body>"
        "<h2>{{ROOT}}</h2>"
        "</body>"
        "</html>"));
      elm.addToken("ROOT", rootPage);
      return true;
    }
    else if (uri == "/hello") {   // for the /hello page
      page.setUri(uri.c_str());
      elm.setMold(F(
        "<html>"
        "<body>"
        "<p style=\"color:Tomato;\">{{HELLO}}</p>"
        "</body>"
        "</html>"));
      elm.addToken("HELLO", helloPage);
      return true;
    }
    else {
      return false;    // Not found to accessing exception URI.
    }
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();

  WiFi.mode(WIFI_STA);
  delay(100);
  WiFi.begin(SSID, PSK);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(100);
  }

  // Prepare dynamic web page
  page.exitCanHandle(handleAcs);    // Handles for all requests.
  page.insert(server);

  // Start web server
  server.begin();
  Serial.print("http server:");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
}
