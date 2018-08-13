#pragma once
#include "td/utils/port/config.h"
#include "td/utils/common.h"

#include "td/utils/MovableValue.h"
namespace td {
class StringBuilder;
}
namespace td {
class NativeFd {
 public:
#if TD_PORT_POSIX
  using Raw = int;
#elif TD_PORT_WINDOWS
  using Raw = HANDLE;
#endif
  NativeFd() = default;
  NativeFd(NativeFd &&) = default;
  NativeFd &operator=(NativeFd &&) = default;
  explicit NativeFd(Raw raw);
  NativeFd(Raw raw, bool nolog);
#if TD_PORT_WINDOWS
  explicit NativeFd(SOCKET raw);
#endif
  ~NativeFd();

  explicit operator bool() const;
  static constexpr Raw empty_raw();
  Raw raw() const;
  Raw fd() const;
#if TD_PORT_WINDOWS
  Raw io_handle() const;
  SOCKET socket() const;
#elif TD_PORT_POSIX
  Raw socket() const;
#endif
  void close();
  Raw release();

 private:
#if TD_PORT_POSIX
  MovableValue<Raw, -1> fd_;
#elif TD_PORT_WINDOWS
  MovableValue<Raw, INVALID_HANDLE_VALUE> fd_;
  bool is_socket_{false};
#endif
};

StringBuilder &operator<<(StringBuilder &sb, const NativeFd &fd);
}  // namespace td
