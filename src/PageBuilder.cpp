/**
 *  PageBuilder class implementation which includes the class methods of 
 *  PageElement.
 *  @file   PageBuilder.cpp
 *  @author hieromon@gmail.com
 *  @version    1.5.1
 *  @date   2021-11-14
 *  @copyright  MIT license.
 */

#include <Arduino.h>
#include "PageBuilder.h"
#include "PageStream.h"

// Determining the valid file system currently configured
namespace PageBuilderFS { PB_APPLIED_FILECLASS& flash = PB_APPLIED_FILESYSTEM; };

// Allocate static null string
const String PageArgument::_nullString = String();

// A set of fixed directives just for sending No-cache headers
const PageBuilder::_httpHeaderConstST  PageBuilder::_headersNocache[] PROGMEM = {
  { "Cache-Control", "no-cache,no-store,must-revalidate" },
  { "Pragma", "nocache" },
  { "Expires", "-1" }
};

/**
 * get request argument value, specifies an i as index to get POST body.
 * @param   i Index of the arguments
 * @return  Requested value
 */
String PageArgument::arg(int i) const {
  if (i >= 0 && i < static_cast<int>(size())) {
    return _item(i).value;
  }
  return _nullString;
}

/**
 * Get request argument value, use arg("plain") to get POST body
 * @param   name  Name of requested query parameter
 * @return  requested value
 */
String PageArgument::arg(const String& name) const {
  for (auto& item : _arguments) {
    if (item.name.equalsIgnoreCase(name))
      return item.value;
  }
  return _nullString;
}
/**
 * Get request argument name, specifies an i as index to get argument
 * name contained POST body.
 * @param i Index of the arguments
 * @return  Requested argument name, the null string as the specified
 *          index is out of range.
 */
String PageArgument::argName(int i) const {
  if (i >= 0 && i < static_cast<int>(size())) {
    return _item(i).name;
  }
  return _nullString;
}

/**
 * Register new argument with the name and value.
 * @param   name
 * @param   value
 */
void PageArgument::push(const String& name, const String& value) {
  _arguments.push_front({ name, value });
}

/**
 * Get a set of name and value, returns as _RequestArgumentST.
 * @param   i Index of registered arguments array
 * @return  Reference of a _RequestArgumentST
 */
const PageArgument::_RequestArgumentST& PageArgument::_item(int i) const {
  _RequestArgumentLT::const_iterator it = _arguments.cbegin();
  std::advance(it, i);
  return *it;
}

/**
 * Add a PageElement token, with registering the correspondence handler.
 * It is an interface for tokens placed in the heap.
 * @param   token   const char*
 */
void PageElement::addToken(const char* token, HandleFuncT handler) {
  TokenSource source(token, handler);
  _sources.push_back(source);
}

/**
 * Add a PageElement token, with registering the correspondence handler.
 * It is an interface for tokens placed in the .irom.text segment.
 * @param   token   const __FlashStringHelper*
 */
void PageElement::addToken(const __FlashStringHelper* token, HandleFuncT handler) {
  TokenSource source(token, handler);
  _sources.push_back(source);
}

/**
 * Construct HTML content, store into the buffer as String.
 * @param   buffer  Reference of the String storage which the HTML is constructed.
 * @return  Size of an actual HTML content.
 */
size_t PageElement::build(String& buffer) {
  PageArgument  args;
  return build(buffer, args);
}

/**
 * Construct an HTML content separated by one element.
 * It will expand the output destination buffer in advance to reserve
 * the capacity to store constructed HTML.
 * @param   buffer  Reference of the String storage which the HTML is constructed.
 * @param   args    Arguments to be passed to the token handler.
 * @return  Size of an actual HTML content.
 */
size_t PageElement::build(String& buffer, PageArgument& args) {
  char    c;
  size_t  wc = 0;
  size_t  rSize = _reserveSize;

  // Expands the output buffer.
  if (!rSize)
    rSize = (getApproxSize() + 32) & (~0xe0);
  buffer.clear();
  if (!buffer.reserve(rSize)) {
    PB_DBG("Element reservation failed, free:%u\n", ESP.getFreeHeap());
  }

  rewind();                 // Reset the scanning position.
  c = _contextRead(args);   // Content construction loop
  while (c) {               // Stops at nul character
    if (buffer.concat(c))
      wc++;
    else {
      PB_DBG("Element building failure\n");
      break;
    }
    c = _contextRead(args);
  }
  return wc;
}

