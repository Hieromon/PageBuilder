/**
 *  Declaration of PaguBuilder class and accompanying PageElement, PageArgument class.
 *  @file PageBuilder.h
 *  @author hieromon@gmail.com
 *  @version  1.2.0
 *  @date 2018-12-01
 *  @copyright  MIT license.
 */

#ifndef _PAGEBUILDER_H
#define _PAGEBUILDER_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <functional>
#include <vector>
#include <memory>
#include <Stream.h>
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
using WebServerClass = ESP8266WebServer;
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
using WebServerClass = WebServer;
#endif

// Uncomment the following PB_DEBUG to enable debug output.
#define PB_DEBUG

// Debug output destination can be defined externally with PB_DEBUG_PORT
#ifndef PB_DEBUG_PORT
#define PB_DEBUG_PORT Serial
#endif // !PB_DEBUG_PORT
#ifdef PB_DEBUG
#define PB_DBG(...) do {PB_DEBUG_PORT.print("[PB] "); PB_DEBUG_PORT.printf( __VA_ARGS__ );} while (0)
#else
#define PB_DBG(...)
#endif // !PB_DEBUG

#define PAGEELEMENT_FILE  "file:"

/** HTTP get or post method argument pair structure. */
typedef struct _RequestArgumentS {
  String  _key;     /**< Parameter name */
  String  _value;   /**< Parameter value */
} RequestArgument;

/** Transfer encoding type with ESP8266WebSever::sendcontent */
typedef enum {
  PB_Chunked,          /**< Use chunked transfer */
  PB_Through,          /**< Specifiy content length */
  PB_Auto              /**< Dynamically change the transfer encoding according to the content length. */
} TransferEncoding_t;

/**
 *  Stores the uri parameter or POST argument that occurred in the http request.
 */
class PageArgument {
 public:
  PageArgument() : _arguments(nullptr) {}
  PageArgument(String key, String value) : _arguments(nullptr) { push(key, value); }
  ~PageArgument() {}
  String  arg(String name);
  String  arg(int i);
  String  argName(int i);
  int   args();
  size_t  size() { return (size_t)args(); }
  bool  hasArg(String name);
  void  push(String key, String value);

 protected:
  /** RequestArgument element list structure */
  typedef struct _RequestArgumentSL {
    std::unique_ptr<RequestArgument>  _argument;    /**< A unique pointer to a RequestArgument */
    std::unique_ptr<_RequestArgumentSL> _next;      /**< Pointer to the next element */
  } RequestArgumentSL;

  /** Root element of the list of RequestArgument */
  std::unique_ptr<RequestArgumentSL>  _arguments;

 private:
  RequestArgument*  item(int index);
};

/** Wrapper type definition of handler function to handle token. */
typedef std::function<String(PageArgument&)>  HandleFuncT;

/** Structure with correspondence of token and handler defined in PageElement. */
typedef struct {
  String        _token;     /**< Token string */
  HandleFuncT   _builder;   /**< correspondence handler function */
} TokenSourceST;

/** TokenSourceST linear container type.
 *  Elements of tokens and handlers usually contained in PageElement are 
 *  managed linearly as a container using std::vector.
 */
typedef std::vector<TokenSourceST>  TokenVT;

/**
 *  PageElement class have parts of a HTML page and the components of the part 
 *  to be processed at specified position in the HTML as the token.
 */
class PageElement {
 public:
  PageElement() : _mold(nullptr) {}
  explicit PageElement(const char* mold) : _mold(mold), _source(std::vector<TokenSourceST>()) {}
  PageElement(const char* mold, TokenVT source) : _mold(mold), _source(source) {}
  virtual ~PageElement();

  const char*   mold() { return _mold; }
  TokenVT       source() { return _source; }
  String        build();
  static String build(const char* mold, TokenVT tokenSource, PageArgument& args);
  void          setMold(const char* mold) { _mold = mold; }
  void          addToken(String token, HandleFuncT handler);

 protected:
  const char* _mold;    //*< A pointer to HTML model string(char array). */
  TokenVT     _source;  //*< Container of processable token and handler function. */
};

