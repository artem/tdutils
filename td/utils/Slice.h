#pragma once
#include "td/utils/Slice-decl.h"
#include "td/utils/logging.h"

#include <algorithm>

namespace td {
/*** MutableSlice ***/
inline MutableSlice::MutableSlice() : MutableSlice(const_cast<char *>(""), (size_t)0) {
}

inline MutableSlice::MutableSlice(void *s, size_t len) : s_(static_cast<char *>(s)), len_(len) {
  CHECK(s_ != nullptr);
}

inline MutableSlice::MutableSlice(string &s) : MutableSlice(&s[0], s.size()) {
}

inline MutableSlice::MutableSlice(const Slice &from) : MutableSlice(const_cast<char *>(from.begin()), from.size()) {
}

inline MutableSlice::MutableSlice(void *s, void *t) : MutableSlice(s, static_cast<char *>(t) - static_cast<char *>(s)) {
}

inline void MutableSlice::clear() {
  s_ = const_cast<char *>("");
  len_ = 0;
}

inline size_t MutableSlice::size() const {
  return len_;
}

inline MutableSlice &MutableSlice::remove_prefix(size_t prefix_len) {
  CHECK(prefix_len <= len_);
  s_ += prefix_len;
  len_ -= prefix_len;
  return *this;
}
inline MutableSlice &MutableSlice::remove_suffix(size_t suffix_len) {
  CHECK(suffix_len <= len_);
  len_ -= suffix_len;
  return *this;
}

inline MutableSlice &MutableSlice::truncate(size_t size) {
  if (len_ > size) {
    len_ = size;
  }
  return *this;
}

inline MutableSlice &MutableSlice::rtruncate(size_t size) {
  if (len_ > size) {
    s_ += len_ - size;
    len_ = size;
  }
  return *this;
}

inline MutableSlice MutableSlice::copy() {
  return *this;
}

inline bool MutableSlice::empty() const {
  return len_ == 0;
}

inline char *MutableSlice::data() const {
  return s_;
}

inline char *MutableSlice::begin() const {
  return s_;
}

inline unsigned char *MutableSlice::ubegin() const {
  return reinterpret_cast<unsigned char *>(s_);
}

inline char *MutableSlice::end() const {
  return s_ + len_;
}

inline unsigned char *MutableSlice::uend() const {
  return reinterpret_cast<unsigned char *>(s_) + len_;
}

inline string MutableSlice::str() const {
  return string(begin(), size());
}

inline MutableSlice MutableSlice::substr(size_t from) const {
  CHECK(from <= len_);
  return MutableSlice(s_ + from, len_ - from);
}
inline MutableSlice MutableSlice::substr(size_t from, size_t size) const {
  CHECK(from <= len_);
  return MutableSlice(s_ + from, std::min(size, len_ - from));
}

inline size_t MutableSlice::rfind(char c) const {
  for (size_t pos = len_; pos-- > 0;) {
    if (s_[pos] == c) {
      return pos;
    }
  }
  return static_cast<size_t>(-1);
}

inline void MutableSlice::copy_from(Slice from) {
  CHECK(size() >= from.size());
  memcpy(ubegin(), from.ubegin(), from.size());
}

inline char &MutableSlice::back() {
  CHECK(1 <= len_);
  return s_[len_ - 1];
}

inline char &MutableSlice::operator[](size_t i) {
  return s_[i];
}

/*** Slice ***/
inline Slice::Slice() : Slice("", (size_t)0) {
}

inline Slice::Slice(const MutableSlice &other) : Slice(other.begin(), other.size()) {
}

inline Slice::Slice(const void *s, size_t len) : s_(static_cast<const char *>(s)), len_(len) {
  CHECK(s_ != nullptr);
}

inline Slice::Slice(const string &s) : Slice(s.c_str(), s.size()) {
}

inline Slice::Slice(const std::vector<unsigned char> &v) : Slice(v.data(), v.size()) {
}

inline Slice::Slice(const std::vector<char> &v) : Slice(v.data(), v.size()) {
}

inline Slice::Slice(const void *s, const void *t)
    : Slice(s, static_cast<const char *>(t) - static_cast<const char *>(s)) {
}

inline void Slice::clear() {
  s_ = "";
  len_ = 0;
}

inline size_t Slice::size() const {
  return len_;
}

inline Slice &Slice::remove_prefix(size_t prefix_len) {
  CHECK(prefix_len <= len_);
  s_ += prefix_len;
  len_ -= prefix_len;
  return *this;
}

inline Slice &Slice::remove_suffix(size_t suffix_len) {
  CHECK(suffix_len <= len_);
  len_ -= suffix_len;
  return *this;
}

inline Slice &Slice::truncate(size_t size) {
  if (len_ > size) {
    len_ = size;
  }
  return *this;
}
inline Slice &Slice::rtruncate(size_t size) {
  if (len_ > size) {
    s_ += len_ - size;
    len_ = size;
  }
  return *this;
}

inline Slice Slice::copy() {
  return *this;
}

inline bool Slice::empty() const {
  return len_ == 0;
}

inline const char *Slice::data() const {
  return s_;
}

inline const char *Slice::begin() const {
  return s_;
}

inline const unsigned char *Slice::ubegin() const {
  return reinterpret_cast<const unsigned char *>(s_);
}

inline const char *Slice::end() const {
  return s_ + len_;
}

inline const unsigned char *Slice::uend() const {
  return reinterpret_cast<const unsigned char *>(s_) + len_;
}

inline string Slice::str() const {
  return string(begin(), size());
}

inline Slice Slice::substr(size_t from) const {
  CHECK(from <= len_);
  return Slice(s_ + from, len_ - from);
}
inline Slice Slice::substr(size_t from, size_t size) const {
  CHECK(from <= len_);
  return Slice(s_ + from, std::min(size, len_ - from));
}

inline size_t Slice::rfind(char c) const {
  for (size_t pos = len_; pos-- > 0;) {
    if (s_[pos] == c) {
      return pos;
    }
  }
  return static_cast<size_t>(-1);
}

inline char Slice::back() const {
  CHECK(1 <= len_);
  return s_[len_ - 1];
}

inline char Slice::operator[](size_t i) const {
  return s_[i];
}

inline bool operator==(const Slice &a, const Slice &b) {
  return a.size() == b.size() && memcmp(a.data(), b.data(), a.size()) == 0;
}

inline bool operator!=(const Slice &a, const Slice &b) {
  return !(a == b);
}

inline MutableCSlice::MutableCSlice(void *s, void *t) : MutableSlice(s, t) {
  CHECK(*static_cast<char *>(t) == '\0');
}

inline CSlice::CSlice(const char *s, const char *t) : Slice(s, t) {
  CHECK(*t == 0);
}

}  // namespace td