/**
 * Construct a content with subsequently for streaming output.
 * @param   buffer  Output buffer
 * @param   length  Buffer capacity
 * @param   args    Arguments to be passed to the token handler.
 * @return  Size of constructed content
 */
size_t PageElement::build(char* buffer, size_t length, PageArgument& args) {
  size_t  wc = 0;

  while (wc < length) {
    char  c = _contextRead(args);
    if (!c)
      break;
    *(buffer + wc++) = c;
  }
  return wc;
}

/**
 * Read as context while replacing the token contained in the mold with
 * the actual string
 * @param   args    Arguments to be passed to the token handler.
 * @return  A read character
 */
char PageElement::_contextRead(PageArgument& args) {
  char  c = '\0';

  // Reading is invalid if the mold is read to the end and the
  // element has reached the end.
  if (!_eoe) {
    if (_sub_c) {
      // If the token separation is not established after reading the
      // first delimiter, the character is valid as content.
      c = _sub_c;
      _sub_c = '\0';
    }
    else {
      bool  subseq = false;
      do {
        c = _read();
        if (c == PAGEBUILDER_TOKENDELIMITER_OPEN) {
          _sub_c = _read();
          if (_sub_c == PAGEBUILDER_TOKENDELIMITER_OPEN) {
            _sub_c = '\0';
            // here, extract a token from the _mold
            String  token = _extractToken();
            if (token.length()) {
              // here, matches a token
              HandleFuncT exchanger = nullptr;
              for (TokenSource& source : _sources) {
                // Find the token replacement source
                if (source.match(token.c_str())) {
                  exchanger = source.builder;
                  break;
                }
              }
              if (exchanger) {
                // Get token replacement string, extract into the content
                _indexStack.push(_raw);
                _raw._fillin = exchanger(args);
                _raw._storage = TokenSource::STORAGE_CLASS_t::STRING;
                _raw._s = 0;
                _raw._p = nullptr;
                // Read context again due to source changes
                c = _contextRead(args);
              }
            }
            else
              subseq = true;
          }
        }
      } while (subseq);
    }
  }
  _eoe = !(c);
  return c;
}

/**
 * Extract a token
 * @return  String of the token
 */
String PageElement::_extractToken(void) {
  String  token;

  char  c = _read();
  while (c != '\0') {
    if (c == PAGEBUILDER_TOKENDELIMITER_CLOSE) { 
      const char  sub_c = _read();
      if (sub_c == PAGEBUILDER_TOKENDELIMITER_CLOSE || !sub_c)
        break;
      else {
        token += c;
        token += sub_c;
      }
    }
    else
      token += c;
    c = _read();
  }
  PB_DBG_DUMB("%s ", token.c_str());
  return token;
}

/**
 * Reads characters according to the storage class where the source is stored.
 * @return  A read character
 */
char PageElement::_read(void) {
  int c = 0x0;

  if (_raw._storage == TokenSource::STORAGE_CLASS_t::HEAP)
    c = *_raw._p++;
  else if (_raw._storage == TokenSource::STORAGE_CLASS_t::TEXT)
    c = static_cast<char>(pgm_read_byte(_raw._p++));
  else if (_raw._storage == TokenSource::STORAGE_CLASS_t::STRING) {
    if (_raw._s < _raw._fillin.length())
      c = _raw._fillin[_raw._s++];
    else
      // Forcibly release the fill string that has been read.
      _raw._fillin.~String();
  }
  else if (_raw._storage == TokenSource::STORAGE_CLASS_t::FILE) {
    if (!_raw._file) {
      PB_DBG_DUMB("\n");
      File  mf = PageBuilderFS::flash.open(_mold, "r");
      if (mf) {
        PB_DBG("_mold %s opened, ", mf.name());
        _raw._file = std::make_shared<File>(mf);
      }
      else {
        PB_DBG("_mold %s open failed", _mold);
        return '\0';
      }
    }
    if ((c = _raw._file->read()) == -1) {
      _raw._file->close();
      _raw._file.reset();
      c = '\0';
    }
  }

  // Just the eos will not indicate the finished reading.
  if (!c) {
    if (!_indexStack.empty()) {
      // Recovers the last reading address the mold during reading.
      // And returns to the previous reading process.
      _raw = _indexStack.top();
      _indexStack.pop();
      c = _read();
    }
  }
  return static_cast<char>(c);
}

/**
 * Reset the scanning address of the mold,
 * also the token replacement string.
 */
void PageElement::rewind(void) {
  while (!_indexStack.empty())
    _indexStack.pop();
  _raw._storage = _storage;
  _raw._s = 0;
  _raw._p = _mold;
  _sub_c = '\0';
  _eoe = false;
}

