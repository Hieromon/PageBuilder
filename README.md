# PageBuilder - HTML assembly aid for ESP8266/ESP32 WebServer
[![Build Status](https://travis-ci.org/Hieromon/PageBuilder.svg?branch=master)](https://travis-ci.org/Hieromon/PageBuilder)
[![arduino-library-badge](https://www.ardu-badge.com/badge/PageBuilder.svg)](https://www.ardu-badge.com/PageBuilder)

*An arduino library to create html string in the sketch for ESP8266/ESP32 WebServer.* 

PageBuilder is an Arduino library class dedicated to the _ESP8266WebServer_ or the _WebServer(ESP32)_ for easily generating HTML pages and sending them to the client.

## Features

* Ability to completely separate HTML structure and the web page generation logic in the sketch
* No need for inline coding of URI access handler of ESP8266WebServer class or ESP32's WebServer class
* Fixed HTML statement parts like template can be allocated as PROGMEM
* Its HTML source can be stored SPIFFS and obtain automatically
* Arbitrary token can be specified inline HTML statement
* Automatically sent to client for HTML page are generated  

Ordinary sketch | Sketch by PageBuilder  
----------------|----------------------  
![ordinary_sketch](https://user-images.githubusercontent.com/12591771/33361508-cfad0c2e-d51b-11e7-873d-a401dc7decd9.png) | ![pagebuilder_sketch](https://user-images.githubusercontent.com/12591771/33361523-dc7e30cc-d51b-11e7-83a5-5f27bef0b71b.png)
  

## Works on

- For ESP8266  
Generic esp8266 module and other representatives works fine. ESP8266 Arduino core 2.4.0 or higher is necessary. 
- For ESP32  
Arduino core for ESP32 supported boards works fine. ESP32 Arduino core 1.0.0 or higher is necessary.

## Installation

Download this file as a zip, and extract the resulting folder into your Arduino Libraries folder. See [Installing Additional Arduino Libraries](https://www.arduino.cc/en/Guide/Libraries).  
Required [Arduino IDE](http://www.arduino.cc/en/main/software) is current upstream at **the 1.8 level or later**, and also [ESP8266 Arduino core](https://github.com/esp8266/Arduino) or [ESP32 Arduino core](https://github.com/espressif/arduino-esp32).

## Example

- A most simple example. - No URI handler is needed, only the URI path and html coded.

```c++
#include "PageBuilder.h"

// root page
PageElement ROOT_PAGE_ELEMENT("<a href=\"/hello\">hello</a>");
PageBuilder ROOT_PAGE("/", {ROOT_PAGE_ELEMENT});
// /hello page
PageElement HELLO_PAG_ELEMENT("Hello, world!<p><a href=\"/bye\">bye</a></p>");
PageBuilder HELLO_PAGE("/hello", {HELLO_PAG_ELEMENT});
// /bye page
PageElement BYE_PAGE_ELEMENT("Good bye!");
PageBuilder BYE_PAGE("/bye", {BYE_PAGE_ELEMENT});

ROOT_PAGE.insert(Server);     // Add root page
HELLO_PAGE.insert(Server);    // Add /hello page
BYE_PAGE.insert(Server);      // add /bye page
```
- Share multiple elements on different pages.

```c++
#include "PageBuilder.h"

static const char _HEAD[] PROGMEM = "<html>" ;
static const char _BODY[] PROGMEM = "<body>This is {{PAGE}}.</body>";
static const char _FOOT[] PROGMEM = "</html>";

String setBody1(PageArgument& args) {
	return String("Page1");
}

String setBody2(PageArgument& args) {
	return String("Page2");
}

PageElement header( _HEAD );
PageElement body1( _BODY, { {"PAGE", setBody1} });
PageElement body2( _BODY, { {"PAGE", setBody2} });
PageElement footer( _FOOT );

PageBuilder Page1( "/page1", {header, body1, footer} );
PageBuilder Page2( "/page2", {header, body2, footer} );
```
- HTML source stored in SPIFFS.  

The following screenshot is an example PageBuilder sketch using HTML source stored in SPIFFS. It scan the nearby signal and connect the ESP8266 to the specified access point.  
This case is [FSPage.ino example sketch](examples/FSPage/README.md) in this repository.

<kbd>![Join to WiFi](https://user-images.githubusercontent.com/12591771/34141214-14cb8cfc-e4c3-11e7-8afd-90efbdd6ac87.png "Join to WiFi")</kbd> <kbd>![Connect to WiFi](https://user-images.githubusercontent.com/12591771/34141303-aba4b284-e4c3-11e7-872b-69b9301cef10.png "Connect to WiFi")</kbd> <kbd>![Welcome](https://user-images.githubusercontent.com/12591771/34141320-cf390af6-e4c3-11e7-9bbd-44d7f80aa076.png "Welcome")</kbd>  

## Usage

### Data structure of PageBuilder

In order to successfully generate an HTML page using PageBuilder please understand the data structure of PageBuilder.  
PageBuilder library consists of three objects that are related to each other as the below. `PageBuilder` inherits `RequestHandler` provided from **ESP8266WebServer** (in the ESP8266 arduino core) or **WebServer** (in the ESP32 arduino core) library and is invoked from `ESP8266WebServer`/`WebServer` in response to http requests. PageBuilder owns its URI string and multiple PageElement objects.  
Source strings of HTML are owned by `PageElement` (`mold` in the figure). Its string contains an identifier called a **token**. The **token** appears as `{{ }}` in the middle of the source HTML string (`_token` in the figure). The tokens are paired with functions to replace them with actual HTML sentences. When URI access has occurred server from the client, its paired function is invoked by extension of `handleClient()` method then the **token** will replace to actual statement to complete the HTML and sends it. `PageElement` can have multiple tokens (i.e., it can define several tokens in one HTML source element).  
![default_data_structure](https://user-images.githubusercontent.com/12591771/33360699-293dc5ac-d518-11e7-8d31-728d500f02bf.png)  
To properly generate a web page, you need to code its function that replaces the token with HTML, and its function must return a String.  
```c++
String AsName(PageArgument& args) {      // User coded function
...    ~~~~~~
  return String("My name");
}
String AsDaytime(PageArgument& args) {   // User coded function
...    ~~~~~~~~~
  return String("afternoon");
}

// Source HTML string
const char html[] = "hello <b>{{NAME}}</b>, <br>Good {{DAYTIME}}.";
...                             ^^^^                   ^^^^^^^
...                             token                  token

PageElement header_elem("<html><body>");
PageElement footer_elem("</body></html>");
PageElement body_elem(html, { {"NAME", AsName}, {"DAYTIME", AsDayTime} });
...                             ^^^^   ~~~~~~     ^^^^^^^   ~~~~~~~~~
...                             token  User coded function to replace the token

PageBuilder page("/hello", { header_elem, body_elem, footer_elem });
...
ESP8266WebServer  webServer;  // in ESP8266 case.
page.insert(webServer);
webServer.begin();
...  // 'on' method is no needed.
webServer.handleClient();
```
http\://your.webserver.address/hello will respond as follows.

```html
<html><body>hello <b>My name</b>, <br>Good afternoon.</body></html>
```

### Invoke the HTML assembly

No need in the sketch. It would be invoked from the instance inherited from the WebServer class which corresponding to the platform ESP8266 or ESP32. Also, like the `on` method of the WebServer class, you need to register the PageBuilder object with the web server object using the `insert` method.

```c++
String func(PageArgument& args);
PageElement element("mold", {{"token", func}})
PageBuilder page("/uri", { element });

ESP8266WebServer server;  // Probably 'WebServer' in ESP32 case.
page.insert(server);    // This is needed.

server.handleClient();  // Invoke from this.
```

### Arguments of invoked user function

Arguments are passed to the **function** that should be implemented corresponding to tokens. It is the parameter value as GET or POST at the http request occurred like as `url?param=value` in HTTP GET, and its parameters are stored in `PageArgument` object and passed to the function as below.  
- HTTP GET with `http://xxx.xxx.xxx/?param=value`  
- HTTP POST with `/ HTTP/1.1 Host:xxx.xxx.xxx Connection:keep-alive param=value`

```c++
String func(PageArgument& args) {
  if (args.hasArg("param"))
    return args.arg("param");
}

PageElement element("hello {{NAME}}.", {{"NAME", func}});
```
An argument can be accessed with the following method of `PageArgument` class.

#### `String PageArgument::arg(String name)`  
Returns the value of the parameter specified by `name`.

#### `String PageArgument::arg(int i)`  
Returns the value of the parameter indexed `i`.

#### `String PageArgument::argName(int i)`  
Returns parameter name of indexed i.

#### `int PageArgument::args()`  
Get parameters count of the current http request.

#### `size_t PageArgument::size()`  
Same as `args()`.

#### `bool PageArgument::hasArg(String name)`  
Returns whether the `name` parameter is specified in the current http request.

## Declare PageBuilder object and PageElement object

### Include directive

```c++
#include "PageBuilder.h"
```

#### PageBuilder constructor
```c++
PageBuilder::PageBuilder();
PageBuilder::PageBuilder(PageElementVT element, HTTPMethod method = HTTP_ANY, TransferEncoding_t chunked = PB_Auto);
PageBuilder::PageBuilder(const char* uri, PageElementVT element, HTTPMethod method = HTTP_ANY, TransferEncoding_t chunked = PB_Auto);
```
- `element` : **PageElement** container wrapper. Normally, use the brackets to specify initializer.  
  ```c++
  PageElement elem1();
  PageElement elem2();
  PageBuilder page("/", {elem1, elem2});
  ```
- `method` : Enum value of HTTP method as `HTTP_ANY`, `HTTP_GET`, `HTTP_POST` that page should respond.
- `uri` : A URI string of the page.
- `chunked` : Enumeration type for the transfer-encoding as TransferEncoding_t type. `PB_Auto`, `PB_ByteStream`, `PB_Chunk` can be specified. If `PB_Auto` is specified, would be determined automatically to switch them the chunk. Its criteria is defined with `MAX_CONTENTBLOCK_SIZE` macro in `PageBuilder.cpp` code. If `PB_ByteStream` is specified, PageBuilder will determine the way of sending either String writing or byte stream according to a size of the content.

#### PageElement constructor
```c++
PageElement::PageElement();
PageElement::PageElement(const char* mold);
PageElement::PageElement(const char* mold, TokenVT source);
```
- `mold` : A pointer to HTML model string(const char array, PROGMEM available).
- `source` : Container of processable token and handler function. A **TokenVT** type is std::vector to the structure with the pair of *token* and *func*. It prepares with an initializer.  
  ```c++
  String func1(PageArgument& args);
  String func2(PageArgument& args);
  PageElement elem(html, {{"TOKEN1", func1}, {"TOKEN2", func2}});
  ```
  `mold` can also use external files placed on SPIFFS. Since HTML consists of more strings, the program area may be smaller in sketches using many pages.  
  External files can have HTML source specified by `mold`. That file would be allocated on the SPIFFS file system. This allows you to reduce the sketch size and assign more capacity to the program.  
  You can specify the HTML source file name by `mold` parameter in the following format.  
  ```
  file:FILE_NAME
  ```
  `FILE_NAME` is the name of the HTML source file containing `/`. If prefix **file:** is specified in `mold` parameter, the PageElement class reads its file from SPIFFS as HTML source. A sample sketch using this way is an example as [FSPage.ino](examples/FSPage/README.md).  
  For details for how to write HTML source file to SPIFFS of ESP8266, please refer to [Uploading files to file system](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#uploading-files-to-file-system).

## Methods

### PageBuilder Methods

#### `void PageBuilder::addElement(PageElement& element)`  
Add a new **PageElement** object to the container of **PageBuilder**. 
- `element` : PageElement object.

#### `void PageBuilder::atNotFound(ESP8266WebServer& server)`<br>`void PageBuilder::atNotFound(WebServer& server)`  
Register **the not found page** to the ESP8266WebServer. It has the same effect as `onNotFound` method of `ESP8266WebServer`/`WebServer`. The page registered by `atNotFound` method is response with http code 404.  
Note that only the most recently registered PageBuilder object is valid.  
- `server` : A reference of ESP8266WebServer (in ESP8266 case) or WebServer (in ESP32 case) object to register the page.

#### `String PageBuilder::build(void)`
Returns the built html string from `const char* mold` that processed *token* by the user *function* of **TokenVT** which code as `{"token",function_name}`. The `build` method handles all *PageElement* objects that a *PageBuilder* contained.

#### `void PageBuilder::cancel(void)`  
Notify to **PageBuilder** that the generated HTML string should not be send.  
**PageBuilder** internally sends generated HTML with http 200 when invoked as *RequestHandler* from *handleClient()*. If the sketch wants to respond only to http response without generating HTML, you need to stop automatic transmission using the cancel method. The following example responds 302 with keep-alive connection. This response does not contain content. So the *Token func* will have the following code.
```c++
  server.sendHeader("Location", redirect-path, true);
  server.sendHeader("Connection", "keep-alive");
  server.send(302, "text/plain", "");
  server.client().stop();
```
The sketch sends an http response in the *Token func* then **PageBuilder** should be stopped 200 response. The **cancel** method notifies this situation to the **PageBuilder**.  This example is in [**SendNakedHttp**](examples/SendNakedHttp/SendNakedHttp.ino).
```c++
  String tokenFunc(PageArgument& args);

  ESP8266WebServer	server;
  PageElement elm("{{RES}}", { {"RES", tokenFunc} });
  PageBuilder page("/", { elm });

  String tokenFunc(PageArgument& args) {
    server.sendHeader("Location", redirect-path, true);
    server.sendHeader("Connection", "keep-alive");
    server.send(302, "text/plain", "");
    server.client().stop();
    page.cancel();
    return "";
  }
```

#### `void PageBuilder::clearElement(void)`  
Clear enrolled **PageElement** objects in the **PageBuilder**.

#### `void PageBuilder::exitCanHandle(PrepareFuncT prepareFunc)`  
- `prepareFunc` : User function instead of canHandle. This user function would be invoked at all request received.  
```bool prepareFunc(HTTPMethod method, String uri);```  
  - `method` : Same as parameter of PageBuilder constructor, `HTTP_ANY`, `HTTP_GET`, `HTTP_POST`.  
  - `uri` : A URI string at this time.  
  - Return : True if this URI request is processed by this PageBuilder, False if it is ignored.

  **Important notes.** The prepareFunc specified by eixtCanHandled is called twice at one http request. See [Application hints](#application-hints) for details.

#### `void PageBuilder::insert(ESP8266WebServer& server)`<br>`void PageBuilder::insert(WebServer& server)`  
Register the page and starts handling. It has the same effect as `on` method of `ESP8266WebServer` (in ESP8266 case)/`WebServer` (in ESP32 case).
- `server` : A reference of the ESP8266WebServer or the WebServer object to register the page.

#### `void PageBuilder::setUri(const char* uri)`  
Set URI of this page.
- `uri` : A pointer of URI string.

#### `const char* PageBuilder::uri()`  
Get URI of this page.

#### `void PageBuilder::chunked(const TransferEncoding_t chunked)`
Set Transfer-Encoding with chunked, or not.
- `chunked` : Enumeration type for the transfer-encoding.

#### `void PageBuilder::reserve(size_t size)`
Set buffer size for reserved content building buffer.
- `size` : Reservation size. If you do not specify a reserved buffer size by this function, the buffer for the build function will not be reserved. As a result, memory insufficient is likely to occur due to fragmentation.

### PageElement methods

#### `const char* PageElement::mold()`  
Get mold string in the PageElement.

#### `String PageElement::build()`  
Returns the HTML element string from `const char* mold` that processed *token* by the user *function* of **TokenVT**.

#### `void PageElement::setMold(const char* mold)`  
Sets the source HTML element string.

#### `void PageElement::addToken(String token, HandleFuncT handler)`  
Add the source HTML element string.

## Application hints<br>to reducing the memory for the HTML source

A usual way, the sketch needs to statically prepare the PageElement object for each element of the web page, so assigning the web contents constructed by multi-page with `static const char*` (including PROGMEM) strangles the heap area.  
However, if the sketch can dynamically create a corresponding page at the time of receiving an HTTP request, you can reduce the number of PageBuilder instances and PageElement instances.
By using **setMold** and **addToken** method of the PegeElement class, the sketch can construct the multiple pages of web content with just one PageBuilder object and a PageElement object.  

### Which method is first called from the WebServer when the URL is requested?

In the first place, the request handler described in the **ESP8266WerbServer::on** (or **WebServer::on**) method would be registered as the *RequestHandler* class. The *RequestHandler* has the **canHandle** method which purpose is to determine if the handler corresponds to the requested URI. **ESP8266WebServer::handleClient** (or **WebServer::handleClient**) method uses the **canHandle** method of the *RequestHandler* class for each URI request to determine the handler which should be invoked in all registered handlers. Which means that the **canHandle** method is the first called, and the **PageBuilder** has the hook way for the this.

### Handling by a single PageBuilder object for all URI requests.  
Using that hook way the sketch can aggregate all URI requests into a single PageBuilder object. The **exitCanHandle** method of PageBuilder specifies the user function to be called which is instead of the **canHandle** method. That user function overrides the canHandle method.  

### The function by exitCanHandle.  
Declaration of the function.
```c++
bool func(HTTPMethod method, String uri);
```
- `method` : Same as parameter of PageBuilder constructor, `HTTP_ANY`, `HTTP_GET`, `HTTP_POST`.  
- `uri` : A URI string at this time.  
- Return : True if this URI request is processed by this PageBuilder, False if it is ignored.

Generally, the logic of the function to implementation is the follows.  

a. Analysis of URI and generation of PageElement object for that page.  
b. Setting the HTML mold of that page by [setMold](#void-pageelementsetmoldconst-char-mold) method.  
c. Registering the *token function* included in the mold by [addToken](#void-pageelementaddtokenstring-token-handlefunct-handler) method.  
d. Action to ignore if the same URI of an already generated page is requested.  

The function would be called twice at one http request. The cause is the internal logic of ESP8266WebServer (Relating to URI handler detection and URL parameter parsing), so the function specified by exitCanHandle needs to ignore the second call.

### An example using this way.  
[DynamicPage.ino](examples/DynamicPage/DynamicPage.ino)


## Change log

#### [1.3.3] 2019-04-13
- Fixed loss of built content when HTML is large.

#### [1.3.2] 2019-02-07
- Supports AutoConnect v0.9.7.

#### [1.3.1] 2019-02-04
- Fixed the captive portal not appear.

#### [1.3.0] 2019-01-29
- Adds building buffer reservation option to avoid built content lost.

#### [1.2.3] 2019-01-22
- Fix leaking memory.
- Add [ArduBadge](https://www.ardu-badge.com/PageBuilder).

#### [1.2.2] 2018-12-26
- Fix losing built content.
 
#### [1.2.1] 2018-12-18
- Fixed 4-byte alignment of mold string in flash.

#### [1.2.0] 2018-12-01
- Fixed incomplete transmission of large HTML string. Chunked-encoding has been implemented accordingly.
- Fix the destructor PageBuilder can be inherited.

#### [1.1.0] 2018-08-21
- Supports ESP32 platform depends on the **WebServer** class.  
A stable version of [arduino-esp32 core](https://github.com/espressif/arduino-esp32/releases/latest) version 1.0.0 or higher is required.

#### [1.0.1] 2018-03-19
- Fix WebELD example, no library change.

#### [1.0.0] 2018-02-17
- Supports **cancel** method in PageBuilder class.
- Supports **setMold** method in PageElement class.
- Supports **addToken** method in PageElement class.

#### [0.93] 2017-12-19
- Supports external file on SPIFFS for PageElement as HTML source data.

#### [0.92] 2017-12-07
- A minor fixes.

#### [0.91] 2017-12-01
- Supports **atNotFound** method in PageBuilder class.

#### [0.9] 2017-11-29
- Release candidate.

## License

The PseudoPWM class is licensed under the [MIT License](LICENSE.md).  
Copyright &copy; 2018-2019 hieromon@gmail.com
