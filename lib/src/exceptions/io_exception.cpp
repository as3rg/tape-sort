#include "../../include/exceptions/io_exception.h"

namespace tape {
  io_exception::io_exception(const std::string& string): runtime_error(string) {}

  io_exception::io_exception(const char* string): runtime_error(string) {}
} // tape