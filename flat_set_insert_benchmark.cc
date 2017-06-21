#include "benchmarks/insert_algorithms.h"

#include <algorithm>
#include <random>
#include <vector>

#include "third_party/benchmark/include/benchmark/benchmark.h"

namespace {

constexpr auto small_set_size = 1000u;
constexpr auto small_insert_size = 20u;

const auto& test_case_small_int_set() {
  static const std::pair<std::vector<int>, std::vector<int>> cached_res = [] {
    std::vector<int> already_in(small_set_size);
    std::vector<int> new_elements(small_insert_size);
    auto random_number = [] {
      static std::mt19937 g;
      static std::uniform_int_distribution<> dis(1, 10000);
      return dis(g);
    };

    std::generate(already_in.begin(), already_in.end(), random_number);
    std::sort(already_in.begin(), already_in.end());
    std::generate(new_elements.begin(), new_elements.end(), random_number);
    return std::make_pair(std::move(already_in), std::move(new_elements));
  }();

  return cached_res;
}

template <typename F>
// requires PureFunction<F>
void benchmark_unique_insert(benchmark::State& state, F insertion_algorithm) {
  const auto& input = test_case_small_int_set();
  while(state.KeepRunning()) {
    auto c = input.first;
    insertion_algorithm(c, input.second.begin(), input.second.end());
  }
}

void benchmark_do_nothing(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto&&...) {});
}

void benchmark_one_at_a_time(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::one_at_a_time(c, f, l, std::less<>{});
  });
}

void benchmark_stable_sort_and_unique(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::stable_sort_and_unique(c, f, l, std::less<>{});
  });
}

void benchmark_naive_inplace_merge(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::naive_inplace_merge(c, f, l, std::less<>{});
  });
}

void benchmark_copy_unique_full_inplace_merge(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::copy_unique_full_inplace_merge(c, f, l, std::less<>{});
  });
}

void benchmark_copy_unique_inplace_merge_begin(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::copy_unique_inplace_merge_begin(c, f, l, std::less<>{});
  });
}

void benchmark_copy_unique_inplace_merge_upper_bound(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::copy_unique_inplace_merge_upper_bound(c, f, l, std::less<>{});
  });
}

void benchmark_copy_unique_inplace_merge_no_buffer(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::copy_unique_inplace_merge_no_buffer(c, f, l, std::less<>{});
  });
}

BENCHMARK(benchmark_do_nothing);
BENCHMARK(benchmark_one_at_a_time);
BENCHMARK(benchmark_stable_sort_and_unique);
BENCHMARK(benchmark_naive_inplace_merge);
BENCHMARK(benchmark_copy_unique_full_inplace_merge);
BENCHMARK(benchmark_copy_unique_inplace_merge_begin);
BENCHMARK(benchmark_copy_unique_inplace_merge_upper_bound);
BENCHMARK(benchmark_copy_unique_inplace_merge_no_buffer);

}  // namespace


BENCHMARK_MAIN();
