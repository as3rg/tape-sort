#include "helpers.h"

time_checker::time_checker() : current(std::chrono::steady_clock::now()) {}

int64_t time_checker::checkpoint() {
  const auto new_cur = std::chrono::steady_clock::now();
  const auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(new_cur - current).count();
  current = new_cur;
  return diff;
}

std::string get_file_name(const std::string& suffix) {
  return std::string("./tmp/tape_") + testing::UnitTest::GetInstance()->current_test_suite()->name() + "_" +
         testing::UnitTest::GetInstance()->current_test_info()->name() + "_" + suffix + ".txt";
}