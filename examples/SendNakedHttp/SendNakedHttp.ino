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

// This example shows the utility to send http response
// independently in the token function.
// If the sketch sends independently http response within
// the token function then it must inform to the PageBuilder
// about sending stop for http:200.

#if defined(ARDUINO_ARCH_ESP8266)
ESP8266WebServer  server;
#elif defined(ARDUINO_ARCH_ESP32)
WebServer  server;
#endif

String tokenFunc(PageArgument&);
PageElement elm("{{RES}}", {{ "RES", tokenFunc }});
PageBuilder page("/", { elm });   // Accessing "/" will call the tokenFunc.
PageElement elmHello("hello, world");
PageBuilder pageHello("/hello", { elmHello });

// The tokenFunc returns no HTML and sends redirection-code as
// particular response by an owned http header.
String tokenFunc(PageArgument& args) {

  // When accessing "/", it sends its own http response so that
  // it will be transferred to "/hello".
  server.sendHeader("Location", "/hello", true);
  server.sendHeader("Connection", "keep-alive");
  server.send(302, "text/plain", "");
  server.client().stop();

  // The cancel method notifies sending stop to the PageBuilder.
  page.cancel();
  return "";
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSK);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(300);
  }
  page.insert(server);
  pageHello.insert(server);
  server.begin();
  Serial.println("http server:" + WiFi.localIP().toString());
}

void loop() {
  server.handleClient();
}
