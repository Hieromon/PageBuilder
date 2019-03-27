/**
 *  PageBuilder class implementation which includes the class methods of 
 *  PageElement.
 *  @file   PageBuilder.cpp
 *  @author hieromon@gmail.com
 *  @version    1.3.3
 *  @date   2019-03-11
 *  @copyright  MIT license.
 */

#include "PageBuilder.h"
#include "PageStream.h"
#include <FS.h>
#ifdef ARDUINO_ARCH_ESP32
#include <SPIFFS.h>
#endif

// A maximum length of the content block to switch to chunked transfer.
#define MAX_CONTENTBLOCK_SIZE 1270

/** 
 *  HTTP header structure.
 */
typedef struct {
    const char* _field;         /**< HTTP header field */
    const char* _directives;    /**< HTTP header directives */
} HTTPHeaderS;

/**
 *  No cache control http response headers.
 *  The definition of the response header of this fixed character string is 
 *  used in the sendNocacheHeader method.
 */
static const HTTPHeaderS    _HttpHeaderNocache[] PROGMEM = {
    { "Cache-Control", "no-cache, no-store, must-revalidate" },
    { "Pragma", "no-cache" },
    { "Expires", "-1" }
};

// PageBuilder class methods

/**
 *  Returns whether this page handle HTTP request.<br>
 *  Matches HTTP methods and uri defined in the PageBuilder constructor and 
 *  returns whether this page should handle the request. This function 
 *  overrides the canHandle method of the RequestHandler class.
 *  @param  requestMethod   The http method that caused this http request.
 *  @param  requestUri      The uri of this http request.
 *  @retval true    This page can handle this request.
 *  @retval false   This page does not correspond to this request. 
 */
bool PageBuilder::canHandle(HTTPMethod requestMethod, String requestUri) {
    if (_canHandle) {
        return _canHandle(requestMethod, requestUri);
    }
    else {
        if (_method != HTTP_ANY && _method != requestMethod)
            return false;
        else if (requestUri != _uri)
            return false;
        return true;
    }
}

/**
 *  Returns whether the page can be uploaded.<br>
 *  However, since the PageBuilder class does not support uploading, it always 
 *  returns false. This function overrides the canUpload method of the 
 *  RequestHandler class.
 *  @param  requestUri  The uri of this upload request.
 *  @retval false   This page cannot receive the upload request.
 */
bool PageBuilder::canUpload(String uri) {
    PB_DBG("%s upload request\n", uri.c_str());
    if (!_upload || !canHandle(HTTP_POST, uri))
        return false;
    return true;
}

/**
 *  Invokes a user sketch function which would be registered by the
 *  onUpload function to process the received upload data.
 *  @param  server      A reference of ESP8266WebServer.
 *  @param  requestUri  Request uri of this time.
 *  @param  upload      A reference of the context of the HTTPUpload structure. 
 */
void PageBuilder::upload(WebServerClass& server, String requestUri, HTTPUpload& upload) {
    (void)server;
    if (canUpload(requestUri))
        _upload(requestUri, upload);
}

/**
 *  Generates HTML content and sends it to the client in response.<br>
 *  This function overrides the handle method of the RequestHandler class.
 *  @param  server          A reference of ESP8266WebServer.
 *  @param  requestMethod   Method of http request that originated this response.
 *  @param  requestUri      Request uri of this time.
 *  @retval true    A response send.
 *  @retval false   This request could not handled.
 */
