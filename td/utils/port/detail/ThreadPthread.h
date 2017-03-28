#pragma once
#include "td/utils/port/config.h"

#ifdef TD_THREAD_PTHREAD

#include <pthread.h>

namespace td {
namespace detail {
class ThreadPthread {
 public:
  ThreadPthread() = default;
  ThreadPthread(ThreadPthread &&) = default;
  ThreadPthread &operator=(ThreadPthread &&) = default;
  template <class Function, class... Args>
  explicit ThreadPthread(Function &&f, Args &&... args) {
    func_ = std::make_unique<std::unique_ptr<Destructor>>(
        create_destructor([args = std::make_tuple(decay_copy(std::forward<Function>(f)),
                                                  decay_copy(std::forward<Args>(args))...)]() mutable {
          invoke_tuple(std::move(args));
          clear_thread_locals();
        }));
    pthread_create(&thread_, nullptr, run_thread, func_.get());
    is_inited_ = true;
  }
  void join() {
    if (is_inited_.get()) {
      pthread_join(thread_, nullptr);
    }
  }
  ~ThreadPthread() {
    join();
  }

 private:
  MovableValue<bool> is_inited_;
  pthread_t thread_;
  std::unique_ptr<std::unique_ptr<Destructor>> func_;

  template <class T>
  std::decay_t<T> decay_copy(T &&v) {
    return std::forward<T>(v);
  }

  static void *run_thread(void *ptr) {
    auto func = reinterpret_cast<decltype(func_.get())>(ptr);
    func->reset();
    return nullptr;
  }
};
}  // namespace detail
}  // namespace td

#endif  // TD_THREAD_PTHREAD
