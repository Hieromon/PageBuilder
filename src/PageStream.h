/**
 *  Declaration of PaguStream class.
 *  @file PageStream.h
 *  @author hieromon@gmail.com
 *  @version  1.3.2
 *  @date 2019-02-07
 *  @copyright  MIT license.
 */
#ifndef _PAGESTREAM_H
#define _PAGESTREAM_H

#include <Stream.h>
//#include <StreamString.h>

/**
 * An implementation of a class with a Stream interface to pass an
 * instance of that class to ESP8266WebServer::streamFile.
 * @param content A reference of String which contains sending HTML.
 */
class PageStream : public Stream {
 public:
  explicit PageStream(String& str) : _content(str), _pos(0) {}
  virtual ~PageStream() {}
  virtual int    available() { return _content.length() - _pos; }
  virtual void   flush() {}
  virtual const String& name() const { static const String _empty; return _empty; }
  virtual int    peek() { return _pos < _content.length() ? _content[_pos] : -1; }
  virtual int    read() { return _pos < _content.length() ? _content[_pos++] : -1; }
  virtual size_t readBytes(char* buffer, size_t length);
  virtual size_t size() const { return _content.length(); }
  virtual size_t write(uint8_t c) { _content += static_cast<char>(c); return 1; }

 private:
  String& _content;
  size_t  _pos;
};

#endif  // !_PAGESTREAM_H
