#pragma once

#include "td/utils/port/thread.h"

#include <atomic>
#include <memory>

namespace td {

class SpinLock {
  struct Unlock {
    void operator()(SpinLock *ptr) {
      ptr->unlock();
    }
  };

  class InfBackoff {
    int cnt = 0;

   public:
    bool next() {
      cnt++;
      if (cnt < 50) {
        return true;
      } else {
        td::this_thread::yield();
        return true;
      }
    }
  };

 public:
  using Lock = std::unique_ptr<SpinLock, Unlock>;

  Lock lock() {
    InfBackoff backoff;
    while (!try_lock()) {
      backoff.next();
    }
    return Lock(this);
  }
  bool try_lock() {
    return !flag_.test_and_set(std::memory_order_acquire);
  }

 private:
  std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
  void unlock() {
    flag_.clear(std::memory_order_release);
  }
};

}  // namespace td
