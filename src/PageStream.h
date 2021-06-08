/**
 *  Declaration of PageStream class.
 *  @file PageStream.h
 *  @author hieromon@gmail.com
 *  @version  1.5.0
 *  @date 2020-12-31
 *  @copyright  MIT license.
 */

#ifndef _PAGESTREAM_H_
#define _PAGESTREAM_H_

#include <WiFiClient.h>
#include <Stream.h>

/**
 * Implementation of a class with a Stream interface for sending the
 * HTML content of a page via ESP8266WebServer::streamFile.
 * Streams the string that can be referenced by the content to the
 * WiFiClient specified as a client parameter.
 */
class PageStream : public Stream {
 public:
  PageStream(String& content, WiFiClient& client) : _content(content), _pos(0), _client(client) {}
  virtual ~PageStream() {}
  int available(void) override { return _content.length() - _pos; }
  void  flush(void) override {}
  const String& name(void) const { static const String  _empty; return _empty; }
  int peek(void) override { return _pos < _content.length() ? _content[_pos] : -1; }
  int read(void) override { return _pos < _content.length() ? _content[_pos++] : -1; }
  size_t  size(void) const { return _content.length(); }
  size_t  readBytes(char* buffer, size_t length) override;
  size_t  write(uint8_t c) override { _client.write(c); return sizeof(char); }

 protected:
  String& _content;
  size_t  _pos;
  WiFiClient& _client;
};

#endif // !_PAGESTREAM_H_
