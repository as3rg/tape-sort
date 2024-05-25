#pragma once
#include "tape.h"

#include <algorithm>
#include <random>
#include <vector>

namespace tape {
  namespace helpers {
    /**
     * Class, which contains the information about some subarray.<br>
     */
    template <typename Compare>
    class subarray_info {
    private:
      Compare compare_;
      bool equal_ = true;
      int32_t element_ = 0;
      size_t size_ = 0;

    public:
      /**
       * @return some element of the subarray. The elements are uniformly distributed.
       */
      [[nodiscard]] int32_t element() const {
        return element_;
      }

      /**
       * @return @code true@endcode if all the elements of the subarray are equal (with given comparator).
       */
      [[nodiscard]] bool equal() const {
        return equal_;
      }

      /**
       * @return size of the subarray.
       */
      [[nodiscard]] size_t size() const {
        return size_;
      }

      explicit subarray_info(Compare compare) : compare_(compare) {}

      /**
       * Update the information with new element of the subarray.<br>
       */
      void update(const int32_t value) {
        static std::mt19937 gen(std::random_device{}());
        equal_ = equal_ && (size_ == 0 || !compare_(element_, value) && !compare_(value, element_));

        /**
         * About the probability:
         * Let it be the i-th call of update() (so the size_ == i - 1).
         * The element() is updated with the new value with the probability c[i] = 1 / i.
         * So for each j the element() is equal to the j-th value with probability
         * p[j] = c[j] * (1 - c[j+1]) * ... * (1 - c[i]) = 1 / i.
         * Thus after each call of update() the element() is equal to one of the values with the uniform distribution.
         */
        if (std::uniform_int_distribution<>(0, size_)(gen) == 0) {
          element_ = value;
        }
        ++size_;
      }
    };

    /**
     * Move the head backward and read the value.
     * @throws io_exception if reading fails
     */
    template <typename T>
      requires(tape<T>::READABLE)
    int32_t peek(tape<T>& current) {
      current.prev();
      return current.get();
    }

    /**
     * Write the value and move the head forward.
     * @throws io_exception if writing fails
     */
    template <typename T>
      requires(tape<T>::WRITABLE)
    void put(tape<T>& current, const int32_t value) {
      current.set(value);
      current.next();
    }

