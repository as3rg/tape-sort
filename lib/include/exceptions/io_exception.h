#pragma once
#include <stdexcept>

namespace tape {
  /**
   * Exception, which is thrown when reading, writing or flushing fails.
   */
  class io_exception : public std::runtime_error {
  public:
    explicit io_exception(const std::string& string);

    explicit io_exception(const char* string);
  };
} // namespace tape