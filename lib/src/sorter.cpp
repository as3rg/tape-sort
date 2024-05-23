#include "../include/sorter.h"

int32_t tape::helpers::subarray_info::min() const {
  return min_;
}

int32_t tape::helpers::subarray_info::max() const {
  return max_;
}

size_t tape::helpers::subarray_info::size() const {
  return size_;
}

using LIMITS = std::numeric_limits<int32_t>;

tape::helpers::subarray_info::subarray_info()
  : min_(LIMITS::max()), max_(LIMITS::min()), size_(0) {}

void tape::helpers::subarray_info::update(const int32_t value) {
  min_ = std::min(min_, value);
  max_ = std::max(max_, value);
  ++size_;
}