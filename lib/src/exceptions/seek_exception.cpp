#include "../../include/exceptions/seek_exception.h"

namespace tape {
  seek_exception::seek_exception(const std::string& string): runtime_error(
      string) {}

  seek_exception::seek_exception(const char* string): runtime_error(string) {}
} // tape