bool PageBuilder::_sink(int code, WebServerClass& server) { //, HTTPMethod requestMethod, String requestUri) {
    // Retrieve request arguments
    PageArgument args;
    for (uint8_t i = 0; i < server.args(); i++)
        args.push(server.argName(i), server.arg(i));

    // Invoke page building function
    _cancel = false;

    // If the page is must-revalidate, send a header
    if (_noCache)
        sendNocacheHeader(server);

    // send http content to client
    if (!_cancel) {
        String  content;                // Content is available only in a NOT _cancel block
        size_t  contLen;
        bool    _chunked = (_sendEnc == PB_Chunk);

        if (_sendEnc == PB_ByteStream || _sendEnc == PB_Auto) {
            // Build the content, and determine the transfer mode by
            // the length of the built content.
            content = build(args);
            contLen = content.length();
            if (_sendEnc == PB_Auto && contLen >= MAX_CONTENTBLOCK_SIZE)
                _chunked = true;
        }
        PB_DBG("Free heap:%d, content len.:", ESP.getFreeHeap());
        if (_chunked)
            PB_DBG_DUMB("%s\n", "unknown");
        else
            PB_DBG_DUMB("%d\n", contLen);
        PB_DBG("Res:%d, Chunked:%d\n", code, _sendEnc);
        // Start content transfer, switch the transfer method depending
        // on whether it is chunked transfer or byte stream.
        if (_chunked) {
            PB_DBG("Transfer-Encoding:chunked\n");
            server.setContentLength(CONTENT_LENGTH_UNKNOWN);
            server.send(code, F("text/html"), _emptyString);
            if (PB_Chunk) {
                // Chunk block is a PageElement unit.
                for (uint8_t i = 0; i < _element.size(); i++) {
                    PageElement& element = _element[i].get();
                    content = PageElement::build(element.mold(), element.source(), args);
                    if (content.length())
                        server.sendContent(content);
                }
            }
            else {
                // Here, PB_Auto turned to chunk mode.
                size_t  pos = 0;
                while (pos < contLen) {
                    String contBuffer = content.substring(pos, pos + MAX_CONTENTBLOCK_SIZE); 
                    server.sendContent(contBuffer);
                    pos += MAX_CONTENTBLOCK_SIZE;
                }
            }
            server.sendContent(_emptyString);
        }
        else {
            if (_sendEnc == PB_ByteStream || contLen > MAX_CONTENTBLOCK_SIZE) {
                PageStream  contStream(content);
                server.streamFile(contStream, F("text/html"));
                server.client().flush();
            } else {
                server.setContentLength(contLen);
                server.send(code, F("text/html"), content);
            }
        }
    }
    return true;
}

/**
 *  The page builder handler generates HTML based on PageElement and sends
 *  it to the client.
 *  @param  server          A reference of ESP8266WebServer.
 *  @param  requestMethod   Method of http request that originated this response.
 *  @param  requestUri      Request uri of this time.
 *  @retval true    A response send.
 *  @retval false   This request could not handled.
 */
bool PageBuilder::handle(WebServerClass& server, HTTPMethod requestMethod, String requestUri) {
    // Screening the available request
    if (!canHandle(requestMethod, requestUri))
        return false;

    // Throw with 200 response
    return _sink(200, server);
}

/**
 *  Send a 404 response.
 */
void PageBuilder::exit404(void) {
    if (_server != nullptr) {
        _noCache = true;

        // Throw with 404 response
        _sink(404, *_server);
    }
}

/**
 *  Dispose collection for cataloged PageElement.
 */
void PageBuilder::clearElement() {
    _element.clear();
    PageElementVT().swap(_element);
}

/**
 *  Send a http header for the page to be must-revalidate on the client.
 *  @param  server  A reference of ESP8266WebServer.
 */
void PageBuilder::sendNocacheHeader(WebServerClass& server) {
    for (uint8_t i = 0; i < sizeof(_HttpHeaderNocache) / sizeof(HTTPHeaderS); i++)
        server.sendHeader(_HttpHeaderNocache[i]._field, _HttpHeaderNocache[i]._directives);
}

/**
 *  Register the not found page to ESP8266Server.
 *  @param  server  A reference of ESP8266WebServer.
 */
void PageBuilder::atNotFound(WebServerClass& server) {
    _server = &server;
    server.onNotFound(std::bind(&PageBuilder::exit404, this));
}

