#pragma once

#include <cassert>
#include <fstream>
#include <limits>
#include <thread>

#include "exceptions/io_exception.h"
#include "exceptions/seek_exception.h"

namespace tape {
  /**
   * Stream-based <a href="https://en.wikipedia.org/wiki/Tape_drive">tape</a> emulator.
   * @tparam Stream Stream type. Should be derived either from std::istream or std::ostream.
   * Should be seekable.
   */
  template <typename Stream>
  class tape {
  public:
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

    using value_t = int32_t;
    static constexpr ptrdiff_t VALUE_SIZE = sizeof(value_t);

    /**
     * @invariant 0 <= pos <= size
     */
    size_t pos;
    size_t size;
    Stream stream;

    /**
     * IIndicates if @code buffer@endcode represents the value on the current position.
     */
    bool consistent = false;
    value_t buffer = 0;
    const size_t stream_offset_;

    const delay_config delays_;

  public:
    /**
     * Create the tape from the given stream.
     * @param stream target stream
     * @param size size of the tape
     * @param pos current position of the tape head
     * @param stream_offset offset in bytes from the beginning of the stream to the position of the first element
     * @param delays config that defines emulation of operation delays
     * @throws std::invalid_argument if pos > size
     */
    tape(Stream&& stream, const size_t size, const size_t pos = 0,
         const size_t stream_offset = 0, const delay_config& delays = {})
      : pos(pos),
        size(size),
        stream(std::move(stream)),
        stream_offset_(stream_offset),
        delays_(delays) {
      if (pos > size) {
        throw std::invalid_argument("pos <= size expected");
      }
      stream.exceptions(std::ios_base::goodbit);
      seek_impl(0);
    }

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
     * @throws seek_exception if seeking fails.
     * Invariants may be destroyed until successfull @code seek()@endcode, @code next()@endcode or @code prev()@endcode
     */
    void seek(const ptrdiff_t diff) {
      seek_impl(diff);
      delay(delays_.rewind_delay, delays_.rewind_step_delay, std::llabs(diff));
    }

    /**
     * Get the value by the current head position.<br>
     * Emulates delay in @code read_delay@endcode ns.
     * @throws io_exception if reading fails.
     * No invariants are destroyed.
     * @throws seek_exception if seeking fails.
     * Invariants may be destroyed until successfull @code seek()@endcode, @code next()@endcode or @code prev()@endcode
     */
    value_t get()
      requires(READABLE) {
      assert(!is_end());
      if (!consistent) {
        buffer = get_value();
        consistent = true;
      }
      delay(delays_.read_delay);
      return buffer;
    }

    /**
     * Set the value by the current head position.<br>
     * Emulates delay in @code write_delay@endcode ns.
     * @throws io_exception if setting fails.
     * No invariants are destroyed, but the current value is undefined.
     * @throws seek_exception if seeking fails.
     * Invariants may be destroyed until successfull @code seek()@endcode, @code next()@endcode or @code prev()@endcode
     */
    void set(const value_t new_value)
      requires(WRITABLE) {
      assert(!is_end());
      consistent = false;
      set_value(new_value);
      buffer = new_value;
      consistent = true;
      delay(delays_.write_delay);
    }

    /**
     * Move head one position forward.<br>
     * Emulates delay in @code next_delay@endcode ns.<br>
     * Same as @code seek(1)@endcode except delays.
     * @throws seek_exception if seeking fails.
     * Invariants may be destroyed until successfull @code seek()@endcode, @code next()@endcode or @code prev()@endcode
     */
    tape& next() {
      seek_impl(1);
      delay(delays_.next_delay);
      return *this;
    }

    /**
     * Move head one position backward.<br>
     * Emulates delay in @code next_delay@endcode ns.<br>
     * Same as @code seek(-1)@endcode except delays.
     * @throws seek_exception if seeking fails.
     * Invariants may be destroyed until successfull @code seek()@endcode, @code next()@endcode or @code prev()@endcode
     */
    tape& prev() {
      seek_impl(-1);
      delay(delays_.next_delay);
      return *this;
    }

    /**
     * Flush the stream.
     * @throws io_exception if flushing fails.
     */
    void flush() {
      stream.flush();
      if (!stream) {
        throw io_exception("error flushing");
      }
    }

    friend void swap(tape& lhs, tape& rhs) noexcept(
      std::is_nothrow_swappable_v<decltype(stream)>) {
      using std::swap;
      swap(lhs.pos, rhs.pos);
      swap(lhs.size, rhs.size);
      swap(lhs.stream, rhs.stream);
      swap(lhs.consistent, rhs.consistent);
      swap(lhs.buffer, rhs.buffer);
    }

  private:
    static constexpr size_t MAX_SIZE_T = std::numeric_limits<size_t>::max();

    /**
     * Move head by @code diff@endcode positions.
     * If @code diff < 0@endcode, the head moves backwards.
     * @throws seek_exception if seeking fails.
     * Invariants may be destroyed until successfull @code seek()@endcode, @code next()@endcode or @code prev()@endcode
     */
    void seek_impl(const ptrdiff_t diff) {
      assert(check_diff(diff));
      pos += diff;
      consistent &= (diff == 0);

      stream.clear();
      size_t file_pos = stream_offset_ + pos * VALUE_SIZE;
      if constexpr (WRITABLE) {
        stream.seekp(file_pos, std::ios_base::beg);
      }
      if constexpr (READABLE) {
        stream.seekg(file_pos, std::ios_base::beg);
      }
      if (!stream) {
        throw seek_exception("error seeking the stream");
      }
    }

    /**
     * Check if @code pos += diff@endcode does not destroy the invariants.
     */
    [[nodiscard]] bool check_diff(const ptrdiff_t diff) const {
      if (diff > 0) {
        // check overflow and the invariats
        return pos <= MAX_SIZE_T - diff && pos + diff <= size;
      }
      return pos >= static_cast<size_t>(-diff);
    }

    /**
     * Read the value from the current head position. Does not move head.
     * @throws io_exception if reading fails.
     * No invariants are destroyed.
     * @throws seek_exception if seeking fails.
     * Invariants may be destroyed until successfull @code seek()@endcode, @code next()@endcode or @code prev()@endcode
     */
    value_t get_value()
      requires(READABLE) {
      value_t buffer = 0;
      stream.read(reinterpret_cast<char*>(&buffer), VALUE_SIZE);

      if (stream.eof()) {
        buffer = 0;
        stream.clear();
      }

      const bool fail = !stream;
      seek_impl(0);

      if (fail) {
        throw io_exception("error getting the value");
      }
      return buffer;
    }

    /**
     * Write the @code value@endcode to the current head position. Does not move head.
     * @throws io_exception if setting fails.
     * No invariants are destroyed, but the current value is undefined.
     * @throws seek_exception if seeking fails.
     * Invariants may be destroyed until successfull @code seek()@endcode, @code next()@endcode or @code prev()@endcode
     */
    void set_value(const value_t value)
      requires(WRITABLE) {
      stream.write(reinterpret_cast<const char*>(&value), VALUE_SIZE);

      const bool fail = !stream;
      seek_impl(0);

      if (fail) {
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
  };
} // namespace tape