/**
 *  Declaration of PaguStream class.
 *  @file PageStream.h
 *  @author hieromon@gmail.com
 *  @version  1.2.0
 *  @date 2018-12-01
 *  @copyright  MIT license.
 */
#ifndef _PAGESTREAM_H
#define _PAGESTREAM_H

#include <Stream.h>

/**
 * An implementation of a class with a Stream interface to pass an
 * instance of that class to ESP8266WebServer::streamFile.
 * @param content A reference of String which contains sending HTML.
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

#endif  // !_PAGESTREAM_H
