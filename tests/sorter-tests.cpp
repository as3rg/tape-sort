#include "helpers.h"

#include "../utilities/include/file-guard.h"

constexpr size_t N = 100;

auto cmp = std::less<int32_t>{};
auto rev_cmp = std::greater<int32_t>{};

template <size_t MOD>
auto mod_cmp = [](const int32_t l, const int32_t r) {
  return (l % MOD) < (r % MOD);
};

size_t bit_cnt(uint32_t v) {
  size_t res = 0;
  while (v) {
    res += v & 1;
    v >>= 1;
  }
  return res;
}

auto bit_cnt_cmp = [](const int32_t l, const int32_t r) {
  return bit_cnt(l) < bit_cnt(r);
};

auto unsigned_cmp = [](const uint32_t l, const uint32_t r) {
  return l < r;
};

std::vector<std::function<bool(int32_t, int32_t)>> comps{
    cmp,
    rev_cmp,
    mod_cmp<2>,
    mod_cmp<239>,
    bit_cnt_cmp,
    unsigned_cmp
};

template <typename T, typename Compare>
void check_part(tape::tape<T>& src, const tape::helpers::subarray_info<Compare> info,
                const std::vector<int32_t>& expected) {
  auto data = tape::helpers::tape_to_vec(src, info.size());
  std::sort(data.begin(), data.end());

  EXPECT_EQ(info.size(), data.size());
  if (info.size() != 0) {
    EXPECT_NE(std::find(expected.cbegin(), expected.cend(), info.element()), expected.cend());
  }
  EXPECT_EQ(data, expected);
}

template <typename It, typename Pred>
std::vector<int32_t> filtered(It begin, size_t size, Pred pred) {
  std::vector<int32_t> result(size);
  It end = begin + size;
  result.erase(std::copy_if(begin, begin + size, result.begin(), pred), result.end());
  std::sort(result.begin(), result.end());
  return result;
}

template <typename SrcStream, typename LeftStream, typename RightStream, typename Compare>
void split_test(SrcStream src_stream, LeftStream left_stream, RightStream right_stream, Compare compare) {
  tape::tape src(std::move(src_stream), N);
  tape::tape left(std::move(left_stream), N);
  tape::tape right(std::move(right_stream), N);

  auto data = gen_data<N>();
  fill(src, data);
  const auto key = data[N / 2] + 1;

  auto [linfo, rinfo] = tape::helpers::split(src, left, right, compare, key, N);
  EXPECT_TRUE(src.is_begin());

  check_part(left, linfo, filtered(data.begin(), N, [compare, key](int32_t v) {
    return compare(v, key);
  }));

  check_part(right, rinfo, filtered(data.begin(), N, [compare, key](int32_t v) {
    return !compare(v, key);
  }));
}

TEST(sorter_tests, split) {
  const file_guard fout(get_file_name("out"));
  const file_guard fleft(get_file_name("left"));
  const file_guard fright(get_file_name("right"));

  for (size_t i = 0; i < 10; ++i) {
    for (const auto& cmp : comps) {
      split_test(std::stringstream(), std::stringstream(), std::stringstream(), cmp);
      split_test(std::fstream(fout.path()), std::fstream(fleft.path()), std::fstream(fright.path()), cmp);

      split_test(std::fstream(fout.path()), std::stringstream(), std::stringstream(), cmp);
      split_test(std::stringstream(), std::fstream(fleft.path()), std::fstream(fright.path()), cmp);
    }
  }
}

template <typename InStream, typename OutStream, typename Compare, typename Sort>
void sort_test(InStream in_stream, OutStream out_stream, Compare compare, Sort sort) {
  tape::tape in(std::move(in_stream), N);
  tape::tape out(std::move(out_stream), N);

  auto data = gen_data<N>();
  fill(in, data);
  in.seek(-N);

  sort(in, out, compare);

  auto vec = tape::helpers::tape_to_vec(out, N);
  std::reverse(vec.begin(), vec.end());
  for (size_t i = 0; i < N - 1; ++i) {
    EXPECT_FALSE(compare(vec[i + 1], vec[i]));
  }
}

template <typename TIn, typename TOut, typename Compare>
void sort_test1(TIn in_stream, TOut out_stream, Compare compare) {
  sort_test(std::move(in_stream), std::move(out_stream), compare, &tape::sort<TIn, TOut, Compare>);
}

TEST(sorter_tests, sort1) {
  const file_guard fout(get_file_name("out"));
  const file_guard fin(get_file_name("in"));

  for (size_t i = 0; i < 10; ++i) {
    for (const auto& cmp : comps) {
      sort_test1(std::stringstream(), std::stringstream(), cmp);
      sort_test1(std::fstream(fin.path()), std::fstream(fout.path()), cmp);

      sort_test1(std::fstream(fin.path()), std::stringstream(), cmp);
      sort_test1(std::stringstream(), std::fstream(fout.path()), cmp);
    }
  }
}

template <typename TIn, typename TOut, typename T1, typename T2, typename T3, typename Compare>
void sort_test2(TIn in_stream, TOut out_stream, T1 tmp1_stream, T2 tmp2_stream, T3 tmp3_stream, const size_t chunk_size,
                Compare compare) {
  tape::tape tmp1(std::move(tmp1_stream), N);
  tape::tape tmp2(std::move(tmp2_stream), N);
  tape::tape tmp3(std::move(tmp3_stream), N);
  sort_test(std::move(in_stream), std::move(out_stream), compare,
            [&tmp1, &tmp2, &tmp3, chunk_size](auto& in, auto& out, Compare cmp) {
              return tape::sort(in, out, tmp1, tmp2, tmp3, chunk_size, cmp);
            });
  EXPECT_TRUE(tmp1.is_begin());
  EXPECT_TRUE(tmp2.is_begin());
  EXPECT_TRUE(tmp3.is_begin());
}

TEST(sorter_tests, sort2) {
  const file_guard fin(get_file_name("in"));
  const file_guard fout(get_file_name("out"));

  const file_guard ftmp1(get_file_name("tmp1"));
  const file_guard ftmp2(get_file_name("tmp2"));
  const file_guard ftmp3(get_file_name("tmp3"));

  for (size_t i = 0; i < 10; ++i) {
    for (size_t chunk = 1; chunk < N; chunk <<= 1) {
      for (const auto& cmp : comps) {
        sort_test2(std::stringstream(), std::stringstream(), std::stringstream(), std::stringstream(),
                   std::stringstream(), chunk, cmp);
        sort_test2(std::fstream(fin.path()), std::fstream(fout.path()), std::fstream(ftmp1.path()), std::fstream(ftmp2.path()),
                   std::fstream(ftmp3.path()), chunk, cmp);

        sort_test2(std::stringstream(), std::stringstream(), std::fstream(ftmp1.path()), std::fstream(ftmp2.path()),
                   std::fstream(ftmp3.path()), chunk, cmp);
        sort_test2(std::fstream(fin.path()), std::fstream(fout.path()), std::stringstream(), std::stringstream(),
                   std::stringstream(), chunk, cmp);
      }
    }
  }
}

TEST(sorter_tests, uniform_distribution) {
  constexpr size_t REPEATS = 100000;
  std::array<size_t, N> hist{};
  for (size_t i = 0; i < REPEATS; ++i) {
    tape::helpers::subarray_info info(cmp);
    for (int32_t n = 0; n < N; ++n) {
      info.update(n);
    }
    ++hist[info.element()];
  }
  const double mean = REPEATS * 1.0 / N;
  for (size_t i = 0; i < N; ++i) {
    EXPECT_NEAR(hist[i], mean, mean / 2);
  }
}