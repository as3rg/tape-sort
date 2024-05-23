#pragma once
#include "tape.h"

#include <algorithm>
#include <random>
#include <vector>

namespace tape {
  namespace helpers {
    /**
     * Class, which contains the information about the minimum, maximum and size of some subarray.
     */
    class subarray_info {
    private:
      int32_t min_;
      int32_t max_;
      size_t size_;

    public:
      [[nodiscard]] int32_t min() const;

      [[nodiscard]] int32_t max() const;

      [[nodiscard]] size_t size() const;

      subarray_info();

      /**
       * Update the information with new element of the subarray.
       */
      void update(int32_t value);
    };

    /**
     * Move the head backward and read the value.
     * @throws io_exception if reading fails
     * @throws seek_exception if seeking fails
     */
    template <typename T1>
    int32_t peek(tape<T1>& current) {
      current.prev();
      return current.get();
    }

    /**
     * Write the value and move the head forward.
     * @throws io_exception if writing fails
     * @throws seek_exception if seeking fails
     */
    template <typename T1>
    void put(tape<T1>& current, const int32_t value) {
      current.set(value);
      current.next();
    }

    /**
     * @code put()@endcode the elements from @code vec@endcode in @code current@endcode.<br>
     * The original ordering of the elements from the vector is saved in the tape.<br>
     * @code current@endcode head is after the last elements put (the last element of the vector) after the call.
     *
     * @throws io_exception if writing fails
     * @throws seek_exception if seeking fails
     */
    template <typename T>
    void vec_to_tape(const std::vector<int32_t>& vec, tape<T>& current) {
      for (const auto v : vec) {
        put(current, v);
      }
    }

    /**
     * @code peek()@endcode up to @code size@endcode elements
     * from the @code current@endcode and put them to @code std::vector@endcode
     * The original ordering of the elements from the tape is reversed in the vector.<br>
     * @code source@endcode head is at the leftmost element peeked after the call.
     * @throws io_exception if reading fails
     * @throws seek_exception if seeking fails
     */
    template <typename T>
    std::vector<int32_t> tape_to_vec(tape<T>& current, size_t size) {
      std::vector<int32_t> vec;
      vec.reserve(size);
      while (!current.is_begin() && size--) {
        vec.push_back(peek(current));
      }
      return vec;
    }

    /**
     * @code peek()@endcode exactly @code size@endcode elements from the @code source@endcode.<br>
     * @code put()@endcode the element in @code left@endcode if @code compare(element, key)@endcode.
     * Otherwise @code put()@endcode the element in @code right@endcode.<br>
     * @code left@endcode and @code right@endcode heads are after the last elements put after the call.
     * The original ordering of elements is not saved after the call.<br>
     * @code source@endcode head is at the leftmost element peeked after the call.
     *
     * @return @code std::pair@endcode of the @code subarray_info@endcode of the elements
     * put in @code left@endcode and @code right@endcode
     */
    template <typename T1, typename T2, typename T3, typename Compare>
    std::pair<subarray_info, subarray_info> split(tape<T1>& source, tape<T2>& left,
                                                  tape<T3>& right, Compare& compare,
                                                  const int32_t key,
                                                  const size_t size) {
      subarray_info left_info;
      subarray_info right_info;

      for (size_t i = 0; i < size; ++i) {
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

    /**
     * Put @code info.size()@endcode elements from @code current@endcode to @code out@endcode in the sorted order. <br>
     * The ordering is defined by the @code compare@endcode. <br>
     * @code current@endcode head is at the leftmost of @code info.size()@endcode elements after the call. <br>
     * @code tmp1@endcode and @code tmp2@endcode are not changed after the call.<br>
     * @code out@endcode head is after the last elements put after the call.<br>
     * If @code info.size() <= chunk_size@endcode, the sorting is performed in memory. Otherwise, recursively.
     */
    template <typename T1, typename T2, typename T3, typename T4, typename Compare>
    void sort_impl(tape<T4>& out, tape<T1>& current, tape<T2>& tmp1, tape<T3>& tmp2,
                   const subarray_info& info, const size_t chunk_size,
                   Compare& compare) {
      if (info.size() == 0)
        return;
      if (info.min() == info.max()) {
        for (size_t i = 0; i < info.size(); ++i) {
          helpers::put(out, helpers::peek(current));
        }
        return;
      }
      if (info.size() <= chunk_size) {
        auto vec = tape_to_vec(current, info.size());
        std::sort(vec.begin(), vec.end(), compare);
        vec_to_tape(vec, out);
        return;
      }

      static std::mt19937 gen(std::random_device{}());

      const int32_t mid =
          std::uniform_int_distribution(info.min(), info.max())(gen);
      auto [left_info, right_info] =
          split<>(current, tmp1, tmp2, compare, mid, info.size());
      sort_impl(out, tmp1, current, tmp2, left_info, chunk_size, compare);
      sort_impl(out, tmp2, current, tmp1, right_info, chunk_size, compare);
    }
  } // namespace helpers

  /**
   * Put elements from @code in@endcode to @code out@endcode in the sorted order. <br>
   * @code in@endcode, @code tmp1@endcode, @code tmp2@endcode and @code tmp3@endcode are not changed after the call.<br>
   * @code out@endcode head is after the last elements put after the call.<br>
   *
   * @param in tape with elements to sort. Can be read-only
   * @param out tape to write the sorted elements. Can be write-only
   * @param tmp1 temporary tape. Must be readable and writable
   * @param tmp2 temporary tape. Must be readable and writable
   * @param tmp3 temporary tape. Must be readable and writable
   * @param chunk_size the maximum number of elements that can be stored in memory
   * @param compare comparator which defines the ordering
   */
  template <typename T_IN, typename T_OUT, typename T1, typename T2, typename T3,
            typename Compare = std::less<int32_t>>
    requires(tape<T_IN>::READABLE && tape<T_OUT>::WRITABLE &&
             tape<T1>::BIDIRECTIONAL && tape<T2>::BIDIRECTIONAL &&
             tape<T3>::BIDIRECTIONAL)
  void sort(tape<T_IN>& in, tape<T_OUT>& out, tape<T1>& tmp1, tape<T2>& tmp2,
            tape<T3>& tmp3, size_t chunk_size = 1, Compare compare = Compare()) {
    helpers::subarray_info info;

    while (!in.is_end()) {
      const int32_t value = in.get();
      in.next();
      helpers::put(tmp1, value);
      info.update(value);
    }

    in.seek_impl(-info.size());
    helpers::sort_impl(out, tmp1, tmp2, tmp3, info, chunk_size, compare);
  }
} // namespace tape