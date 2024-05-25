#pragma once

#include <filesystem>

class file_guard {
private:
  std::filesystem::path path_;

public:
  explicit file_guard(std::filesystem::path path, const std::string& initial_data = "");

  file_guard(const file_guard& other) = delete;

  file_guard(file_guard&& other) noexcept;

  file_guard& operator=(const file_guard& other) = delete;

  file_guard& operator=(file_guard&& other) noexcept;

  [[nodiscard]] std::filesystem::path path() const noexcept {
    return path_;
  }

  ~file_guard();
};