#include "../lib/include/sorter.h"
#include "../lib/include/tape.h"

#include <filesystem>
#include <utility>
#include <gtest/gtest.h>

#include <random>

class file_guard {
public:
  std::filesystem::path path;

  explicit file_guard(std::filesystem::path path) : path(std::move(path)) {}

  explicit file_guard(const std::filesystem::path& path, const std::string& data) : file_guard(path) {
    std::ofstream out(path);
    out.write(data.data(), data.size());
    out.close();
  }

  [[nodiscard]] bool check_equal(const std::string& data) const {
    std::ifstream in(path);
    std::vector<char> vec(data.size(), 0);
    in.read(vec.data(), vec.size());
    return memcmp(data.data(), vec.data(), data.size()) == 0;
  }

  ~file_guard() noexcept(false) {
    remove(path);
  }
};

constexpr std::string tmpdir = "/tmp/";

template <size_t N>
constexpr std::array<int32_t, N> gen_data() {
  static std::mt19937 gen(std::random_device{}());
  static std::uniform_int_distribution<> distribution;

  std::array<int32_t, N> data{};
  for (int32_t i = 0; i < N; ++i) {
    data[i] = distribution(gen);
  }
  return data;
}

constexpr size_t N = 1000;
auto data = gen_data<N>();

template <typename Stream, size_t N>
void expect_equals(tape::tape<Stream>& tp, const std::array<int32_t, N>& data) {
  EXPECT_TRUE(tp.is_end());
  for (int32_t i = N; i--;) {
    EXPECT_EQ(data[i], tape::helpers::peek(tp));
  }
  EXPECT_TRUE(tp.is_begin());
}

template <typename Stream, size_t N>
void fill(tape::tape<Stream>& tp, const std::array<int32_t, N>& data) {
  EXPECT_TRUE(tp.is_begin());
  for (int32_t i = 0; i < N; ++i) {
    tape::helpers::put(tp, data[i]);
  }
  EXPECT_TRUE(tp.is_end());
}

template <size_t N>
std::string get_string(const std::array<int32_t, N>& data) {
  const auto* buf_ptr = reinterpret_cast<const char*>(data.data());
  constexpr size_t size = N * sizeof(int32_t);
  return {buf_ptr, size};
}

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
void beg_end_test(Stream stream, const size_t n) {
  tape::tape tp(std::move(stream), N);
  for (size_t i = 0; i < n; ++i) {
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
  beg_end_test(std::stringstream(), N);
  beg_end_test(std::ostringstream(), N);

  // filestreams
  const file_guard file_guard(tmpdir + "tape_beg_end.txt", "");
  beg_end_test(std::fstream(file_guard.path), N);
  beg_end_test(std::ofstream(file_guard.path), N);
}

template <typename Stream, size_t N>
void getting_test(Stream stream, const std::array<int32_t, N>& data) {
  tape::tape tp(std::move(stream), N, N);
  expect_equals(tp, data);
}

TEST(tape_tests, get) {
  // stringstreams
  auto str = get_string(data);
  {
    std::stringstream stream(str);
    getting_test(std::move(stream), data);
  }
  {
    std::istringstream stream(str);
    getting_test(std::move(stream), data);
  }

  // filestreams
  file_guard file_guard(tmpdir + "tape_get.txt", str);
  {
    std::fstream stream(file_guard.path);
    getting_test(std::move(stream), data);
  }
  {
    std::ifstream stream(file_guard.path);
    getting_test(std::move(stream), data);
  }
}

template <typename Stream, size_t N>
void setting_test(Stream& stream, const std::array<int32_t, N>& data) {
  tape::tape tp(std::move(stream), N);
  fill(tp, data);
  stream = tp.release();
}

TEST(tape_tests, set) {
  // stringstreams
  auto str = get_string(data);
  {
    std::stringstream stream;
    setting_test(stream, data);
    EXPECT_EQ(str, stream.str());
  }
  {
    std::ostringstream stream;
    setting_test(stream, data);
    EXPECT_EQ(str, stream.str());
  }

  // filestreams
  file_guard file_guard(tmpdir + "tape_set.txt", str);
  {
    std::fstream stream(file_guard.path);
    setting_test(stream, data);
    stream.close();
    EXPECT_TRUE(file_guard.check_equal(str));
  }
  {
    std::ofstream stream(file_guard.path);
    setting_test(stream, data);
    stream.close();
    EXPECT_TRUE(file_guard.check_equal(str));
  }
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

template <typename Stream>
void random_access_test(tape::tape<Stream>& tp) {
  std::array<int32_t, N> data{};
  std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<> index_distribution(0, N - 1);
  std::uniform_int_distribution<> value_distribution;
  std::uniform_int_distribution<> bool_distribution(0, 1);

  size_t index = 0;
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
  {
    tape::tape tp(std::stringstream(), N);
    random_access_test(tp);
  }
  {
    const file_guard file_guard(tmpdir + "tape_ra.txt", "");
    tape::tape tp(std::fstream(file_guard.path), N);
    random_access_test(tp);
  }
}

TEST(tape_tests, file_close_and_open) {
  auto data = gen_data<N>();
  const file_guard file_guard(tmpdir + "tape_cl_op.txt");
  {
    tape::tape tp(std::ofstream(file_guard.path), N);
    fill(tp, data);
    tp.flush();
  }
  {
    std::ifstream in(file_guard.path);
    in.seekg(N * sizeof(int32_t));
    tape::tape tp(std::move(in), N, N);
    expect_equals(tp, data);
  }
}

//todo offset tests
//todo position tests
//todo extending tests
//todo seek and next tests
//todo time tests