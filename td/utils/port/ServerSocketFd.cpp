#include "td/utils/port/ServerSocketFd.h"

#include "td/utils/port/config.h"

#include "td/utils/logging.h"
#include "td/utils/port/IPAddress.h"
#include "td/utils/port/PollFlags.h"

#if TD_PORT_POSIX

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#endif

#if TD_PORT_WINDOWS
#include "td/utils/port/detail/WineventPoll.h"
#include "td/utils/SpinLock.h"
#include "td/utils/VectorQueue.h"
#endif

#include <atomic>
#include <cstring>

namespace td {

namespace detail {
#if TD_PORT_WINDOWS
class ServerSocketFdImpl : private IOCP::Callback {
 public:
  ServerSocketFdImpl(NativeFd fd, int socket_family) : info_(std::move(fd)), socket_family_(socket_family) {
    VLOG(fd) << get_native_fd().io_handle() << " create ServerSocketFd";
    IOCP::get()->subscribe(get_native_fd(), this);
    notify_iocp_read();
  }
  void close() {
    notify_iocp_close();
  }
  PollableFdInfo &get_poll_info() {
    return info_;
  }
  const PollableFdInfo &get_poll_info() const {
    return info_;
  }

  const NativeFd &get_native_fd() const {
    return info_.native_fd();
  }

  Result<SocketFd> accept() {
    auto lock = lock_.lock();
    if (accepted_.empty()) {
      get_poll_info().clear_flags(PollFlags::Read());
      return Status::Error(-1, "Operation would block");
    }
    return accepted_.pop();
  }

  Status get_pending_error() {
    Status res;
    {
      auto lock = lock_.lock();
      if (!pending_errors_.empty()) {
        res = pending_errors_.pop();
      }
      if (res.is_ok()) {
        get_poll_info().clear_flags(PollFlags::Error());
      }
    }
    return res;
  }

 private:
  PollableFdInfo info_;

  SpinLock lock_;
  VectorQueue<SocketFd> accepted_;
  VectorQueue<Status> pending_errors_;
  static constexpr size_t MAX_ADDR_SIZE = sizeof(sockaddr_in6) + 16;
  char addr_buf_[MAX_ADDR_SIZE * 2];

  bool close_flag_{false};
  std::atomic<int> refcnt_{1};
  bool is_read_active_{false};
  OVERLAPPED read_overlapped_;

  char close_overlapped_;

  NativeFd accept_socket_;
  int socket_family_;

  void on_close() {
    close_flag_ = true;
    info_.set_native_fd({});
  }
  void on_read() {
    VLOG(fd) << get_native_fd().io_handle() << " on_read";
    if (is_read_active_) {
      is_read_active_ = false;
      auto r_socket = SocketFd::from_native_fd(std::move(accept_socket_));
      VLOG(fd) << get_native_fd().io_handle() << " finish accept";
      if (r_socket.is_error()) {
        return on_error(r_socket.move_as_error());
      }
      {
        auto lock = lock_.lock();
        accepted_.push(r_socket.move_as_ok());
      }
      get_poll_info().add_flags_from_poll(PollFlags::Read());
    }
    loop_read();
  }
  void loop_read() {
    CHECK(!is_read_active_);
    accept_socket_ = NativeFd(socket(socket_family_, SOCK_STREAM, 0));
    std::memset(&read_overlapped_, 0, sizeof(read_overlapped_));
    VLOG(fd) << get_native_fd().io_handle() << " start accept";
    auto status = AcceptEx(get_native_fd().socket(), accept_socket_.socket(), addr_buf_, 0, MAX_ADDR_SIZE,
                           MAX_ADDR_SIZE, nullptr, &read_overlapped_);
    if (check_status(status, "Failed to accept connection")) {
      inc_refcnt();
      is_read_active_ = true;
    }
  }
  bool check_status(DWORD status, Slice message) {
    if (status == 0) {
      return true;
    }
    auto last_error = WSAGetLastError();
    if (last_error == ERROR_IO_PENDING) {
      return true;
    }
    on_error(OS_SOCKET_ERROR(message));
    return false;
  }
  bool dec_refcnt() {
    if (--refcnt_ == 0) {
      delete this;
      return true;
    }
    return false;
  }
  void inc_refcnt() {
    CHECK(refcnt_ != 0);
    refcnt_++;
  }

  void on_error(Status status) {
    {
      auto lock = lock_.lock();
      pending_errors_.push(std::move(status));
    }
    get_poll_info().add_flags_from_poll(PollFlags::Error());
  }

  void on_iocp(Result<size_t> r_size, OVERLAPPED *overlapped) override {
    // called from other thread
    if (dec_refcnt() || close_flag_) {
      return;
    }
    if (r_size.is_error()) {
      return on_error(r_size.move_as_error());
    }

    if (overlapped == nullptr) {
      return on_read();
    }
    if (overlapped == &read_overlapped_) {
      return on_read();
    }

    if (overlapped == reinterpret_cast<OVERLAPPED *>(&close_overlapped_)) {
      return on_close();
    }
    UNREACHABLE();
  }
  void notify_iocp_read() {
    VLOG(fd) << get_native_fd().io_handle() << " notify_read";
    inc_refcnt();
    IOCP::get()->post(0, this, nullptr);
  }
  void notify_iocp_close() {
    VLOG(fd) << get_native_fd().io_handle() << " notify_close";
    IOCP::get()->post(0, this, reinterpret_cast<OVERLAPPED *>(&close_overlapped_));
  }
};
void ServerSocketFdImplDeleter::operator()(ServerSocketFdImpl *impl) {
  impl->close();
}
#elif TD_PORT_POSIX
class ServerSocketFdImpl {
 public:
  explicit ServerSocketFdImpl(NativeFd fd) : info_(std::move(fd)) {
  }
  PollableFdInfo &get_poll_info() {
    return info_;
  }
  const PollableFdInfo &get_poll_info() const {
    return info_;
  }