    /**
     * @code put()@endcode the elements from @code vec@endcode in @code current@endcode.<br>
     * The original ordering of the elements from the vector is saved in the tape.<br>
     * @code current@endcode head is after the last elements put (the last element of the vector) after the call.
     *
     * @throws io_exception if writing fails
     */
    template <typename T>
      requires(tape<T>::WRITABLE)
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
     */
    template <typename T>
      requires(tape<T>::READABLE)
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
     * @throws io_exception if reading or writing to some of the tapes fails
     */
    template <typename TSrc, typename TLeft, typename TRight, typename Compare>
      requires(tape<TSrc>::READABLE && tape<TLeft>::WRITABLE && tape<TRight>::WRITABLE)
    std::pair<subarray_info<Compare>, subarray_info<Compare>> split(tape<TSrc>& source, tape<TLeft>& left,
                                                                    tape<TRight>& right, Compare compare,
                                                                    const int32_t key,
                                                                    const size_t size) {
      subarray_info left_info(compare);
      subarray_info right_info(compare);

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
     * @code peek()@endcode @code info.size()@endcode elements from @code current@endcode and
     * @code put()@endcode them in @code out@endcode in the sorted order. <br>
     * The ordering is defined by the @code compare@endcode. <br>
     * @code tmp1@endcode and @code tmp2@endcode data before the head and the head position are not changed after the call.
     * The data after the head can be lost.<br>
     * @code out@endcode head is after the last elements put after the call.<br>
     * If @code info.size() <= chunk_size@endcode, the sorting is performed in memory. Otherwise, recursively.
     * @throws io_exception if reading or writing to some of the tapes fails
     */
    template <typename TOut, typename T1, typename T2, typename T3, typename Compare>
      requires(tape<TOut>::WRITABLE &&
               tape<T1>::BIDIRECTIONAL && tape<T2>::BIDIRECTIONAL &&
               tape<T3>::BIDIRECTIONAL)
    void sort_impl(tape<TOut>& out, tape<T1>& current, tape<T2>& tmp1, tape<T3>& tmp2,
                   const subarray_info<Compare>& info, const size_t chunk_size,
                   Compare compare) {
      if (info.size() == 0)
        return;
      if (info.equal()) {
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

      auto [left_info, right_info] = split<>(current, tmp1, tmp2, compare, info.element(), info.size());
      sort_impl(out, tmp1, current, tmp2, left_info, chunk_size, compare);
      sort_impl(out, tmp2, current, tmp1, right_info, chunk_size, compare);
    }
  } // namespace helpers

  /**
   * Put elements from @code in@endcode to @code out@endcode in the sorted order. <br>
   * @code in@endcode is not changed after the call.<br>
   * @code out@endcode head is after the last elements put after the call.<br>
   * The function uses as much allocated memory as the @code in@endcode data occupies.<br>
   * The sort is not stable.
   *
   * @param in tape with elements to sort. Can be read-only. The head should be at the beginning of the data
   * @param out tape to write the sorted elements. Can be write-only. The head should be at the first position to write
   * @param compare comparator which defines the ordering
   * @throws io_exception if reading or writing to some of the tapes fails
   */
  template <typename TIn, typename TOut,
            typename Compare = std::less<int32_t>>
    requires(tape<TIn>::READABLE && tape<TOut>::WRITABLE)
  void sort(tape<TIn>& in, tape<TOut>& out, Compare compare = Compare()) {
    std::vector<int32_t> vec;
    size_t size = 0;
    while (!in.is_end()) {
      const int32_t value = in.get();
      in.next();
      vec.emplace_back(value);
      ++size;
    }

    in.seek(-size);

    std::sort(vec.begin(), vec.end(), compare);
    helpers::vec_to_tape(vec, out);
  }

  /**
   * Put elements from @code in@endcode to @code out@endcode in the sorted order. <br>
   * @code in@endcode is not changed after the call.<br>
   * @code tmp1@endcode, @code tmp2@endcode and @code tmp3@endcode data before the head and the head position are not changed after the call.
   * The data after the head can be lost.<br>
   * @code out@endcode head is after the last elements put after the call.<br>
   * The function uses no more than @code chunk_size * sizeof(int32_t)@endcode bytes of allocated memory.<br>
   * The sort is not stable.
   *
   * @param in tape with elements to sort. Can be read-only. The head should be at the beginning of the data
   * @param out tape to write the sorted elements. Can be write-only. The head should be at the first position to write
   * @param tmp1 temporary tape. Must be readable and writable.
   * Should have at least as much space after the head as the size of the sorted data
   * @param tmp2 temporary tape. Must be readable and writable
   * Should have at least as much space after the head as the size of the sorted data
   * @param tmp3 temporary tape. Must be readable and writable
   * Should have at least as much space after the head as the size of the sorted data
   * @param chunk_size the maximum number of elements that can be stored in memory
   * @param compare comparator which defines the ordering
   * @throws io_exception if reading or writing to some of the tapes fails
   */
  template <typename TIn, typename TOut, typename T1, typename T2, typename T3,
            typename Compare = std::less<int32_t>>
    requires(tape<TIn>::READABLE && tape<TOut>::WRITABLE &&
             tape<T1>::BIDIRECTIONAL && tape<T2>::BIDIRECTIONAL &&
             tape<T3>::BIDIRECTIONAL)
  void sort(tape<TIn>& in, tape<TOut>& out, tape<T1>& tmp1, tape<T2>& tmp2,
            tape<T3>& tmp3, size_t chunk_size = 0, Compare compare = Compare()) {
    helpers::subarray_info<Compare> info(compare);

    while (!in.is_end()) {
      const int32_t value = in.get();
      in.next();
      helpers::put(tmp1, value);
      info.update(value);
    }

    in.seek(-info.size());
    helpers::sort_impl(out, tmp1, tmp2, tmp3, info, chunk_size, compare);
  }
} // namespace tape