#pragma once

#include "td/utils/common.h"
#include "td/utils/logging.h"
#include "td/utils/port/FileFd.h"
#include "td/utils/port/path.h"

#include <cstdlib>
#include <limits>

namespace td {

// TODO move implementation to cpp
class FileLog : public LogInterface {
 private:
  static constexpr int DEFAULT_ROTATE_THRESHOLD = 10 * (1 << 20);

 public:
  FileLog() = default;
  FileLog(const FileLog &other) = delete;
  FileLog &operator=(const FileLog &other) = delete;
  FileLog(FileLog &&other) = delete;
  FileLog &operator=(FileLog &&other) = delete;
  ~FileLog() override {
    if (!fd_.empty()) {
      fd_.close();
    }
  }
  void append(CSlice xslice, int log_level) override {
    Slice slice = xslice;
    while (!slice.empty()) {
      auto r_size = fd_.write(slice);
      if (r_size.is_error()) {
        std::abort();
      }
      auto written = r_size.ok();
      size_ += static_cast<off_t>(written);
      slice.remove_prefix(written);
    }
    if (log_level == VERBOSITY_NAME(FATAL)) {
      std::abort();
    }

    if (size_ > rotate_threshold_) {
      auto status = rename(path_, path_ + ".old");
      if (status.is_error()) {
        std::abort();
      }
      do_rotate();
    }
  }

  void rotate() override {
    if (path_.empty()) {
      return;
    }
    do_rotate();
  }

  void init(string path, off_t rotate_threshold = DEFAULT_ROTATE_THRESHOLD) {
    path_ = std::move(path);

    auto r_fd = FileFd::open(path_, FileFd::Create | FileFd::Write | FileFd::Append);
    LOG_IF(FATAL, r_fd.is_error()) << "Can't open log: " << r_fd.error();
    fd_ = r_fd.move_as_ok().move_as_fd();
    Fd::duplicate(fd_, Fd::Stderr()).ignore();
    auto stat = fd_.stat();
    size_ = stat.size_;
    rotate_threshold_ = rotate_threshold;
  }

  void init(Fd fd) {
    path_ = "";
    fd_ = std::move(fd);
    size_ = 0;
    rotate_threshold_ = std::numeric_limits<off_t>::max();
  }

 private:
  Fd fd_;
  string path_;
  off_t size_;
  off_t rotate_threshold_;

  void do_rotate() {
    auto current_verbosity_level = GET_VERBOSITY_LEVEL();
    SET_VERBOSITY_LEVEL(std::numeric_limits<int>::min());  // to ensure that nothing will be printed to the closed log
    CHECK(!path_.empty());
    fd_.close();
    auto r_fd = FileFd::open(path_, FileFd::Create | FileFd::Truncate | FileFd::Write);
    if (r_fd.is_error()) {
      std::abort();
    }
    fd_ = r_fd.move_as_ok().move_as_fd();
    Fd::duplicate(fd_, Fd::Stderr()).ignore();
    size_ = 0;
    SET_VERBOSITY_LEVEL(current_verbosity_level);
  }
};
}  // namespace td
