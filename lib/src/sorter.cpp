#include "../include/sorter.h"

int32_t tape::helpers::split_info::min() const {
  return min_;
}

int32_t tape::helpers::split_info::max() const {
  return max_;
}

size_t tape::helpers::split_info::size() const {
  return size_;
}

tape::helpers::split_info::split_info(const int32_t current_min, const int32_t current_max): min_(current_max),
                                                                                             max_(current_min),
                                                                                             size_(0) {}

void tape::helpers::split_info::update(const int32_t value) {
  min_ = std::min(min_, value);
  max_ = std::max(max_, value);
  ++size_;
}