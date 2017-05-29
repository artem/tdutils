#include "td/utils/port/config.h"

#ifdef TD_EVENTFD_LINUX

#include <sys/eventfd.h>

#include "td/utils/port/detail/EventFdLinux.h"

namespace td {
namespace detail {
EventFdLinux::operator FdRef() {
  return get_fd();
}
void EventFdLinux::init() {
  int fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  auto eventfd_errno = errno;
  LOG_IF(FATAL, fd == -1) << Status::PosixError(eventfd_errno, "eventfd call failed");

  fd_ = Fd(fd, Fd::Mode::Own);
}

bool EventFdLinux::empty() {
  return fd_.empty();
}

void EventFdLinux::close() {
  fd_.close();
}

Status EventFdLinux::get_pending_error() {
  return Status::OK();
}

const Fd &EventFdLinux::get_fd() const {
  return fd_;
}

Fd &EventFdLinux::get_fd() {
  return fd_;
}

void EventFdLinux::release() {
  uint64 value = 1;
  // NB: write_unsafe is used, because release will be call from multible threads.
  auto result = fd_.write_unsafe(Slice(&value, sizeof(value)));
  if (result.is_error()) {
    LOG(FATAL) << "EventFdLinux write failed: " << result.error();
  }
  size_t size = result.ok();
  if (size != sizeof(value)) {
    LOG(FATAL) << "EventFdLinux write returned " << value << " instead of " << sizeof(value);
  }
}

void EventFdLinux::acquire() {
  uint64 res;
  auto result = fd_.read(MutableSlice(&res, sizeof(res)));
  if (result.is_error()) {
    if (result.error().code() == EAGAIN || result.error().code() == EWOULDBLOCK) {
    } else {
      LOG(FATAL) << "EventFdLinux read failed: " << result.error();
    }
  }
  fd_.clear_flags(Fd::Read);
}
}  // namespace detail
}  // namespace td

#endif  // TD_EVENTFD_LINUX