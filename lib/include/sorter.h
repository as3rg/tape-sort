#pragma once
#include "tape.h"

#include <algorithm>
#include <random>
#include <vector>

namespace tape {
  namespace helpers {
    class split_info {
    private:
      int32_t min_;
      int32_t max_;
      size_t size_;

    public:
      [[nodiscard]] int32_t min() const;

      [[nodiscard]] int32_t max() const;

      [[nodiscard]] size_t size() const;

      split_info(int32_t current_min, int32_t current_max);

      void update(int32_t value);
    };

    template <typename T1>
    int32_t peek(tape<T1>& current) {
      --current;
      return current.get();
    }

    template <typename T1>
    void put(tape<T1>& current, const int32_t value) {
      current.set(value);
      ++current;
    }

    template <typename T1, typename T2, typename T3, typename Compare>
    std::pair<split_info, split_info> split(
        tape<T1>& source, tape<T2>& left, tape<T3>& right, Compare& compare, const int32_t key,
        const split_info& info) {

      split_info left_info(info.min(), info.max());
      split_info right_info(info.min(), info.max());

      for (size_t i = 0; i < info.size(); ++i) {
        const int32_t value = helpers::peek(source);
        if (compare(value, key)) {
          helpers::put(left, value);
          left_info.update(value);
        } else {
          helpers::put(right, value);
          right_info.update(value);
        }
      }
      return std::make_pair(left_info, right_info);
    }

    template <typename T>
    void vec_to_tape(const std::vector<int32_t>& vec, tape<T>& current) {
      for (const auto v : vec) {
        put(current, v);
      }
    }

    // todo vector is reversed
    template <typename T>
    std::vector<int32_t> tape_to_vec(tape<T>& current, size_t size) {
      std::vector<int32_t> vec;
      vec.reserve(size);
      while (!current.is_begin() && size--) {
        vec.push_back(peek(current));
      }
      return vec;
    }

    template <typename T1, typename T2, typename T3, typename T4, typename Compare>
    void sort(tape<T4>& out, tape<T1>& current, tape<T2>& tmp1, tape<T3>& tmp2, const split_info& info, const size_t M,
              Compare& compare) {
      if (info.size() == 0)
        return;
      if (info.min() == info.max()) {
        for (size_t i = 0; i < info.size(); ++i) {
          helpers::put(out, helpers::peek(current));
        }
        return;
      }
      if (info.size() <= M) {
        auto vec = tape_to_vec(current, info.size());
        std::sort(vec.begin(), vec.end(), compare);
        vec_to_tape(vec, out);
        return;
      }

      static std::mt19937 gen(std::random_device{}());

      const int32_t mid = std::uniform_int_distribution(info.min(), info.max())(gen);
      auto [left_info, right_info] = split<>(current, tmp1, tmp2, compare, mid, info);
      sort(out, tmp1, current, tmp2, left_info, M, compare);
      sort(out, tmp2, current, tmp1, right_info, M, compare);
    }
  }

  template <typename T1, typename T2, typename T3, typename T4, typename T5, typename Compare = std::less<int32_t>>
  void sort(tape<T1>& in, tape<T2>& out, tape<T3>& tmp1, tape<T4>& tmp2, tape<T5>& tmp3, size_t M = 1,
            Compare compare = Compare()) {
    using LIMITS = std::numeric_limits<int32_t>;
    helpers::split_info info(LIMITS::min(), LIMITS::max());

    while (!in.is_end()) {
      const int32_t value = in.get();
      ++in;
      helpers::put(tmp1, value);
      info.update(value);
    }
    helpers::sort(out, tmp1, tmp2, tmp3, info, M, compare);
  }
}