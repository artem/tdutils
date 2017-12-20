#pragma once

#include "td/utils/port/config.h"

#ifdef TD_EVENTFD_LINUX

#include "td/utils/common.h"
#include "td/utils/port/EventFdBase.h"
#include "td/utils/port/Fd.h"
#include "td/utils/Status.h"

namespace td {
namespace detail {

class EventFdLinux final : public EventFdBase {
  Fd fd_;

 public:
  void init() override;

  bool empty() override;

  void close() override;

  Status get_pending_error() override TD_WARN_UNUSED_RESULT;

  const Fd &get_fd() const override;
  Fd &get_fd() override;

  void release() override;

  void acquire() override;
};

}  // namespace detail
}  // namespace td

#endif
