/**
 * Declaration of PageBuilder class and accompanying PageElement, PageArgument class.
 * @file PageBuilder.h
 * @author hieromon@gmail.com
 * @version  1.5.3
 * @date 2022-03-02
 * @copyright  MIT license.
 */

#ifndef _PAGEBUILDER_H_
#define _PAGEBUILDER_H_

#include <tuple>
#include <type_traits>
#include <functional>
#include <forward_list>
#include <stack>
#include <vector>
#include <iterator>
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
using WebServer = ESP8266WebServer;
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif

// Uncomment the following PB_DEBUG to enable debug output.
// #define PB_DEBUG

// Debug output destination can be defined externally with PB_DEBUG_PORT
#ifndef PB_DEBUG_PORT
#define PB_DEBUG_PORT Serial
#endif // !PB_DEBUG_PORT
#ifdef PB_DEBUG
#define PB_DBG_DUMB(fmt, ...) do {PB_DEBUG_PORT.printf_P((PGM_P)PSTR(fmt), ## __VA_ARGS__ );} while (0)
#define PB_DBG(fmt, ...) do {PB_DEBUG_PORT.printf_P((PGM_P)PSTR("[PB] " fmt), ## __VA_ARGS__ );} while (0)
#else
#define PB_DBG_DUMB(...) do {(void)0;} while (0)
#define PB_DBG(...) do {(void)0;} while (0)
#endif // !PB_DEBUG

// The PB_USE_SPIFFS and PB_USE_LITTLEFS macros declare which filesystem
// to apply. Their definitions are contradictory to each other and you
// cannot activate both at the same time.
//#define PB_USE_SPIFFS
//#define PB_USE_LITTLEFS
// Each platform supported by PageBuilder has a default file system,
// which is LittleFS for ESP8266 and SPIFFS for ESP32. Neither PB_USE_SPIFFS
// nor PB_USE_LITTLE_FS needs to be explicitly defined as long as you use
// the default file system. The default file system for each platform is assumed.
// SPIFFS has deprecated on EP8266 core. PB_USE_SPIFFS flag indicates
// that the migration to LittleFS has not completed.

// Globally deploy the applicable file system classes and instances.
// The type of the applicable file system to be used is switched
// according to the AC_USE_SPIFFS and AC_USE_LITTLEFS definition.
// The file system indicator to apply is expanded by the macro definition,
// AUTOCONNECT_APPLIED_FILECLASS is assigned the class name,
// and AUTOCONNECT_APPLIED_FILESYSTEM is assigned the global instance name.
#if defined(ARDUINO_ARCH_ESP8266)
#define PB_DEFAULT_FILESYSTEM 2
#elif defined(ARDUINO_ARCH_ESP32)
#define PB_DEFAULT_FILESYSTEM 1
#endif

#if !defined(PB_USE_SPIFFS) && !defined(PB_USE_LITTLEFS)
#define PB_USE_FILESYSTEM PB_DEFAULT_FILESYSTEM
#elif defined(PB_USE_SPIFFS)
#define PB_USE_FILESYSTEM 1
#elif defined(PB_USE_LITTLEFS)
#define PB_USE_FILESYSTEM 2
#endif

// Note: If LittleFS.h becomes Not Found in PlatformIO, try specifying
// lib_ldf_mode=deep with platformio.ini. Due to the deep nesting by
// preprocessor instructions, the include file cannot be detected by the
// chain mode (nested include search) of PlatformIO's dependent library
// search.
#ifdef ARDUINO_ARCH_ESP8266
#define PB_APPLIED_FILECLASS              fs::FS
#endif
#if PB_USE_FILESYSTEM == 1
#include <FS.h>
#define PB_APPLIED_FILESYSTEM             SPIFFS
#ifdef ARDUINO_ARCH_ESP32
#include <SPIFFS.h>
#define PB_APPLIED_FILECLASS              fs::SPIFFSFS
#endif
#elif PB_USE_FILESYSTEM == 2
#include <LittleFS.h>
#define PB_APPLIED_FILESYSTEM             LittleFS
#ifdef ARDUINO_ARCH_ESP32
#define PB_APPLIED_FILECLASS              fs::LittleFSFS

