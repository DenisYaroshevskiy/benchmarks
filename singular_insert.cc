#include <algorithm>
#include <numeric>
#include <iostream>

#include "benchmarks/insert_algorithms.h"

#include "third_party/benchmark/include/benchmark/benchmark.h"

template <typename Searcher>
void searcher_benchmark(benchmark::State& state, Searcher searcher) {
  std::vector<int> input(1000u);
  std::iota(input.begin(), input.end(), 0);
  int looking_for = state.range(0);
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(searcher(input.begin(), input.end(), looking_for));
  }
}

void lower_bound_linear(benchmark::State& state) {
  searcher_benchmark(state, [](auto f, auto l, auto looking_for) {
    auto greater_eq =
        helpers::not_fn(helpers::less_than(std::less<>{}, looking_for));
    return std::find_if(f, l, greater_eq);
  });
}

void lower_bound_biased(benchmark::State& state) {
  searcher_benchmark(state, [](auto f, auto l, auto looking_for) {
    return helpers::lower_bound_biased(f, l, looking_for, std::less<>{});
  });
}

void lower_bound_standard(benchmark::State& state) {
  searcher_benchmark(state, [](auto f, auto l, auto looking_for) {
    return std::lower_bound(f, l, looking_for);
  });
}

BENCHMARK(lower_bound_linear)->Arg(2);
BENCHMARK(lower_bound_biased)->Arg(2);
BENCHMARK(lower_bound_standard)->Arg(2);

BENCHMARK(lower_bound_linear)->Arg(10);
BENCHMARK(lower_bound_biased)->Arg(10);
BENCHMARK(lower_bound_standard)->Arg(10);

BENCHMARK(lower_bound_linear)->Arg(50);
BENCHMARK(lower_bound_biased)->Arg(50);
BENCHMARK(lower_bound_standard)->Arg(50);

BENCHMARK(lower_bound_linear)->Arg(200);
BENCHMARK(lower_bound_biased)->Arg(200);
BENCHMARK(lower_bound_standard)->Arg(200);

BENCHMARK(lower_bound_linear)->Arg(350);
BENCHMARK(lower_bound_biased)->Arg(350);
BENCHMARK(lower_bound_standard)->Arg(350);

BENCHMARK_MAIN();
