#include "td/utils/Slice.h"
#include "td/utils/Span.h"

#if TD_PORT_POSIX
#include <sys/uio.h>
#endif
namespace td {
struct IoVector {
#if TD_PORT_POSIX
  using Value = struct iovec;
#else
  using Value = Slice;
#endif

 public:
  void clear() {
    vector_.clear();
    data_size_ = 0;
  }
  void push_back(Slice slice) {
    vector_.push_back(as_value(slice));
    data_size_ += slice.size();
  }
  size_t size() const {
    return vector_.size();
  }
  size_t data_size() const {
    return data_size_;
  }

  // private?
  Span<Value> as_span() {
    return vector_;
  }

 private:
  Value as_value(Slice slice) {
#if TD_PORT_POSIX
    Value res;
    res.iov_len = slice.size();
    res.iov_base = const_cast<char *>(slice.data());
    return res;
#else
    return slice;
#endif
  }
  std::vector<Value> vector_;
  size_t data_size_{0};
};
}  // namespace td
