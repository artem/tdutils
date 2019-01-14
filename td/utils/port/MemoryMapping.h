#pragma once
#include "td/utils/common.h"
#include "td/utils/port/FileFd.h"

namespace td {
namespace detail {
class MemoryMappingImpl;
}

class MemoryMapping {
 public:
  struct Options {
    int64 offset{0};
    int64 size{-1};

    Options() {
    }
    Options &with_offset(int64 offset) {
      this->offset = offset;
      return *this;
    }
    Options &with_size(int64 size) {
      this->size = size;
      return *this;
    }
  };

  static Result<MemoryMapping> create_anonymous(const Options &options = {});
  static Result<MemoryMapping> create_from_file(const FileFd &file, const Options &options = {});

  Slice as_slice() const;
  MutableSlice as_mutable_slice();  // returns empty slice if memory is read-only

  MemoryMapping(MemoryMapping &&other);
  MemoryMapping &operator=(MemoryMapping &&other);
  ~MemoryMapping();
  MemoryMapping(const MemoryMapping &other) = delete;
  const MemoryMapping &operator=(const MemoryMapping &other) = delete;

 private:
  std::unique_ptr<detail::MemoryMappingImpl> impl_;
  explicit MemoryMapping(std::unique_ptr<detail::MemoryMappingImpl> impl);
};
}  // namespace td