  const NativeFd &get_native_fd() const {
    return info_.native_fd();
  }
  Result<SocketFd> accept() {
    sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    int native_fd = get_native_fd().fd();
    int r_fd = detail::skip_eintr([&] { return ::accept(native_fd, reinterpret_cast<sockaddr *>(&addr), &addr_len); });
    auto accept_errno = errno;
    if (r_fd >= 0) {
      return SocketFd::from_native_fd(NativeFd(r_fd));
    }

    if (accept_errno == EAGAIN
#if EAGAIN != EWOULDBLOCK
        || accept_errno == EWOULDBLOCK
#endif
    ) {
      get_poll_info().clear_flags(PollFlags::Read());
      return Status::Error(-1, "Operation would block");
    }

    auto error = Status::PosixError(accept_errno, PSLICE() << "Accept from [fd = " << native_fd << "] has failed");
    switch (accept_errno) {
      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENOTSOCK:
      case EOPNOTSUPP:
        LOG(FATAL) << error;
        UNREACHABLE();
        break;
      default:
        LOG(ERROR) << error;
      // fallthrough
      case EMFILE:
      case ENFILE:
      case ECONNABORTED:  //???
        get_poll_info().clear_flags(PollFlags::Read());
        get_poll_info().add_flags(PollFlags::Close());
        return std::move(error);
    }
  }

  Status get_pending_error() {
    if (get_poll_info().get_flags().has_pending_error()) {
      return Status::OK();
    }
    TRY_STATUS(detail::get_socket_pending_error(get_native_fd()));
    get_poll_info().clear_flags(PollFlags::Error());
    return Status::OK();
  }

 private:
  PollableFdInfo info_;
};
void ServerSocketFdImplDeleter::operator()(ServerSocketFdImpl *impl) {
  delete impl;
}
#endif
}  // namespace detail

ServerSocketFd::ServerSocketFd() = default;
ServerSocketFd::ServerSocketFd(ServerSocketFd &&) = default;
ServerSocketFd &ServerSocketFd::operator=(ServerSocketFd &&) = default;
ServerSocketFd::~ServerSocketFd() = default;
ServerSocketFd::ServerSocketFd(std::unique_ptr<detail::ServerSocketFdImpl> impl) : impl_(impl.release()) {
}
PollableFdInfo &ServerSocketFd::get_poll_info() {
  return impl_->get_poll_info();
}

const PollableFdInfo &ServerSocketFd::get_poll_info() const {
  return impl_->get_poll_info();
}

Status ServerSocketFd::get_pending_error() {
  return impl_->get_pending_error();
}

const NativeFd &ServerSocketFd::get_native_fd() const {
  return impl_->get_native_fd();
}

Result<SocketFd> ServerSocketFd::accept() {
  return impl_->accept();
}

void ServerSocketFd::close() {
  impl_.reset();
}

bool ServerSocketFd::empty() const {
  return !impl_;
}

Result<ServerSocketFd> ServerSocketFd::open(int32 port, CSlice addr) {
  IPAddress address;
  TRY_STATUS(address.init_ipv4_port(addr, port));
  NativeFd fd{socket(address.get_address_family(), SOCK_STREAM, 0)};
  if (!fd) {
    return OS_SOCKET_ERROR("Failed to create a socket");
  }

  TRY_STATUS(fd.set_is_blocking_unsafe(false));
  auto sock = fd.socket();

  linger ling = {0, 0};
#if TD_PORT_POSIX
  int flags = 1;
#ifdef SO_REUSEPORT
  setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char *>(&flags), sizeof(flags));
#endif
#elif TD_PORT_WINDOWS
  BOOL flags = TRUE;
#endif
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&flags), sizeof(flags));
  setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char *>(&flags), sizeof(flags));
  setsockopt(sock, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char *>(&ling), sizeof(ling));
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&flags), sizeof(flags));

  int e_bind = bind(sock, address.get_sockaddr(), static_cast<socklen_t>(address.get_sockaddr_len()));
  if (e_bind != 0) {
    return OS_SOCKET_ERROR("Failed to bind a socket");
  }

  // TODO: magic constant
  int e_listen = listen(sock, 8192);
  if (e_listen != 0) {
    return OS_SOCKET_ERROR("Failed to listen on a socket");
  }

#if TD_PORT_POSIX
  auto impl = std::make_unique<detail::ServerSocketFdImpl>(std::move(fd));
#elif TD_PORT_WINDOWS
  auto impl = std::make_unique<detail::ServerSocketFdImpl>(std::move(fd), address.get_address_family());
#endif

  return ServerSocketFd(std::move(impl));
}

}  // namespace td
