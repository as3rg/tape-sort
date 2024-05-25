#pragma once

#include <cassert>
#include <fstream>
#include <limits>
#include <thread>

#include "exceptions/io_exception.h"

#include <algorithm>
#include <utility>

namespace tape {
  /**
   * Config for delay emulation.
   */
  class delay_config {
  public:
    /**
     * Delay in ns of the reading operations.
     */
    size_t read_delay = 0;

    /**
     * Delay in ns of the writing operations.
     */
    size_t write_delay = 0;

    /**
     * Delay in ns of a step of the rewinding operations.<br>
     * Resulting delay of the rewinding operation is @code rewind_delay + rewind_step_delay * abs(diff)@endcode,
     * where @code diff@endcode is the rewind width (count of steps).
     */
    size_t rewind_step_delay = 0;

    /**
     * Constant delay in ns of rewinding operations.<br>
     * Resulting delay of the rewinding operation is @code rewind_delay + rewind_step_delay * abs(diff)@endcode,
     * where @code diff@endcode is the rewind width.
     */
    size_t rewind_delay = 0;

    /**
     * Delay in ns to move the tape head to the next/previous position.
     */
    size_t next_delay = 0;
  };

  /**
   * Stream-based <a href="https://en.wikipedia.org/wiki/Tape_drive">tape</a> emulator.
   * @tparam Stream Stream type. Should be derived either from std::istream or std::ostream.
   * Should be seekable.<br>
   * If the tape is writable, the given stream is extended to the size of the tape.
   * Otherwise, the stream expected to be at least as big as the tape.
   */
  template <typename Stream>
  class tape {
  public:
    /**
     * Indicates if the current tape can be used to read.
     */
    static constexpr bool READABLE = std::derived_from<Stream, std::istream>;
    /**
     * Indicates if the current tape can be used to write.
     */
    static constexpr bool WRITABLE = std::derived_from<Stream, std::ostream>;

    /**
     * Indicates if the current tape can be used to read and write both.
     */
    static constexpr bool BIDIRECTIONAL = READABLE && WRITABLE;

  private:
    static_assert(READABLE || WRITABLE);
    static_assert(std::is_move_constructible_v<Stream>);

    using value_t = int32_t;
    static constexpr ptrdiff_t VALUE_SIZE = sizeof(value_t);

    /**
     * @invariant 0 <= pos <= size
     */
    size_t pos = 0;
    size_t size = 0;
    size_t stream_offset = 0;
    Stream stream;

    /**
     * Indicates if @code buffer@endcode represents the value on the current position.
     */
    bool consistent = false;
    value_t buffer = 0;

    delay_config delays;

  public:
    tape() noexcept(std::is_nothrow_default_constructible_v<Stream>) requires(std::is_default_constructible_v<Stream>)
    = default;

    /**
     * Create the tape from the given stream.
     * @param stream target stream
     * @param size size of the tape
     * @param delays config that defines emulation of operation delays
     * @throws std::invalid_argument if pos > size
     * @throws io_exception if extending fails
     */
    tape(Stream&& stream, const size_t size,
         const delay_config& delays) noexcept(std::is_nothrow_move_constructible_v<Stream>) : tape(
        std::move(stream), size, 0, 0, delays) {}

    /**
     * Create the tape from the given stream.
     * @param stream target stream
     * @param size size of the tape
     * @param pos current position of the tape head
     * @param stream_offset offset in bytes from the beginning of the stream to the position of the first element
     * @param delays config that defines emulation of operation delays
     * @throws std::invalid_argument if pos > size
     * @throws io_exception if extending fails
     */
    tape(Stream&& stream, const size_t size, const size_t pos = 0,
         const size_t stream_offset = 0,
         const delay_config& delays = {}) noexcept(std::is_nothrow_move_constructible_v<Stream>)
      : pos(pos),
        size(size),
        stream_offset(stream_offset),
        stream(std::move(stream)),
        delays(delays) {
      if (pos > size) {
        throw std::invalid_argument("pos <= size expected");
      }
      stream.exceptions(std::ios_base::goodbit);

      extend();
    }