// With ESP32 platform core version less 2.0, reverts the LittleFS class and
// the exported instance to the ordinary LittleFS_esp32 library owns.
#if !defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR<2
#undef PB_APPLIED_FILESYSTEM
#define PB_APPLIED_FILESYSTEM             LITTLEFS
#undef PB_APPLIED_FILECLASS
#define PB_APPLIED_FILECLASS              fs::LITTLEFSFS
#endif
#endif
#endif

// The length of one content block is predefined and determined at compilation.
// The block length affects how the content is sent. If the HTML content
// of the generated page exceeds this size, PageBuilder will send it
// with chunks.
#ifndef PAGEBUILDER_CONTENTBLOCK_SIZE
#define PAGEBUILDER_CONTENTBLOCK_SIZE     1270
#endif

// Delimiter character to appear the token in the page element
// It must be given as a pair of OPEN and CLOSE.
#ifndef PAGEBUILDER_TOKENDELIMITER_OPEN
#define PAGEBUILDER_TOKENDELIMITER_OPEN   '{'
#endif
#ifndef PAGEBUILDER_TOKENDELIMITER_CLOSE
#define PAGEBUILDER_TOKENDELIMITER_CLOSE  '}'
#endif

#ifndef PAGEELEMENT_TOKENIDENTIFIER_FILE
#define PAGEELEMENT_TOKENIDENTIFIER_FILE  "file:"
#endif

/**
 * Container for HTTP request parameters from the current client of the
 * ESP8266WebServer. It provides access methods equivalent to the HTTP
 * query parameters access provided by the ESP8266WebServer.
 */
class PageArgument {
 public:
  PageArgument() {}
  PageArgument(const String& key, const String& value) { push(key, value); }
  ~PageArgument() {}
  String  arg(const char* name) const { return arg(String(name)); }
  String  arg(const String& name) const;
  String  arg(int i) const;
  String  argName(int i) const;
  size_t  args(void) const { return size(); }
  size_t  size(void) const { return std::distance(_arguments.begin(), _arguments.end()); }
  bool  hasArg(const char* name) const { return hasArg(String(name)); }
  bool  hasArg(const String& name) const { return (arg(name) != _nullString); }
  void  push(const String& name, const String& value);

 protected:
  // RequestArgument element structure
  typedef struct _RequestArguemnt {
    String  name;
    String  value;
  } _RequestArgumentST;

  // Root element of the list of RequestArgument
  using _RequestArgumentLT = std::forward_list<_RequestArgumentST>;
  _RequestArgumentLT _arguments;

 private:
  const _RequestArgumentST& _item(int i) const;
  static const String  _nullString;
};

// Wrapper type definition of handler function to handle token.
typedef std::function<String(PageArgument&)>  HandleFuncT;

/**
 * TokenSource manages the replacement source for PageElement.
 * The replacement source is defined as a token together with the
 * handler. It also supports proper reading depending on the distinction
 * between the type of PageElement and the storage where the token is
 * placed (it is a heap area or a text block that is a PROGMEM attribute).
 */
class TokenSource {
 public:
  // Storage identifier of the token placed.
  enum STORAGE_CLASS_t {
    HEAP,         /**< For const char */
    TEXT,         /**< For __FlashStringHelper */
    STRING,       /**< For String */
    FILE          /**< For File */
  };

  TokenSource() : token(nullptr) {}
  TokenSource(const char* token, HandleFuncT builder) : token(token), builder(builder), _storage(STORAGE_CLASS_t::HEAP) {}
  TokenSource(const __FlashStringHelper* token, HandleFuncT builder) : token(reinterpret_cast<PGM_P>(token)), builder(builder), _storage(STORAGE_CLASS_t::TEXT) {}
  virtual ~TokenSource() {}
  bool  match(const char* key) const {
    return !(_storage == HEAP ? strcmp(key, token) : strcmp_P(key, reinterpret_cast<const char*>(token)));
  }

  PGM_P         token;                /**< a token */
  HandleFuncT   builder;              /**< User defined handler to replace a token */

 private:
  STORAGE_CLASS_t  _storage;          /**< Explicit distinction of storage where token is placed */
};

/**
 * TokenSourceST linear container type.
 * Elements of tokens and handlers usually contained in PageElement are
 * managed linearly as a container using std::vector.
 */
using TokenVT = std::vector<TokenSource>;

