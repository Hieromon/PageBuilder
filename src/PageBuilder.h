/**
 *  Declaration of PageBuilder class and accompanying PageElement, PageArgument class.
 *  @file PageBuilder.h
 *  @author hieromon@gmail.com
 *  @version  1.4.2
 *  @date 2020-05-25
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
// #define PB_DEBUG

// SPIFFS has deprecated on EP8266 core. This flag indicates that
// the migration to LittleFS has not completed.
// Uncomment the following PB_USE_SPIFFS to enable SPIFFS.
// #define PB_USE_SPIFFS

// Debug output destination can be defined externally with PB_DEBUG_PORT
#ifndef PB_DEBUG_PORT
#define PB_DEBUG_PORT Serial
#endif // !PB_DEBUG_PORT
#ifdef PB_DEBUG
#define PB_DBG_DUMB(fmt, ...) do {PB_DEBUG_PORT.printf(PSTR(fmt), ## __VA_ARGS__ );} while (0)
#define PB_DBG(fmt, ...) do {PB_DEBUG_PORT.printf(PSTR("[PB] " fmt), ## __VA_ARGS__ );} while (0)
#else
#define PB_DBG_DUMB(...) do {} while (0)
#define PB_DBG(...) do {} while (0)
#endif // !PB_DEBUG

#define PAGEELEMENT_FILE  "file:"

/** HTTP get or post method argument pair structure. */
typedef struct _RequestArgumentS {
  String  _key;     /**< Parameter name */
  String  _value;   /**< Parameter value */
} RequestArgument;

/** Transfer encoding type with ESP8266WebSever::sendcontent */
typedef enum {
  PB_Auto,             /**< Use chunked transfer */
  PB_ByteStream,       /**< Specify content length */
  PB_Chunk             /**< Dynamically change the transfer encoding according to the content length. */
} TransferEncoding_t;

/**
 *  Stores the uri parameter or POST argument that occurred in the http request.
 */
class PageArgument {
 public:
  PageArgument() : _arguments(nullptr) {}
  PageArgument(const String& key, const String& value) : _arguments(nullptr) { push(key, value); }
  ~PageArgument() {}
  String  arg(const String& name);
  String  arg(int i);
  String  argName(int i);
  int   args(void);
  size_t  size(void) { return (size_t)args(); }
  bool  hasArg(const String& name);
  void  push(const String& key, const String& value);

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

  const char*   mold(void) { return _mold; }
  TokenVT       source(void) { return _source; }
  String        build(void);
  static String build(const char* mold, TokenVT tokenSource, PageArgument& args);
  void          setMold(const char* mold) { _mold = mold; }
  void          addToken(const String& token, HandleFuncT handler);

 protected:
  const char* _mold;    //*< A pointer to HTML model string(char array). */
  TokenVT     _source;  //*< Container of processable token and handler function. */
};

/**
 *  PageElement container.
 *  The PageBuilder class treats multiple PageElements constituting a html 
 *  page as a reference container as std::reference_wrapper.*/
typedef std::vector<std::reference_wrapper<PageElement>>  PageElementVT;

/** The type of user-owned function for preparing the handling of current URI. */
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
  PageBuilder() : _method(HTTP_ANY), _upload(nullptr), _noCache(true), _cancel(false), _sendEnc(PB_Auto), _rSize(0), _server(nullptr), _canHandle(nullptr) {}
  explicit PageBuilder(PageElementVT element, HTTPMethod method = HTTP_ANY, bool noCache = true, bool cancel = false, TransferEncoding_t chunked = PB_Auto) :
    _element(element),
    _method(method),
    _upload(nullptr),
    _noCache(noCache),
    _cancel(cancel),
    _sendEnc(chunked),
    _rSize(0),
    _server(nullptr),
    _canHandle(nullptr) {}
  PageBuilder(const char* uri, PageElementVT element, HTTPMethod method = HTTP_ANY, bool noCache = true, bool cancel = false, TransferEncoding_t chunked = PB_Auto) :
    _uri(String(uri)),
    _element(element),
    _method(method),
    _upload(nullptr),
    _noCache(noCache),
    _cancel(cancel),
    _sendEnc(chunked),
    _rSize(0),
    _server(nullptr),
    _canHandle(nullptr) {}

  virtual ~PageBuilder() { _server = nullptr; clearElement(); _username.reset(); _password.reset(); _realm.reset(); }

  /** The type of user-owned function for uploading. */
  typedef std::function<void(const String&, const HTTPUpload&)> UploadFuncT;

  virtual bool canHandle(HTTPMethod requestMethod, String requestUri) override;
  virtual bool canUpload(String uri) override;
  bool handle(WebServerClass& server, HTTPMethod requestMethod, String requestUri) override;
  virtual void upload(WebServerClass& server, String requestUri, HTTPUpload& upload) override;

  void setUri(const char* uri) { _uri = String(uri); }
  const char* uri() { return _uri.c_str(); }
  void insert(WebServerClass& server) { server.addHandler(this); }
  void addElement(PageElement& element) { _element.push_back(element); }
  void clearElement();
  static void sendNocacheHeader(WebServerClass& server);
  String build(void);
  String build(PageArgument& args);
  void atNotFound(WebServerClass& server);
  void exit404(void);
  void exitCanHandle(PrepareFuncT prepareFunc) { _canHandle = prepareFunc; }
  virtual void onUpload(UploadFuncT uploadFunc) { _upload = uploadFunc; }
  void cancel() { _cancel = true; }
  void chunked(const TransferEncoding_t devid) { _sendEnc = devid; }
  void reserve(size_t size) { _rSize = size; }
  void authentication(const char* username, const char* password, HTTPAuthMethod mode = BASIC_AUTH, const char* realm = NULL, const String& authFail = String(""));

 protected:
  String        _uri;       /**< uri of this page */
  PageElementVT _element;   /**< PageElement container */
  HTTPMethod    _method;    /**< Method of http request to which this page applies. */
  UploadFuncT   _upload;    /**< 'upload' user owned function */

 private:
  bool    _sink(int code, WebServerClass& server);  /**< send Content */
  char*   _digestKey(const char* key);  /**< save authentication parameter */
  bool    _noCache;               /**< A flag for must-revalidate cache control response */
  bool    _cancel;                /**< Automatic send cancellation */
  TransferEncoding_t  _sendEnc;   /**< Use chunked sending */
  HTTPAuthMethod  _auth;          /**< Authentication method */
  size_t  _rSize;                 /**< Reserved buffer size for content building */
  WebServerClass* _server;
  PrepareFuncT  _canHandle;       /**< 'canHandle' user owned function */
  std::unique_ptr<char[]> _username;  /**< A user name for authentication */
  std::unique_ptr<char[]> _password;  /**< A password for authentication */
  std::unique_ptr<char[]> _realm;     /**< realm for DIGEST */
  String  _fails;                /**< A message for authentication failed */

  static const String _emptyString;
};

#endif