/**
 *  A wrapper function for making response content without specifying the 
 *  parameter of http request.
 *  This function is called from other than the On handle action of 
 *  ESP8266WebServer and assumes execution of content creation only by PageElement. 
 *  @retval String of content built by the instance of PageElement.
 */
String PageBuilder::build(void) {
    PageArgument    args;
    return build(args);
}

/**
 *  Build html content with handling the PageElement elements.<br>
 *  Assemble the content by calling the build method of all owning PageElement 
 *  and concatenating the returned string.
 *  @param  args    URI parameter when this page is requested.
 *  @retval A html content string.  
 */
String PageBuilder::build(PageArgument& args) {
    String content = String("");

    if (_rSize > 0) {
        if (content.reserve(_rSize)) {
            PB_DBG("Content buffer %ld reserved\n", (long)_rSize);
        }
        else {
            PB_DBG("Content buffer cannot reserve(%ld)\n", (long)_rSize);
        }
    }

    for (uint8_t i = 0; i < _element.size(); i++) {
        PageElement& element = _element[i].get();
        String segment = PageElement::build(element.mold(), element.source(), args);
        if (segment.length()) {
            if (!content.concat(segment)) {
                PB_DBG("Content lost, length:%d, free heap:%ld\n", segment.length(), (long)ESP.getFreeHeap());
            }
        }
    }
    return content;
}

/**
 *  Initialize an empty string to allow returning const String& with nothing.
 */
const String PageBuilder::_emptyString = String("");

// PageElement class methods.

/**
 *  PageElement destructor
 */
PageElement::~PageElement() {
    _source.clear();
    TokenVT().swap(_source);
    _mold = nullptr;
}

/***
 *  Add new token to the page element.
 *  A token is pare of a token string and handler function.
 *  @param  token   A token string.
 *  @param  handler A handler function. 
 */
void PageElement::addToken(const String& token, HandleFuncT handler) {
    TokenSourceST  source = TokenSourceST();
    source._token = token;
    source._builder = handler;
    _source.push_back(source);
}

/***
 *  @brief  Build html source.<br>
 *  It is a wrapper entry for calling the static PageElement::build method.
 *  @retval A string generated from the mold of this instance.
 */
String PageElement::build() {
    PageArgument    args;
    return String(PageElement::build(_mold, _source, args));
}

/**
 *  Generate a actual string from the mold including the token. also, this 
 *  function is static to reduce the heap size. 
 *  @param  mold        A pointer of constant mold character array. A string
 *  following the "file:" prefix indicates the filename of the SPIFFS file system.
 *  @param  tokenSource A structure that anchors user functions which processes
 *  tokens defined in the mold.
 *  @param  args        PageArgument instance when this handler requested.
 *  @retval A string containing the content of the token actually converted 
 *  by the user function.
 */
String PageElement::build(const char* mold, TokenVT tokenSource, PageArgument& args) {
    int     contextLength;
    int     scanIndex = 0;
    String  templ = FPSTR(mold);
    String  content = String("");

    // Determining the origin of the mold.
    // When the mold parameter has the prefix "file:", read the mold source
    // from the file.
    if (templ.startsWith(PAGEELEMENT_FILE)) {
        File mf = SPIFFS.open(templ.substring(strlen(PAGEELEMENT_FILE)), "r");
        if (mf) {
            templ = mf.readString();
            mf.close();
        }
        else
            templ = String("");
    }
    contextLength = templ.length();
    while (contextLength > 0) {
        String token;
        int tokenStart, tokenEnd;

        // Extract the braced token
        if ((tokenStart = templ.indexOf("{{", scanIndex)) >= 0) {
            tokenEnd = templ.indexOf("}}", tokenStart);
            if (tokenEnd > tokenStart)
                token = templ.substring(tokenStart + 2, tokenEnd);
            else
                tokenStart = tokenEnd = templ.length();
        }
        else {
            tokenStart = tokenEnd = templ.length();
        }
        // Materialize the template which would be stored to content
        // content += templ.substring(scanIndex, tokenStart);
        bool grown = content.concat(templ.substring(scanIndex, tokenStart));
        if (grown) {
            scanIndex = tokenEnd + 2;
            contextLength = templ.length() - scanIndex;

            // Invoke the callback function corresponding to the extracted token
            for (uint8_t i = 0; i < tokenSource.size(); ++i)
                if (tokenSource[i]._token == token) {
                    grown = content.concat(tokenSource[i]._builder(args));
                    break;
                }
        }
        if (!grown) {
            PB_DBG("Failed building, free heap:%ld\n", (long)ESP.getFreeHeap());
            break;
        }
    }

    PB_DBG("at leaving build: %ld free\n", (long)ESP.getFreeHeap());
    return content;
}