/**
 * A container of the mold as a template that is the basis of the actual
 * HTML and tokens that are replaced during processing.
 */
class PageElement {
 public:
  PageElement() {}
  explicit PageElement(const char* mold) : _sources(TokenVT()) { setMold(mold); }
  explicit PageElement(const __FlashStringHelper* mold) : _sources(TokenVT()) { setMold(mold); }
  PageElement(const char* mold, const TokenVT& sources) : _sources(sources) { setMold(mold); }
  PageElement(const __FlashStringHelper* mold, const TokenVT& sources) : _sources(sources) { setMold(mold); }
  ~PageElement() {}
  void  addToken(const char* token, HandleFuncT handler);
  void  addToken(const __FlashStringHelper* token, HandleFuncT handler);
  size_t  build(String& buffer);
  size_t  build(String& buffer, PageArgument& args);
  size_t  build(char* buffer, size_t length, PageArgument& args);
  size_t  getApproxSize(void) const { return _approxSize; }
  PGM_P mold(void) const { return _mold; }
  void  reserve(const size_t reserveSize = 0) { _reserveSize = reserveSize; }
  void  rewind(void);
  void  setMold(const char* mold);
  void  setMold(const __FlashStringHelper* mold);

 protected:
  // Saves the lexical scan position when generating page elements from
  // the mold and tokens. _LexicalIndexST structure is pushed onto the
  // stack each time a token appearance during the mold scanning.
  typedef struct {
    PGM_P _p;                         /**< Position of the mold or token lexical during scanning */
    unsigned int  _s;                 /**< Read offset in the string replaced from the token */
    String  _fillin;                  /**< String with a token replaced */
    std::shared_ptr<File> _file;      /**< File for file: mold */
    TokenSource::STORAGE_CLASS_t  _storage; /**< Distinct class of storage to be scanned */
  } _LexicalIndexST;

  char    _contextRead(PageArgument& args); /**< Common lexical reader */
  String  _extractToken(void);        /**< Read as context while replacing the tokens */
  char    _read(void);                /**< Common lexical reader */

  char    _sub_c;                     /**< Subsequent characters at a token delimiter appearance */
  size_t  _reserveSize = 0;           /**< Size when reserving read buffer as context */
  size_t  _approxSize;                /**< Approximate length of context without tokens */

  PGM_P   _mold;                      /**< mold */
  TokenVT _sources;                   /**< Array of tokens */
 
 private:
  TokenSource::STORAGE_CLASS_t  _storage;     /**< Storage class of the mold */
  _LexicalIndexST               _raw;         /**< Position of lexical currently being scanned */
  std::stack<_LexicalIndexST>   _indexStack;  /**< Stack for the mold scanning position save */
  bool    _eoe;                       /**< The element has been read */
};

// The type of user-owned function for preparing the handling of current URI.
typedef std::function<bool(HTTPMethod, String)> PrepareFuncT;

/**
 * The PageBuilder class treats multiple PageElements constituting an
 * HTML page as a reference container of std::reference_wrapper.
 * PageElementVT is an alias for the std::vector array that contains
 * multiple PageElements, allowing Sketch to use the PageElements declaration.
 */
using PageElementVT = std::vector<std::reference_wrapper<PageElement>>;

/**
 *  Provides a namespace that is local to PageBuilder's internal scope.
 *  It has type qualifiers used by PageBuilder.*/
namespace PageBuilderUtil {
  // TypeOfArgument as a template
  // Get the type of arguments from a member.
  template<typename T>
  struct TypeOfArgument;

  template<typename T, typename U, typename... V>
  struct TypeOfArgument<U(T::*)(V...)> {
    template<size_t i>
    struct arg {
      typedef typename std::tuple_element<i, std::tuple<V...>>::type  type;
    };
  };

  // Determines the type of the uri argument contained in the member function
  // signature of the RequestHandler class. This redefinition procedure ensures
  // backward compatibility with ESP8266 arduino core 3.0.0 and later.
  // However, it relies solely on the canHandle member function as a criterion
  // and lacks completeness.
  using URI_TYPE_SIGNATURE = std::conditional<
    std::is_lvalue_reference<TypeOfArgument<decltype(&RequestHandler::canHandle)>::arg<1>::type>::value,
    const String&,
    String
  >::type;
};