/**
 * Save the mold string as a type of PGM_P.
 * @param  mold   const char* mold string
 */
void PageElement::setMold(const char* mold) {
  if (strncmp(mold, PAGEELEMENT_TOKENIDENTIFIER_FILE, strlen(PAGEELEMENT_TOKENIDENTIFIER_FILE))) {
    _mold = mold;
    _storage = TokenSource::HEAP;
    _approxSize = strlen(_mold);
  }
  else {
    _mold = mold + strlen(PAGEELEMENT_TOKENIDENTIFIER_FILE);
    _storage = TokenSource::FILE;
  }
}

/**
 * Save the mold string stored in the .irom.text section.
 * @param   mold   __FlashStringHelper class
 */
void PageElement::setMold(const __FlashStringHelper* mold) {
  _mold = reinterpret_cast<PGM_P>(mold);
  _storage = TokenSource::TEXT;
  _approxSize = strlen_P(_mold);
}

/**
 * Default constructor.
 * Assign a PageBuilder instance with an empty PageElement.
 */
PageBuilder::PageBuilder()
: _elements(PageElementVT())
, _method(HTTP_ANY)
, _noCache(false)
, _cancel(false)
, _enc(Auto)
{}

/**
 * Construct a PageBuilder with a container for the specified page element.
 * @param elements  Container of the PageElements.
 * @param method    HTTP request method that the page can respond to.
 * @param noCache   Presence or absence of cache control headers generated by this page by default.
 * @param cancel    Whether this page is HTML construction only and does not involve page submission.
 * @param chunked   Default transfer encoding.
 */
PageBuilder::PageBuilder(PageElementVT elements, HTTPMethod method, bool noCache, bool cancel, TransferEncoding_t chunked)
: _elements(elements)
, _method(method)
, _noCache(noCache)
, _cancel(cancel)
, _enc(chunked)
{}

/**
 * Construct a PageBuilder with a container for the specified page element with the URI.
 * @param uri       URI of the page.
 * @param elements  Container of the PageElements.
 * @param method    HTTP request method that the page can respond to.
 * @param noCache   Presence or absence of cache control headers generated by this page by default.
 * @param cancel    Whether this page is HTML construction only and does not involve page submission.
 * @param chunked   Default transfer encoding.
 */
PageBuilder::PageBuilder(const char* uri, PageElementVT elements, HTTPMethod method, bool noCache, bool cancel, TransferEncoding_t chunked)
: _uri(String(uri))
, _elements(elements)
, _method(method)
, _noCache(noCache)
, _cancel(cancel)
, _enc(chunked)
{}

/**
 * Register the page as a 404 page into the WebServer.
 * @param   server  Reference of WebServer to register the onNotFound exit.
 */
void PageBuilder::atNotFound(WebServer& server) {
  server.onNotFound([&]() {
    setNoCache(true);
    _handle(404, server);
  });
}

/**
 * Register the page for authentication.
 * @param   username  The user name
 * @param   password  The password
 * @param   scheme    HTTP authentication scheme
 * @param   realm     The realm
 * @param   autoFail  Message for fails with authentication
 */
void PageBuilder::authentication(const char* username, const char* password, const HTTPAuthMethod scheme, const char* realm, const String& authFail) {
  _username = username ? String(username) : String();
  _password = password ? String(password) : String();
  _realm = realm ? String(realm) : String();
  _fails = String(authFail);
  _auth = scheme;
}

/**
 * Construct an HTML content, build it as String.
 * @param   content   Building content store buffer
 * @return  Content length built.
 */
size_t PageBuilder::build(String& content) {
  PageArgument  args;

  // Obtain requested arguments from the WebServer,
  // also reconstruct to pass the page handler.
  if (_server) {
    for (uint8_t i = 0; i < _server->args(); i++)
      args.push(_server->argName(i), _server->arg(i));
  }

  // Make an content of the page.
  return build(content, args);
}

/**
 * Construct an HTML content, build it as String.
 * @param   content   Building content store buffer
 * @param   args      HTTP request arguments to pass to the handler.
 * @return  Content length built.
 */