/**
 * String stream class for http transfer-encoding:chunked
 */
class PageStream : public Stream {
 public:
  explicit PageStream(String& content) : _content(content), _pos(0) {}
  virtual  ~PageStream() {}
  virtual int   available() { return _content.length() - _pos; }
  virtual int   read() { return _pos < _content.length() ? _content[_pos++] : -1; }
  virtual int   peek() { return _pos < _content.length() ? _content[_pos] : -1; }
  virtual void  flush() {}
  virtual const String name() const { return ""; }
  virtual const size_t size() const { return _content.length(); }
  virtual size_t write(uint8_t c) { _content += static_cast<char>(c); return 1; }
  virtual size_t readBytes(char *buffer, size_t length);

 private:
  String& _content;
  size_t  _pos;
};

/** PageElement container.
 *  The PageBuilder class treats multiple PageElements constituting a html 
 *  page as a reference container as std::reference_wrapper.*/
typedef std::vector<std::reference_wrapper<PageElement>>  PageElementVT;

/** The user own function for the current uri handling preparation. */
typedef std::function<bool(HTTPMethod, String)> PrepareFuncT;

/**
 *  PageBuilder class is to make easy to assemble and output html stream 
 *  of web page. The page builder class includes the uri of the page, 
 *  the PageElement indicating the HTML model constituting the page, and 
 *  the on handler called from the ESP8266WebServer class.
 *  This class inherits the RequestHandler class of the ESP8266WebServer class.
 */
class PageBuilder : public RequestHandler {
 public:
  PageBuilder() : _uri(nullptr), _method(HTTP_ANY), _noCache(true), _cancel(false), _sendEnc(PB_Auto), _server(nullptr), _canHandle(nullptr) {}
  explicit PageBuilder(PageElementVT element, HTTPMethod method = HTTP_ANY, bool noCache = true, bool cancel = false, TransferEncoding_t chunked = PB_Auto) :
    _uri(nullptr),
    _element(element),
    _method(method),
    _noCache(noCache),
    _cancel(cancel),
    _sendEnc(chunked),
    _server(nullptr),
    _canHandle(nullptr) {}
  PageBuilder(const char* uri, PageElementVT element, HTTPMethod method = HTTP_ANY, bool noCache = true, bool cancel = false, TransferEncoding_t chunked = PB_Auto) :
    _uri(uri),
    _element(element),
    _method(method),
    _noCache(noCache),
    _cancel(cancel),
    _sendEnc(chunked),
    _server(nullptr),
    _canHandle(nullptr) {}

  virtual ~PageBuilder() { _uri = nullptr; _server = nullptr; clearElement(); }

  bool canHandle(HTTPMethod requestMethod, String requestUri) override;
  bool canUpload(String requestUri) override;
  bool handle(WebServerClass& server, HTTPMethod requestMethod, String requestUri) override;

  void setUri(const char* uri) { _uri = uri; }
  const char* uri() { return _uri; }
  void insert(WebServerClass& server) { server.addHandler(this); }
  void addElement(PageElement& element) { _element.push_back(element); }
  void clearElement();
  static void sendNocacheHeader(WebServerClass& server);
  String build(void);
  String build(PageArgument& args);
  void atNotFound(WebServerClass& server);
  void exit404(void);
  void exitCanHandle(PrepareFuncT prepareFunc) { _canHandle = prepareFunc; }
  void cancel() { _cancel = true; }
  void chunked(const TransferEncoding_t devide) { _sendEnc = devide; }

 protected:
  const char*   _uri;       /**< uri of this page */
  PageElementVT _element;   /**< PageElement container */
  HTTPMethod    _method;    /**< Method of http request to which this page applies. */

 private:
  bool  _sink(int code, WebServerClass& server);  /**< send Content */
  bool  _noCache;                 /**< A flag for must-revalidate cache control response */
  bool  _cancel;                  /**< Automatic send cancellation */
  TransferEncoding_t  _sendEnc;   /**< Use chunked sending */
  WebServerClass* _server;
  PrepareFuncT  _canHandle;       /**< 'canHanlde' user owned function */
};

#endif
