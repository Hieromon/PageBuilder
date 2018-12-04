/**
 *  An implementation of a actual function of PageStream class.
 *  @file PageStream.cpp
 *  @author hieromon@gmail.com
 *  @version  1.2.0
 *  @date 2018-12-01
 *  @copyright  MIT license.
 */

#include "PageStream.h"

/**
 *  Read into the buffer from the String.
 *  @param  buffer  A pointer of the buffer to be read string.
 *  @param  length  Maximum reading size.
 *  @return Number of bytes readed. 
 */
size_t PageStream::readBytes(char *buffer, size_t length) {
    size_t  count = 0;
    while (count < length) {
        if (_pos >= _content.length())
            break;
        *buffer++ = _content[_pos++];
        count++;
    }
    return count;
}