size_t PageBuilder::build(String& content, PageArgument& args) {
  size_t  cc = 0;

  size_t  rSize = _reserveSize;
  if (!rSize)
    rSize = _getApproxSize();
  PB_DBG("Buf preserve:%u, Free heap:%u ", rSize, ESP.getFreeHeap());
  if (!content.reserve(rSize)) {
    PB_DBG_DUMB("\n");
    PB_DBG("Content preliminary allocation failed. ");
  }

  // Content is buffered after combining all the elements at once.
  // If the buffer does not have enough space to store the content
  // generated on a PageElement basis, it will lose the content.
  for (auto& element : _elements) {
    String  elementBlock;
    cc += element.get().build(elementBlock, args);
    if (!content.concat(elementBlock)) {
      PB_DBG("Content lost, len:%u free:%u", elementBlock.length(), ESP.getFreeHeap());
      break;
    }
  }
  PB_DBG_DUMB("\n");
  return cc;
}

/**
 * Actual the canHandle function which overrides the RequestHandler.
 * @param   requestMethod   HTTP method of the current request.
 * @param   RequestUri      Requested URI
 * @return  true  the PageBuilder can handle this request.
 * @return  false 
 */
bool PageBuilder::canHandle(HTTPMethod requestMethod, PageBuilderUtil::URI_TYPE_SIGNATURE requestUri) {
  if (_canHandle)
    return _canHandle(requestMethod, requestUri);
  else {
    if (_method != HTTP_ANY && _method != requestMethod)
      return false;
    else if (requestUri != _uri)
      return false;
    return true;
  }
}

/**
 * Actual the canHandle function which overrides the RequestHandler.
 * @param   uri   Requested uploading URI.
 * @return  true  The uploader will activate.
 * @return  false The uploader does not correspond to the requested URI.
 */
bool PageBuilder::canUpload(PageBuilderUtil::URI_TYPE_SIGNATURE uri) {
  PB_DBG("%s upload request\n", uri.c_str());
  if (!_upload || !canHandle(HTTP_POST, uri))
    return false;
  return true;
}

/**
 * Clear the registered PageElements.
 */
void PageBuilder::clearElements(void) {
  _elements.clear();
  PageElementVT().swap(_elements);
}

/**
 * Calculates the approximate content size,
 * not including token replacement.
 * @return  Size of the generating content.
 */
size_t PageBuilder::_getApproxSize(void) const {
  size_t  approxSize = 0;

  for (auto& element : _elements)
    approxSize += (element.get().getApproxSize() + 16) & (~0xf);
  return approxSize;
}

/**
 * RequestHandler::handle function wrapper. It is responsible for the
 * "canHandle" identification and certification.
 * @param  server         Reference of the calling WebServer instance
 * @param  requestMethod  The HTTP request that made this call
 * @param  requestUri     The URI for this request
 * @return true   sent successfull
 * @return false  failed
 */
bool PageBuilder::handle(WebServer& server, HTTPMethod requestMethod, PageBuilderUtil::URI_TYPE_SIGNATURE requestUri) {
#ifdef PB_DEBUG
  const char* _httpMethod;
  if (requestMethod == HTTP_ANY)
    _httpMethod = "ANY";
  else {
    switch (requestMethod) {
    case HTTP_GET:
      _httpMethod = "GET";
      break;
    case HTTP_HEAD:
      _httpMethod = "HEAD";
      break;
    case HTTP_POST:
      _httpMethod = "POST";
      break;
    case HTTP_PUT:
      _httpMethod = "PUT";
      break;
    case HTTP_PATCH:
      _httpMethod = "PATCH";
      break;
    case HTTP_DELETE:
      _httpMethod = "DELETE";
      break;
    case HTTP_OPTIONS:
      _httpMethod = "OPTIONS";
      break;
    default:
      _httpMethod = "?";
    }
  }
  PB_DBG("HTTP_%s %s\n", _httpMethod, requestUri.c_str());
#endif

  if (!canHandle(requestMethod, requestUri))
    return false;

  if (_username.length()) {
    PB_DBG("auth:%s", _username.c_str());
    if (_password.length()) {
      PB_DBG_DUMB("/%s", _password.c_str());
    }
    PB_DBG_DUMB(" %s", _auth == HTTPAuthMethod::BASIC_AUTH ? "basic" : "digest");
    if (!server.authenticate(_username.c_str(), _password.c_str())) {
      PB_DBG_DUMB(" failure\n");
      server.requestAuthentication(_auth, _realm.c_str(), _fails);
      return true;
    }
    PB_DBG_DUMB("\n");
  }

  // Reset the sending cancel, invoke the content generating and send
  _cancel = false;
  _handle(200, server);
  if (_cancel) {
    PB_DBG("Send canceled\n");
  }
  return true;
}

