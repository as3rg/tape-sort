#pragma once
#include <stdexcept>

namespace tape {
  /**
   * Exception, which is thrown when seeking fails.
   */
  class seek_exception : public std::runtime_error {
  public:
    explicit seek_exception(const std::string& string);

    explicit seek_exception(const char* string);
  };
} // tape