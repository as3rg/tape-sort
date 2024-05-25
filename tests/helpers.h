#pragma once

#include "../lib/include/sorter.h"
#include "../lib/include/tape.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <random>

class time_checker {
private:
  std::chrono::steady_clock::time_point current;

public:
  time_checker();

  int64_t checkpoint();
};

template <size_t N>
std::string get_string(const std::array<int32_t, N>& data) {
  const auto* buf_ptr = reinterpret_cast<const char*>(data.data());
  constexpr size_t size = N * sizeof(int32_t);
  return {buf_ptr, size};
}

template <size_t N>
auto gen_data() {
  static std::mt19937 gen(std::random_device{}());
  static std::uniform_int_distribution distribution(std::numeric_limits<int32_t>::min());

  std::array<int32_t, N> data{};
  for (int32_t i = 0; i < N; ++i) {
    data[i] = distribution(gen);
  }
  return data;
}

template <size_t N>
auto gen_data_pair() {
  auto data = gen_data<N>();
  auto str = get_string(data);
  return std::make_pair(data, str);
}

template <size_t N>
void expect_equals(const std::filesystem::path& path, const std::array<int32_t, N>& data) {
  std::ifstream in(path);
  std::array<int32_t, N> fdata;
  in.read(reinterpret_cast<char*>(fdata.data()), N * sizeof(int32_t));
  EXPECT_EQ(data, fdata);
}

template <typename Stream, size_t N>
void expect_equals(tape::tape<Stream>& tp, const std::array<int32_t, N>& data, const size_t offset, const size_t size) {
  EXPECT_TRUE(tp.is_end());
  for (size_t i = size; i--;) {
    EXPECT_EQ(data[i + offset], tape::helpers::peek(tp));
  }
  EXPECT_TRUE(tp.is_begin());
}

template <typename Stream, size_t N>
void expect_equals(tape::tape<Stream>& tp, const std::array<int32_t, N>& data, const size_t offset = 0) {
  expect_equals(tp, data, offset, N - offset);
}

template <typename Stream, size_t N>
void fill(tape::tape<Stream>& tp, const std::array<int32_t, N>& data, const size_t offset, const size_t size) {
  EXPECT_TRUE(tp.is_begin());
  for (size_t i = 0; i < size; ++i) {
    tape::helpers::put(tp, data[i + offset]);
  }
  EXPECT_TRUE(tp.is_end());
}

template <typename Stream, size_t N>
void fill(tape::tape<Stream>& tp, const std::array<int32_t, N>& data, const size_t offset = 0) {
  fill(tp, data, offset, N - offset);
}

std::string get_file_name(const std::string& suffix = "");