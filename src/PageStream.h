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
#include <StreamString.h>

/**
 * An implementation of a class with a Stream interface to pass an
 * instance of that class to ESP8266WebServer::streamFile.
 * @param content A reference of String which contains sending HTML.
 */
class PageStream : public StreamString {
 public:
  explicit PageStream(String& content) : _content(content), _pos(0) {}
  ~PageStream() {}
  char          charAt(unsigned int loc) { return _content[loc]; }
  unsigned char concat(char c) { return _content.concat(c); }
  size_t        length() { return _content.length() - _pos; }
  const String  name() const { return ""; }
  size_t        readBytes(char *buffer, size_t length);
  void          remove(unsigned int index, unsigned int count) { _content.remove(index, count); }
  size_t        size() const { return _content.length(); }

 private:
  String& _content;
  size_t  _pos;
};

#endif  // !_PAGESTREAM_H
