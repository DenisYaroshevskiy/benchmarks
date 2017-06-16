#include <algorithm>
#include <numeric>
#include <iostream>

#include "/space/chromium/src/base/algorithm/sorted_ranges.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"

template <typename Searcher>
void searcher_benchmark(benchmark::State& state, Searcher searcher) {
  std::vector<int> input(state.range(0));
  std::iota(input.begin(), input.end(), 0);
  int looking_for = state.range(1);
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(searcher(input.begin(), input.end(), looking_for));
  }
}

void lower_bound_hinted(benchmark::State& state) {
  searcher_benchmark(state,
                     [hint = state.range(2)](auto f, auto l, auto looking_for) {
    return base::LowerBoundHinted(f, f + hint, l, looking_for);
  });
}

void lower_bound_standard(benchmark::State& state) {
  searcher_benchmark(state, [](auto f, auto l, auto looking_for) {
    return std::lower_bound(f, l, looking_for);
  });
}

void test_cases(benchmark::internal::Benchmark* b) {
  auto add_case = [b](int size, int looking_for, int distance) {
    int hint = (looking_for - distance);
    if (hint < 0)
      hint = looking_for + distance;
    // std::clamp
    hint = std::min(std::max(0, hint), size);
    if (hint > size)
      hint = size;

    b->Args({size, looking_for, hint});
  };

  for (int size : {1000})
    for (int looking_for : {size / 5})
      for (int distance :
           {0, 5, -5, 10, -10, 20, -20, 50, -50, size / 10, size / 2})
        add_case(size, looking_for, distance);

  add_case(1000, 1000, 1000);
  add_case(1000, 0, 1000);
}

BENCHMARK(lower_bound_hinted)->Apply(test_cases);
BENCHMARK(lower_bound_standard)->Apply(test_cases);

BENCHMARK_MAIN();
