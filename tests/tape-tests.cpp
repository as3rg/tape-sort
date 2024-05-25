#include "helpers.h"

#include "../utilities/include/file-guard.h"

constexpr size_t N = 1000;
constexpr size_t STEP = 31;

TEST(tape_tests, readable_writable) {
  {
    using tp = tape::tape<std::istringstream>;
    EXPECT_TRUE(tp::READABLE);
    EXPECT_FALSE(tp::WRITABLE);
    EXPECT_FALSE(tp::BIDIRECTIONAL);
  }

  {
    using tp = tape::tape<std::ostringstream>;
    EXPECT_FALSE(tp::READABLE);
    EXPECT_TRUE(tp::WRITABLE);
    EXPECT_FALSE(tp::BIDIRECTIONAL);
  }

  {
    using tp = tape::tape<std::stringstream>;
    EXPECT_TRUE(tp::READABLE);
    EXPECT_TRUE(tp::WRITABLE);
    EXPECT_TRUE(tp::BIDIRECTIONAL);
  }
  {
    using tp = tape::tape<std::ifstream>;
    EXPECT_TRUE(tp::READABLE);
    EXPECT_FALSE(tp::WRITABLE);
    EXPECT_FALSE(tp::BIDIRECTIONAL);
  }

  {
    using tp = tape::tape<std::ofstream>;
    EXPECT_FALSE(tp::READABLE);
    EXPECT_TRUE(tp::WRITABLE);
    EXPECT_FALSE(tp::BIDIRECTIONAL);
  }

  {
    using tp = tape::tape<std::fstream>;
    EXPECT_TRUE(tp::READABLE);
    EXPECT_TRUE(tp::WRITABLE);
    EXPECT_TRUE(tp::BIDIRECTIONAL);
  }
}

template <typename Stream>
void beg_end_test(Stream stream, const size_t n, const size_t pos = 0) {
  tape::tape tp(std::move(stream), n, pos);
  for (size_t i = pos; i < n; ++i) {
    EXPECT_EQ(tp.is_begin(), i == 0);
    EXPECT_FALSE(tp.is_end());
    tp.next();
  }

  EXPECT_FALSE(tp.is_begin());
  EXPECT_TRUE(tp.is_end());

  for (size_t i = 0; i < n; ++i) {
    EXPECT_FALSE(tp.is_begin());
    EXPECT_EQ(tp.is_end(), i == 0);
    tp.prev();
  }

  EXPECT_TRUE(tp.is_begin());
  EXPECT_FALSE(tp.is_end());
}

TEST(tape_tests, begin_end) {
  for (size_t pos = 0; pos < N; pos += STEP) {
    auto [data, str] = gen_data_pair<N>();
    beg_end_test(std::stringstream(str), N, pos);
    beg_end_test(std::ostringstream(str), N, pos);
    beg_end_test(std::istringstream(str), N, pos);
  }

  for (size_t pos = 0; pos < N; pos += STEP) {
    auto [data, str] = gen_data_pair<N>();
    const file_guard file_guard(get_file_name(), str);
    beg_end_test(std::fstream(file_guard.path()), N, pos);
    beg_end_test(std::ofstream(file_guard.path()), N, pos);
    beg_end_test(std::ifstream(file_guard.path()), N, pos);
  }
}

template <typename Stream, size_t N>
void pos_test(Stream stream, size_t pos, const std::array<int32_t, N>& data) {
  tape::tape tp(std::move(stream), N, pos);
  EXPECT_EQ(tp.get(), data[pos]);
}

TEST(tape_tests, initial_pos) {
  auto [data, str] = gen_data_pair<N>();
  const file_guard file_guard(get_file_name(), str);
  for (size_t pos = 0; pos < N; ++pos) {
    pos_test(std::stringstream(str), pos, data);
    pos_test(std::istringstream(str), pos, data);
    pos_test(std::fstream(file_guard.path()), pos, data);
    pos_test(std::ifstream(file_guard.path()), pos, data);
  }
}

template <typename Stream, size_t N>
void offset_test(Stream stream, size_t offset, const std::array<int32_t, N>& data) {
  const size_t size = N - offset;
  tape::tape tp(std::move(stream), size, size, offset * sizeof(int32_t));
  expect_equals(tp, data, offset, size);
}

TEST(tape_tests, offset) {
  auto [data, str] = gen_data_pair<N>();
  const file_guard file_guard(get_file_name(), str);
  for (size_t offset = 0; offset < N; offset += STEP) {
    offset_test(std::stringstream(str), offset, data);
    offset_test(std::istringstream(str), offset, data);
    offset_test(std::fstream(file_guard.path()), offset, data);
    offset_test(std::ifstream(file_guard.path()), offset, data);
  }
}

template <typename StringStream, typename FileStream>
void get_test() {
  {
    auto [data, str] = gen_data_pair<N>();
    tape::tape tp(StringStream(str), N, N);
    expect_equals(tp, data);
  }

  {
    auto [data, str] = gen_data_pair<N>();
    const file_guard file_guard(get_file_name(), str);
    tape::tape tp(FileStream(file_guard.path()), N, N);
    expect_equals(tp, data);
  }
}

TEST(tape_tests, get) {
  get_test<std::stringstream, std::fstream>();
  get_test<std::istringstream, std::ifstream>();
}