    tape(const tape& other) = delete;

    tape(tape&& other) noexcept(std::is_nothrow_move_constructible_v<Stream>)
      : pos(std::exchange(other.pos, 0)),
        size(std::exchange(other.size, 0)),
        stream_offset(std::exchange(other.stream_offset, 0)),
        stream(std::move(other.stream)),
        consistent(std::exchange(other.consistent, false)),
        buffer(other.buffer),
        delays(std::exchange(other.delays, {})) {}

    tape& operator=(const tape& other) = delete;

    tape& operator=(tape&& other) noexcept(std::is_nothrow_move_assignable_v<Stream>) requires(std::is_move_assignable_v
      <Stream>) {
      if (this != &other) {
        pos = std::exchange(other.pos, 0);
        size = std::exchange(other.size, 0);
        stream_offset = std::exchange(other.stream_offset, 0);
        stream = std::move(other.stream);
        consistent = std::exchange(other.consistent, false);
        buffer = other.buffer;
        delays = other.delays;
      }
      return *this;
    }

    ~tape() noexcept(std::is_nothrow_destructible_v<Stream>) = default;

    /**
     * Checks if the head is at the end of the tape.
     */
    [[nodiscard]] bool is_end() const noexcept {
      return pos == size;
    }

    /**
     * Checks if the head is at the beginning of the tape.
     */
    [[nodiscard]] bool is_begin() const noexcept {
      return pos == 0;
    }

    /**
     * Move head by @code diff@endcode positions.
     * If @code diff < 0@endcode, the head moves backwards.<br>
     * Emulates delay in @code rewind_delay + rewind_step_delay * abs(diff)@endcode ns.
     */
    void seek(const ptrdiff_t diff) {
      seek_impl(diff);
      delay(delays.rewind_delay, delays.rewind_step_delay, std::llabs(diff));
    }

    /**
     * Get the value by the current head position.<br>
     * Emulates delay in @code read_delay@endcode ns.
     * @throws io_exception if reading fails
     */
    value_t get()
      requires(READABLE) {
      assert(!is_end());
      if (!consistent) {
        buffer = get_value();
        consistent = true;
      }

      delay(delays.read_delay);
      return buffer;
    }

    /**
     * Set the value by the current head position.<br>
     * Emulates delay in @code write_delay@endcode ns.
     * @throws io_exception if setting fails
     */
    void set(const value_t new_value)
      requires(WRITABLE) {
      assert(!is_end());

      consistent = false;

      set_value(new_value);

      buffer = new_value;
      consistent = true;

      delay(delays.write_delay);
    }

    /**
     * Move head one position forward.<br>
     * Emulates delay in @code next_delay@endcode ns.<br>
     * Same as @code seek(1)@endcode except delays.
     */
    tape& next() {
      seek_impl(1);
      delay(delays.next_delay);
      return *this;
    }

    /**
     * Move head one position backward.<br>
     * Emulates delay in @code next_delay@endcode ns.<br>
     * Same as @code seek(-1)@endcode except delays.
     */
    tape& prev() {
      seek_impl(-1);
      delay(delays.next_delay);
      return *this;
    }

    /**
     * Flush the stream.
     * @throws io_exception if flushing fails.
     */
    void flush() requires(WRITABLE) {
      stream.flush();
      if (!stream) {
        throw io_exception("error flushing");
      }
    }

    /**
     * Return stream, related to the tape. The stream is set to the first element of the tape.
     * The tape is assigned to the default value.
     * @return stream of the tape
     */
    Stream release() {
      Stream result(std::move(stream));
      pos = size = stream_offset = 0;
      consistent = false;
      delays = {};

      if constexpr (WRITABLE) {
        result.seekp(stream_offset);
      }
      if constexpr (READABLE) {
        result.seekg(stream_offset);
      }

      return result;
    }

