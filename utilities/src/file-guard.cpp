#include "../include/file-guard.h"

#include <fstream>
#include <iostream>
#include <utility>

file_guard::file_guard(std::filesystem::path path, const std::string& initial_data) : path_(std::move(path)) {
  if (path_.has_relative_path()) {
    create_directories(path_.parent_path());
  }
  std::ofstream out(path_);
  out.write(initial_data.data(), initial_data.size());
  out.close();
}

file_guard::file_guard(file_guard&& other) noexcept : path_(std::exchange(other.path_, "")) {}

file_guard& file_guard::operator=(file_guard&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  std::exchange(path_, other.path_);
  return *this;
}

file_guard::~file_guard() {
  std::error_code ec;
  if (!path_.empty() && !remove(path_, ec)) {
    std::cerr << "error while deleting temporary file " << path_ << ": " << ec << std::endl;
  }
}