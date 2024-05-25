#include "helpers.h"

file_guard::file_guard(std::filesystem::path path): path(std::move(path)) {
  if (this->path.has_relative_path()) {
    create_directories(this->path.parent_path());
  }
}

file_guard::file_guard(const std::filesystem::path& path, const std::string& initial_data): file_guard(path) {
  std::ofstream out(path);
  out.write(initial_data.data(), initial_data.size());
  out.close();
}

file_guard::~file_guard() {
  std::error_code ec;
  if (!remove(path, ec)) {
    std::cerr << "error deleting temporary file " << path << ": " << ec << std::endl;
  }
}

time_checker::time_checker() : current(std::chrono::steady_clock::now()) {}

int64_t time_checker::checkpoint() {
  const auto new_cur = std::chrono::steady_clock::now();
  const auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>( new_cur - current).count();
  current = new_cur;
  return diff;
}

std::string get_file_name(const std::string& suffix) {
  return std::string("./tmp/tape_") + testing::UnitTest::GetInstance()->current_test_suite()->name() + "_" +
         testing::UnitTest::GetInstance()->current_test_info()->name() + "_" + suffix + ".txt";
}