    friend void swap(tape& lhs, tape& rhs) noexcept(std::is_nothrow_swappable_v<Stream>) requires (std::is_swappable_v<
      Stream>) {
      using std::swap;
      swap(lhs.pos, rhs.pos);
      swap(lhs.size, rhs.size);
      swap(lhs.stream, rhs.stream);
      swap(lhs.consistent, rhs.consistent);
      swap(lhs.buffer, rhs.buffer);
      swap(lhs.stream_offset, rhs.stream_offset);
      swap(lhs.delays, rhs.delays);
    }

  private:
    static constexpr size_t MAX_SIZE_T = std::numeric_limits<size_t>::max();

    /**
     * Move head by @code diff@endcode positions.
     * If @code diff < 0@endcode, the head moves backwards.
     */
    void seek_impl(const ptrdiff_t diff) noexcept {
      assert(check_diff(diff));
      pos += diff;
      consistent &= (diff == 0);
    }

    /**
     * Check if @code pos += diff@endcode does not destroy the invariants.
     */
    [[nodiscard]] bool check_diff(const ptrdiff_t diff) const noexcept {
      if (diff > 0) {
        // check overflow and the invariats
        return pos <= MAX_SIZE_T - diff && pos + diff <= size;
      }
      return pos >= static_cast<size_t>(-diff);
    }

    /**
     * Read the value from the current head position. Does not move head.
     * @throws io_exception if reading fails
     */
    value_t get_value() requires(READABLE) {
      stream.clear();
      stream.seekg(pos * VALUE_SIZE + stream_offset, std::ios_base::beg);

      value_t buffer = 0;
      auto* buf_ptr = reinterpret_cast<char*>(&buffer);
      stream.read(buf_ptr, VALUE_SIZE);

      if (!stream) {
        throw io_exception("error getting the value");
      }
      return buffer;
    }

    /**
     * Write the @code value@endcode to the current head position. Does not move head.
     * @throws io_exception if setting fails
     */
    void set_value(const value_t value) requires(WRITABLE) {
      stream.clear();
      stream.seekp(pos * VALUE_SIZE + stream_offset, std::ios_base::beg);
      stream.write(reinterpret_cast<const char*>(&value), VALUE_SIZE);

      if (!stream) {
        throw io_exception("error setting the value");
      }
    }

    /**
     * Emulates delay in @code constant_delay@endcode ns
     */
    static void delay(const size_t constant_delay) {
      if (constant_delay == 0)
        return;
      const std::chrono::nanoseconds timespan(constant_delay);
      std::this_thread::sleep_for(timespan);
    }

    /**
     * Emulates delay in @code min(MAX_SIZE_T, constant_delay + step_delay * steps)@endcode ns.
     */
    static void delay(const size_t constant_delay, const size_t step_delay,
                      const size_t steps) {
      size_t result_delay = step_delay * steps;
      if (steps != 0 && result_delay / steps != step_delay) {
        result_delay = MAX_SIZE_T;
      }

      if (result_delay <= MAX_SIZE_T - constant_delay) {
        result_delay += constant_delay;
      } else {
        result_delay = MAX_SIZE_T;
      }
      delay(result_delay);
    }

    /**
     * If WRITABLE, extend stream upto @code size@endcode
     * @throws io_exception if extending fails
     */
    void extend() {
      if constexpr (WRITABLE) {
        stream.seekp(0, std::ios_base::end);

        constexpr value_t value = 0;
        auto buf_ptr = reinterpret_cast<const char*>(&value);

        while (stream && stream.tellp() < size * VALUE_SIZE + stream_offset) {
          stream.write(buf_ptr, VALUE_SIZE);
        }

        if (!stream) {
          throw io_exception("error extending the stream");
        }
      }
    }
  };
} // namespace tape