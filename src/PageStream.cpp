/**
 *  An implementation of a actual function of PageStream class.
 *  @file PageStream.cpp
 *  @author hieromon@gmail.com
 *  @version  1.5.0
 *  @date 2018-12-31
 *  @copyright  MIT license.
 */

#include "PageStream.h"

/**
 * Stream read into the buffer from the context string.
 * @param   buffer A destination to read.
 * @param   length A capacity that can be saved in the buffer.
 * @return  Read length.
 */
size_t PageStream::readBytes(char* buffer, size_t length) {
  size_t  wc = 0;

  while (wc < length) {
    if (_pos >= _content.length())
      break;
    *buffer++ = _content[_pos++];
    wc++;
  }
  return wc;
}