/**
 * HTML assembly aid.
 * It inherits from RequestHandler and meets the requirements of the
 * response handler for url access for the WebServer class.
 */
class PageBuilder : public RequestHandler {
 public:
  // Identifier of transfer coding method with sending HTML
  enum TransferEncoding_t {
    Auto,         /**< Short HTML is sent once, otherwise chunked */
    ByteStream,   /**< Chunked but not split into each PageElement segment */
    Chunked,      /**< Chunked with each PageElement segment */
    Compress,     /**< Not suppoted */
    Deflate,      /**< Not suppoted */
    Gzip,         /**< Not suppoted */
    Identity      /**< Not suppoted */
  };

  // The type of user-owned function for uploading.
  typedef std::function<void(const String&, const HTTPUpload&)> UploadFuncT;

  PageBuilder();
  explicit PageBuilder(PageElementVT elements, HTTPMethod method = HTTP_ANY, bool noCache = true, bool cancel = false, TransferEncoding_t chunked = Auto);
  PageBuilder(const char* uri, PageElementVT elements, HTTPMethod method = HTTP_ANY, bool noCache = true, bool cancel = false, TransferEncoding_t chunked = Auto);
  virtual ~PageBuilder() {}
  void  addElement(PageElement& element) { _elements.push_back(element); }
  void  atNotFound(WebServer& server);
  void  authentication(const char* username, const char* password, const HTTPAuthMethod scheme = HTTPAuthMethod::BASIC_AUTH, const char* realm = NULL, const String& authFail = String(""));
  size_t  build(String& content);
  size_t  build(String& content, PageArgument& args);
  void  cancel(const bool cancelation = true) { _cancel = cancelation; }
  virtual bool  canHandle(HTTPMethod requestMethod, PageBuilderUtil::URI_TYPE_SIGNATURE requestUri) override;
  virtual bool  canUpload(PageBuilderUtil::URI_TYPE_SIGNATURE uri) override;
  void  clearElements(void);
  void  exitCanHandle(PrepareFuncT prepareFunc) { _canHandle = prepareFunc; }
  bool  handle(WebServer& server, HTTPMethod requestMethod, PageBuilderUtil::URI_TYPE_SIGNATURE requestUri) override;
  void  insert(WebServer& server) { server.addHandler(this); }
  virtual void  onUpload(UploadFuncT uploadFunc) { _upload = uploadFunc; }
  void  reserve(const size_t reserveSize) { _reserveSize = reserveSize; }
  void  setNoCache(const bool noCache) { _noCache = noCache; }
  void  setUri(const char* uri) { _uri = String(uri); }
  void  transferEncoding(const TransferEncoding_t encoding) { _enc = encoding; }
  virtual void  upload(WebServer& server, PageBuilderUtil::URI_TYPE_SIGNATURE requestUri, HTTPUpload& upload) override;
  const char* uri(void) const { return _uri.c_str(); }

 protected:
  String        _uri;                 /**< Requested URI */
  PageElementVT _elements;            /**< Array of PageElements */
  HTTPMethod    _method;              /**< Requested HTTP method */
  UploadFuncT   _upload;              /**< Upload handler */

 private:
  size_t  _getApproxSize(void) const; /**< Calculate an approximate generating size o the HTML */
  void    _handle(int code, WebServer& server); /**< URL request handler */

  bool          _noCache;             /**< Need to send the no-cache header */
  bool          _cancel;              /**< Cancel to send content */
  TransferEncoding_t  _enc;           /**< Transfer encoding for this sending */
  HTTPAuthMethod  _auth;              /**< HTTP authentication scheme */
  size_t        _reserveSize = 0;     /**< Buffer reservation size */
  WebServer*    _server = nullptr;    /**< An instance of the WebServer that owns this request handler */
  PrepareFuncT  _canHandle;           /**< An exit of canHandle invoke */
  String        _username;            /**< Username for an auth */
  String        _password;            /**< Password for an auth */
  String        _realm;               /**< REALM for the current auth */
  String        _fails;               /**< Message for fails with authentication */

  // A set of fixed directives just for sending No-cache headers
  typedef struct {
    PGM_P name;
    PGM_P value;
  } _httpHeaderConstST;
  static const _httpHeaderConstST  _headersNocache[] PROGMEM;
};

#endif  // !_PAGEBUILDER_H_
