/**
 *	Declaration of PaguBuilder class and accompanying PageElement, PageArgument class.
 *	@file	PageBuilder.h
 *	@author	hieromon@gmail.com
 *	@version	1.1.0
 *	@date	2018-08-21
 *	@copyright	MIT license.
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
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
using WebServerClass = ESP8266WebServer;
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
using WebServerClass = WebServer;
#endif

#define PAGEELEMENT_FILE	"file:"

/** HTTP get or post method argument pair structure. */
typedef struct _RequestArgumentS {
	String	_key;		/**< Parameter name */
	String	_value;		/**< Parameter value */
} RequestArgument;

/**
 *	Stores the uri parameter or POST argument that occurred in the http request.
 */
class PageArgument {
public:
	PageArgument() : _arguments(nullptr) {}
	PageArgument(String key, String value) : _arguments(nullptr) { push(key, value); }
	~PageArgument() {}
	String	arg(String name);
	String	arg(int i);
	String	argName(int i);
	int		args();
	size_t	size() { return (size_t)args(); }
	bool	hasArg(String name);
	void	push(String key, String value);

protected:
	/** RequestArgument element list structure */
	typedef struct _RequestArgumentSL {
		std::unique_ptr<RequestArgument>	_argument;		/**< A unique pointer to a RequestArgument */
		std::unique_ptr<_RequestArgumentSL>	_next;			/**< Pointer to the next element */
	} RequestArgumentSL;

	/** Root element of the list of RequestArgument */
	std::unique_ptr<RequestArgumentSL>	_arguments;

private:
	RequestArgument*	item(int index);
};

/** Wrapper type definition of handler function to handle token. */
typedef std::function<String(PageArgument&)>	HandleFuncT;

/** Structure with correspondence of token and handler defined in PageElement. */
typedef struct {
	String			_token;			/**< Token string */
	HandleFuncT		_builder;		/**< correspondence handler function */
} TokenSourceST;

/** TokenSourceST linear container type.
 *	Elements of tokens and handlers usually contained in PageElement are 
 *	managed linearly as a container using std::vector.
 */
typedef std::vector<TokenSourceST>	TokenVT;

/**
 *	PageElement class have parts of a HTML page and the components of the part 
 *	to be processed at specified position in the HTML as the token.
 */
class PageElement {
public:
	PageElement() : _mold(nullptr) {}
	PageElement(const char* mold) : _mold(mold), _source(std::vector<TokenSourceST>()) {}
	PageElement(const char* mold, TokenVT source) : _mold(mold), _source(source) {}
	~PageElement();

	const char*		mold() { return _mold; }
	TokenVT			source() { return _source; }
	String			build();
	static String	build(const char* mold, TokenVT tokenSource, PageArgument& args);
	void			setMold(const char* mold) { _mold = mold; }
	void			addToken(String token, HandleFuncT handler);

protected:
	const char*	_mold;		//*< A pointer to HTML model string(char array). */
	TokenVT		_source;	//*< Container of processable token and handler function. */
};

/** PageElement container.
 *	The PageBuilder class treats multiple PageElements constituting a html 
 *	page as a reference container as std::reference_wrapper.*/
typedef std::vector<std::reference_wrapper<PageElement>>	PageElementVT;

/** The user own function for the current uri handling preparation. */
typedef std::function<bool(HTTPMethod, String)>	PrepareFuncT;

/**
 *	PageBuilder class is to make easy to assemble and output html stream 
 *	of web page. The page builder class includes the uri of the page, 
 *	the PageElement indicating the HTML model constituting the page, and 
 *	the on handler called from the ESP8266WebServer class.
 *	This class inherits the RequestHandler class of the ESP8266WebServer class.
 */
class PageBuilder : public RequestHandler {
public:
	PageBuilder() : _uri(nullptr), _method(HTTP_ANY), _server(nullptr) {}
	PageBuilder(PageElementVT element, HTTPMethod method = HTTP_ANY, bool noCache = true, bool cancel = false) :
		_uri(nullptr),
		_element(element),
		_method(method),
		_noCache(noCache),
		_cancel(cancel),
		_server(nullptr),
		_canHandle(nullptr) {}
	PageBuilder(const char* uri, PageElementVT element, HTTPMethod method = HTTP_ANY, bool noCache = true, bool cancel = false) :
		_uri(uri),
		_element(element),
		_method(method),
		_noCache(noCache),
		_cancel(cancel),
		_server(nullptr),
		_canHandle(nullptr) {}

	~PageBuilder() { _uri = nullptr; _server = nullptr; clearElement(); }

	bool canHandle(HTTPMethod requestMethod, String requestUri) override;
	bool canUpload(String requestUri) override;
	bool handle(WebServerClass& server, HTTPMethod requestMethod, String requestUri) override;

	void setUri(const char* uri) { _uri = uri; }
	const char*	uri() { return _uri; }
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

protected:
	const char*		_uri;			/**< uri of this page */
	PageElementVT	_element;		/**< PageElement container */
	HTTPMethod		_method;		/**< Method of http request to which this page applies. */

private:
	bool	_sink(int code, WebServerClass& server); //, HTTPMethod requestMethod, String requestUri);
	bool	_noCache;		/**< A flag for must-revalidate cache control response */
	bool	_cancel;		/**< Automatic send cancellation */
	WebServerClass*	_server;
	PrepareFuncT	_canHandle;		/**< 'canHanlde' user owned function */
};

#endif