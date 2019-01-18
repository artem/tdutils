#pragma once

#include <cstring>
#include <type_traits>

namespace td {

namespace detail {

template <class T>
class As {
 public:
  explicit As(void *ptr) : ptr_(ptr) {
  }

  As(const As &new_value) = delete;
  As &operator=(const As &) = delete;
  As(As &&) = default;
  As &operator=(As &&new_value) && {
    std::memcpy(ptr_, new_value.ptr_, sizeof(T));
    return *this;
  }
  ~As() = default;

  As &operator=(const T &new_value) && {
    std::memcpy(ptr_, &new_value, sizeof(T));
    return *this;
  }

  operator T() const {
    T res;
    std::memcpy(&res, ptr_, sizeof(T));
    return res;
  }

 private:
  void *ptr_;
};

template <class T>
class ConstAs {
 public:
  explicit ConstAs(const void *ptr) : ptr_(ptr) {
  }

  operator T() const {
    T res;
    std::memcpy(&res, ptr_, sizeof(T));
    return res;
  }

 private:
  const void *ptr_;
};

}  // namespace detail

template <class ToT, class FromT,
          std::enable_if_t<std::is_trivially_copyable<ToT>::value && std::is_trivially_copyable<FromT>::value, int> = 0>
detail::As<ToT> as(FromT *from) {
  return detail::As<ToT>(from);
}

template <class ToT, class FromT,
          std::enable_if_t<std::is_trivially_copyable<ToT>::value && std::is_trivially_copyable<FromT>::value, int> = 0>
const detail::ConstAs<ToT> as(const FromT *from) {
  return detail::ConstAs<ToT>(from);
}

}  // namespace td
