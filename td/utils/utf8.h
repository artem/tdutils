#pragma once

#include "td/utils/int_types.h"
#include "td/utils/Slice.h"

namespace td {

// checks UTF-8 string for correctness
bool check_utf8(CSlice str);

// checks if a code unit is a first code unit of a UTF-8 character
inline bool is_utf8_symbol_begin_char(unsigned char c) {
  return (c & 0xC0) != 0x80;
}

// moves pointer one UTF-8 character back
inline const unsigned char *prev_utf8_unsafe(const unsigned char *ptr) {
  while (!is_utf8_symbol_begin_char(*--ptr)) {
    // pass
  }
  return ptr;
}

// moves pointer one UTF-8 character forward and saves code of the skipped character in *code
const unsigned char *next_utf8_unsafe(const unsigned char *ptr, uint32 *code);

// truncates UTF-8 string to the given length in Unicode characters
template <class T>
T utf8_truncate(T str, size_t length) {
  if (str.size() > length) {
    for (size_t i = 0; i < str.size(); i++) {
      if (is_utf8_symbol_begin_char(static_cast<unsigned char>(str[i]))) {
        if (length == 0) {
          return str.substr(0, i);
        } else {
          length--;
        }
      }
    }
  }
  return str;
}

// truncates UTF-8 string to the given length given in UTF-16 code units
template <class T>
T utf8_utf16_truncate(T str, size_t length) {
  for (size_t i = 0; i < str.size(); i++) {
    auto c = static_cast<unsigned char>(str[i]);
    if (is_utf8_symbol_begin_char(c)) {
      if (length <= 0) {
        return str.substr(0, i);
      } else {
        length--;
        if (c >= 0xf0) {  // >= 4 bytes in symbol => surrogaite pair
          length--;
        }
      }
    }
  }
  return str;
}

}  // namespace td