template <typename StringStream, typename FileStream>
void set_test() {
  {
    auto [data, str] = gen_data_pair<N>();
    tape::tape tp(StringStream(), N);
    fill(tp, data);
    EXPECT_EQ(str, tp.release().str());
  }

  {
    auto [data, str] = gen_data_pair<N>();
    const file_guard file_guard(get_file_name(), str);
    tape::tape tp(FileStream(file_guard.path()), N);
    fill(tp, data);
    tp.release();
    expect_equals(file_guard.path(), data);
  }
}

TEST(tape_tests, set) {
  set_test<std::stringstream, std::fstream>();
  set_test<std::ostringstream, std::ofstream>();
}

TEST(tape_tests, swap) {
  auto data1 = gen_data<N>();
  tape::tape tp1(std::stringstream(), N);
  fill(tp1, data1);

  auto data2 = gen_data<N>();
  tape::tape tp2(std::stringstream(), N);
  fill(tp2, data2);

  swap(tp1, tp2);

  expect_equals(tp1, data2);
  expect_equals(tp2, data1);
}

TEST(tape_tests, move_ctr) {
  auto data1 = gen_data<N>();
  tape::tape tp1(std::stringstream(), N);
  fill(tp1, data1);

  tape::tape tp2(std::move(tp1));

  ASSERT_TRUE(tp1.is_begin());
  ASSERT_TRUE(tp1.is_end());
  expect_equals(tp2, data1);
}

TEST(tape_tests, move_assignment) {
  auto data1 = gen_data<N>();
  tape::tape tp1(std::stringstream(), N);
  fill(tp1, data1);

  tape::tape<std::stringstream> tp2;
  ASSERT_TRUE(tp2.is_begin());
  ASSERT_TRUE(tp2.is_end());

  tp2 = std::move(tp1);

  ASSERT_TRUE(tp1.is_begin());
  ASSERT_TRUE(tp1.is_end());
  expect_equals(tp2, data1);
}

TEST(tape_tests, release) {
  auto data1 = gen_data<N>();
  tape::tape tp1(std::stringstream(), N);
  fill(tp1, data1);

  auto stream = tp1.release();
  EXPECT_TRUE(tp1.is_begin());
  EXPECT_TRUE(tp1.is_end());

  tape::tape tp2(std::move(stream), N);
  tp2.seek(N);
  expect_equals(tp2, data1);
}

template <typename Stream>
void seek_by_one(tape::tape<Stream>& tp, ptrdiff_t diff) {
  for (; diff > 0; --diff) {
    tp.next();
  }
  for (; diff < 0; ++diff) {
    tp.prev();
  }
}

template <size_t N, typename Stream>
void random_access_test(Stream stream, size_t pos) {
  tape::tape tp(std::move(stream), N, pos);

  std::array<int32_t, N> data{};
  std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<> index_distribution(0, N - 1);
  std::uniform_int_distribution<> value_distribution;
  std::uniform_int_distribution<> bool_distribution(0, 1);

  size_t index = pos;
  for (size_t i = 0; i < 10000; ++i) {
    const size_t new_index = index_distribution(gen);

    if (bool_distribution(gen)) {
      seek_by_one(tp, new_index - index);
    } else {
      tp.seek(new_index - index);
    }
    index = new_index;

    EXPECT_EQ(tp.get(), data[new_index]);

    const int32_t new_value = value_distribution(gen);
    data[new_index] = new_value;
    tp.set(new_value);

    EXPECT_EQ(tp.get(), data[new_index]);
  }
}

TEST(tape_tests, random_access) {
  for (size_t pos = 0; pos < N; pos += STEP) {
    random_access_test<N>(std::stringstream(), pos);

    const file_guard file_guard(get_file_name());
    random_access_test<N>(std::fstream(file_guard.path()), pos);
  }
}

TEST(tape_tests, file_close_and_open) {
  auto [data, str] = gen_data_pair<N>();
  const file_guard file_guard(get_file_name());
  {
    tape::tape tp(std::ofstream(file_guard.path()), N);
    fill(tp, data);
    tp.flush();
  }
  {
    std::ifstream in(file_guard.path());
    in.seekg(N * sizeof(int32_t));
    tape::tape tp(std::move(in), N, N);
    expect_equals(tp, data);
  }
}

void check_time(time_checker& checker, const size_t target, const size_t error) {
  const int64_t dur = checker.checkpoint();
  ASSERT_GE(dur, target);
  ASSERT_LE(dur, target + error);
}

TEST(tape_tests, delays) {
  constexpr tape::delay_config target_delays{
      .read_delay = 20'000'000ull,
      .write_delay = 30'000'000ull,
      .rewind_step_delay = 10'000'000ull,
      .rewind_delay = 10'000'000ull,
      .next_delay = 40'000'000ull
  };
  constexpr size_t error = 5'000'000;

  tape::tape tp(std::stringstream(), N, target_delays);

  time_checker checker;
  for (int64_t i = 0; i < 20; ++i) {
    tp.get();
    check_time(checker, target_delays.read_delay, error);

    tp.set(0);
    check_time(checker, target_delays.write_delay, error);

    tp.next();
    check_time(checker, target_delays.next_delay, error);
    tp.prev();
    check_time(checker, target_delays.next_delay, error);

    tp.seek(i);
    check_time(checker, target_delays.rewind_delay + target_delays.rewind_step_delay * i, error);
    tp.seek(-i);
    check_time(checker, target_delays.rewind_delay + target_delays.rewind_step_delay * i, error);
  }
}