// PageArgument class methods.

/**
 *  Returns a parameter string specified argument name.
 *  @param  name    A parameter argument name.
 *  @retval A parameter value string.
 */
String PageArgument::arg(const String& name) {
    for (uint8_t i = 0; i < size(); i++) {
        RequestArgument*    argItem = item(i);
        if (argItem->_key == name)
            return argItem->_value;
    }
    return String();
}

/**
 *  Returns a parameter string specified index of arguments array.
 *  @param  i   Index of argument array.
 *  @retval A parameter value string.
 */
String PageArgument::arg(int i) {
    if ((size_t)i < size()) {
        RequestArgument*    argItem = item(i);
        return argItem->_value;
    }
    return String();
}

/**
 *  Returns a argument string specified index of arguments array.
 *  @param  i   Index of argument array.
 *  @retval A string of argument name.
 */
String PageArgument::argName(int i) {
    if ((size_t)i < size()) {
        RequestArgument*    argItem = item(i);
        return argItem->_key;
    }
    return String();
}

/**
 *  Returns number of parameters of this http request.
 *  @retval Number of parameters.
 */
int PageArgument::args() {
    int size = 0;
    RequestArgumentSL*  argList = _arguments.get();
    while (argList != nullptr) {
        size++;
        argList = argList->_next.get();
    }
    return size;
}

/**
 *  Returns whether the http request to PageBuiler that invoked this PageElement
 *  contained parameters.
 *  @retval true    This http request contains parameter(s).
 *  @retval false   This http request has no parameter.
 */
bool PageArgument::hasArg(const String& name) {
    for (uint8_t i = 0; i < size(); i++) {
        RequestArgument*    argument = item(i);
        if (argument == nullptr)
            return false;
        if (argument->_key == name)
            return true;
    }
    return false;
}

/**
 *  Append argument of http request consisting of parameter name and 
 *  argument pair.
 *  @param  key     A parameter name string.
 *  @param  value   A parameter value string.
 */
void PageArgument::push(const String& key, const String& value) {
    RequestArgument*    newArg = new RequestArgument();
    newArg->_key = key;
    newArg->_value = value;
    RequestArgumentSL*  newList = new RequestArgumentSL();
    newList->_argument.reset(newArg);
    newList->_next.reset(nullptr);
    RequestArgumentSL*  argList = _arguments.get();
    if (argList == nullptr) {
        _arguments.reset(newList);
    }
    else {
        while (argList->_next.get() != nullptr)
            argList = argList->_next.get();
        argList->_next.reset(newList);
    }
}

/**
 *  Returns a pointer to RequestArgument instance specified index of
 *  parameters array.
 *  @param  index   A index of parameter arguments array.
 *  @retval A pointer to RequestArgument.
 */
RequestArgument* PageArgument::item(int index) {
    RequestArgumentSL*  argList = _arguments.get();
    while (index--) {
        if (argList == nullptr)
            break;
        argList = argList->_next.get();
    }
    if (argList != nullptr)
        return argList->_argument.get();
    else
        return nullptr;
}