/**
 * The actual existence of the URL handler function called from
 * the WebServer instance.
 * @param   code    HTTP code to respond to the request.
 * @param   server  Reference of the WebServer instance.
 */
void PageBuilder::_handle(int code, WebServer& server) {
  PageArgument  args;

  // Make a set of requested arguments
  for (uint8_t i = 0; i < server.args(); i++)
    args.push(server.argName(i), server.arg(i));

  if (_noCache)
    for (auto& httpHeader : _headersNocache) {
      server.sendHeader(String(FPSTR(httpHeader.name)), String(FPSTR(httpHeader.value)));
    }

  if (_enc == Auto) {
    // TransferEncoding:Auto
    // PageBuilder generates the whole content of the page into a String
    // instance at once. If If its size exceeds PAGEBUILDER_CONTENTBLOCK_SIZE,
    // it will try to output the stream via streamFile. In that case,
    // Transfer-encoding will be chunked.
    String  contentBlock;
    build(contentBlock, args);
    if (!_cancel) {
      if (contentBlock.length() > PAGEBUILDER_CONTENTBLOCK_SIZE) {
        char  wrBuf[PAGEBUILDER_CONTENTBLOCK_SIZE];
        WiFiClient  client = server.client();
        PageStream  content(contentBlock, client);
        server.setContentLength(contentBlock.length());
        server.send(200, "text/html", "");
        while (content.available() > 0) {
          size_t  nRd = content.readBytes(wrBuf, sizeof(wrBuf));
          client.write(wrBuf, nRd);
        }
      }
      else
        server.send(code, "text/html", contentBlock);
      PB_DBG("blk:%u\n", contentBlock.length());
    }
  }

  else if (_enc == Chunked || _enc == ByteStream) {
    // TransferEncoding:Chunked or ByteStream
    // Chunk transmission applies to both of these transmission schemes.
    PB_DBG("Chunked, ");
    bool  firstOrder = true;
    if (_enc == Chunked) {
      // Chunks generate and send a page segment for each element of
      // PageElements. PageBuilder needs enough heap space to store a
      // segment of the page content into a String instance.
      for (auto& element : _elements) {
        String  contentBlock;
        size_t  blkSize = element.get().build(contentBlock, args);
        if (_cancel)
          return;
        else if (firstOrder) {
          server.setContentLength(CONTENT_LENGTH_UNKNOWN);
          server.send(code, "text/html", "");
          firstOrder = false;
        }
        server.sendContent_P(contentBlock.c_str());
        (void)(blkSize);
        PB_DBG("blk:%u\n", blkSize);
      }
    }
    else {
      // ByteStream sending is similar to a chunk, creates a segment of
      // the page for each content element, but not through a String
      // instance. It allocates a fixed-length buffer and reuses it, so
      // it consumes less heap space regardless of HTML generating size.
      char* cBuffer = reinterpret_cast<char*>(malloc(PAGEBUILDER_CONTENTBLOCK_SIZE));
      if (cBuffer) {
        char* bp = cBuffer;
        size_t  cBufferLen = PAGEBUILDER_CONTENTBLOCK_SIZE;
        for (auto& element : _elements) {
          PageElement&  pe = element.get();
          pe.rewind();
          size_t  blkSize = pe.build(bp, cBufferLen, args);
          if (_cancel)
            return;
          else if (firstOrder) {
            server.setContentLength(CONTENT_LENGTH_UNKNOWN);
            server.send(code, "text/html", "");
            firstOrder = false;
          }
          while (blkSize) {
            server.sendContent_P(cBuffer, blkSize);
            PB_DBG_DUMB("blk:%u ", blkSize);
            bp += blkSize;
            cBufferLen -= blkSize;
            if (!cBufferLen) {
              bp = cBuffer;
              cBufferLen = PAGEBUILDER_CONTENTBLOCK_SIZE;
            }
            blkSize = pe.build(bp, cBufferLen, args);
          }
        }
        free(cBuffer);
        PB_DBG_DUMB("\n");
      }
      else {
        PB_DBG_DUMB("failed, free:%u\n", ESP.getFreeHeap());
      }
    }
    server.sendContent("");
  }
}

/**
 * Wrapper for the uploader
 * @param   server      Reference of the WebServer
 * @param   requestUri  Uploading URI
 * @param   upload      The uploader
 */
void PageBuilder::upload(WebServer& server, PageBuilderUtil::URI_TYPE_SIGNATURE requestUri, HTTPUpload& upload) {
  (void)(server);
  if (canUpload(requestUri))
    _upload(requestUri, upload);
}
