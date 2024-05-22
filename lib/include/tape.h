#pragma once

#include <cassert>
#include <fstream>
#include <limits>

namespace tape {
  namespace helpers {

    template <typename T>
    std::true_type is_char_istream_impl(std::basic_istream<char, T>&&);
    std::false_type is_char_istream_impl(...);

    template <typename T>
    constexpr bool is_char_istream_v = decltype(is_char_istream_impl(std::declval<T>()))::value;

    template <typename T>
    std::true_type is_char_ostream_impl(std::basic_ostream<char, T>&&);
    std::false_type is_char_ostream_impl(...);

    template <typename T>
    constexpr bool is_char_ostream_v = decltype(is_char_ostream_impl(std::declval<T>()))::value;
  }

  template <typename Stream>
  class tape {
    static constexpr bool READABLE = helpers::is_char_istream_v<Stream>;
    static constexpr bool WRITABLE = helpers::is_char_ostream_v<Stream>;
    static_assert(READABLE || WRITABLE);

    using value_t = int32_t;
    static constexpr ptrdiff_t VALUE_SIZE = sizeof(value_t);

    size_t pos = 0;
    size_t size = 0;
    Stream stream;
    bool consistent = false;
    value_t buffer = 0;

  public:
    tape() noexcept = default;

    tape(Stream&& stream, const size_t pos, const size_t size) : pos(pos), size(size),
                                                                    stream(std::move(stream)) {}

    tape(const tape& other) = delete;

    tape(tape&& other) noexcept = default;

    tape& operator=(const tape& other) = delete;

    tape& operator=(tape&& other) noexcept {
      if (this != &other) {
        swap(*this, other);
      }
      return *this;
    }

    ~tape() noexcept = default;

    [[nodiscard]] bool is_end() const noexcept {
      return pos == size;
    }

    [[nodiscard]] bool is_begin() const noexcept {
      return pos == 0;
    }

    value_t get() requires(READABLE) {
      assert(!is_end());
      if (!consistent) {
        buffer = get_int(stream);
        consistent = true;
      }
      return buffer;
    }

    void set(const value_t new_value) requires(WRITABLE) {
      assert(!is_end());
      buffer = new_value;
      consistent = true;
      put_int(stream, new_value);
    }

    void seek(const ptrdiff_t diff) {
      assert(check_diff(diff));
      const ptrdiff_t delta = VALUE_SIZE * diff;
      if constexpr (WRITABLE) {
        stream.seekp(delta, std::ios_base::cur);
      }
      if constexpr (READABLE) {
        stream.seekg(delta, std::ios_base::cur);
      }
      pos += diff;
      consistent = false;
    }

    tape& operator++() {
      seek(1);
      return *this;
    }

    tape& operator--() {
      seek(-1);
      return *this;
    }

    friend void swap(tape& lhs, tape& rhs) noexcept(std::is_nothrow_swappable_v<decltype(stream)>) {
      using std::swap;
      swap(lhs.pos, rhs.pos);
      swap(lhs.size, rhs.size);
      swap(lhs.stream, rhs.stream);
      swap(lhs.consistent, rhs.consistent);
      swap(lhs.buffer, rhs.buffer);
    }

  private:
    static constexpr size_t MAX_SIZE_T = std::numeric_limits<size_t>::max();
    static constexpr size_t MIN_PTRDIFF_T = std::numeric_limits<ptrdiff_t>::min();

    [[nodiscard]] bool check_diff(const ptrdiff_t diff) const {
      if (diff > 0) {
        //checks overflow and bounds
        return pos <= MAX_SIZE_T - diff && pos + diff <= size;
      }
      return diff != MIN_PTRDIFF_T && pos >= -diff;
    }

    static int32_t get_int(std::istream& stream) {
      int32_t buffer = 0;
      stream.read(reinterpret_cast<char*>(&buffer), sizeof(buffer));
      if (stream.eof()) {
        buffer = 0;
        stream.clear();
      } else if (!stream) {
        //todo
        assert(false);
      }
      stream.seekg(-stream.gcount(), std::ios_base::cur);
      return buffer;
    }

    static void put_int(std::ostream& stream, int32_t value) {
      constexpr ptrdiff_t delta = sizeof(value);
      stream.write(reinterpret_cast<char*>(&value), delta);
      if (!stream) {
        //todo
        assert(false);
      }
      stream.seekp(-delta, std::ios_base::cur);
    }
  };
} // namespace tape