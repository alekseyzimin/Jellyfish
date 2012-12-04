/*  This file is part of Jellyfish.

    Jellyfish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jellyfish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jellyfish.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>

#include <iostream>

namespace jellyfish {

template<typename word = uint64_t>
class obstream {
  static const size_t bword = sizeof(word) * 8;
  std::ostream* os_;
  size_t       boff;
  word         buffer;

public:
  obstream(std::ostream& os) : os_(&os), boff(0), buffer(0) { }
  ~obstream() { close(); }

  void close() { align(); }

  const std::ostream& stream() const { return *os_; }

  /// Write at most a word. Must satisfy len <= bword
  void write(word w, size_t len) {
    static size_t written = 0;
    w      &= bitmask<word>(len);
    written += len;
    buffer |= w << boff;
    boff   += len;
    if(boff >= bword) {
      os_->write((const char*)&buffer, sizeof(word));
      boff   -= bword;
      buffer  = rshift(w, len - boff);
    }
  }

  /// Align to the next word
  void align() {
    if(boff > 0) {
      os_->write((const char*)&buffer, sizeof(word));
      buffer = 0;
      boff   = 0;
    }
  }

  /// Align to the next byte
  void byte_align() {
    size_t remain = boff % 8;
    if(remain) {
      boff += 8 - remain;
      if(boff == bword) {
        os_->write((const char*)&buffer, sizeof(word));
        buffer = 0;
        boff   = 0;
      }
    }
  }

  /// Pad stream with 1s until next word
  void one_pad() {
    if(boff > 0) {
      buffer |= bitmask<word>(bword) << boff;
      os_->write((const char*)&buffer, sizeof(word));
      buffer = 0;
      boff   = 0;
    }
  }
};

template<typename word = uint64_t>
class ibstream {
  static const size_t bword = sizeof(word) * 8;
  std::istream* is_;
  size_t       bleft; // bits left in buffer
  word         buffer;

public:
  ibstream(std::istream& is) : is_(&is), bleft(0), buffer(0) { }
  ~ibstream() { close(); }

  void close() { align(); }

  const std::istream& stream() const { return *is_; }

  /// Read at most a word. Must satisfy len <= bword
  word read(size_t len) {
    word res;
    res = buffer;
    if(len > bleft) {
      is_->read((char*)&buffer, sizeof(word));
      size_t used   = len - bleft;
      res          |= buffer << bleft;
      bleft         = bword - used;
      buffer        = rshift(buffer, used);
    } else {
      buffer  = rshift(buffer, len);
      bleft  -= len;
    }
    static size_t _read = 0;

    _read += len;
    return res & bitmask<word>(len);
  }

  /// Read and ignore up to next word boundary.
  void align() { bleft = 0; }
};

} // namespace jellyfish