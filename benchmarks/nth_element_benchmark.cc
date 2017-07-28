#include <algorithm>
#include <array>
#include <vector>
#include <random>
#include <iostream>

#include "third_party/benchmark/include/benchmark/benchmark.h"

namespace {

using value_type = std::array<int64_t, 3>;
constexpr size_t kArraySize = 2000;
constexpr size_t kNthElement = 200;

static const std::vector<value_type>& inputs() {
  static const auto cached_inputs = [] {
    std::vector<value_type> res(kArraySize);

    for (auto& x : res) {
      std::generate(x.begin(), x.end(), [] {
        static std::mt19937 g;
        static std::uniform_int_distribution<> dis(1, 10000);
        return dis(g);
      });
    }
    return res;
  }();

  return cached_inputs;
}

template <typename F>
void benchmark_nth_element(benchmark::State& state, F f) {
  while (state.KeepRunning()) {
    auto input_copy = inputs();
    f(input_copy.begin(), input_copy.begin() + kNthElement, input_copy.end());
  }
}

void benchmark_empty(benchmark::State& state) {
  benchmark_nth_element(state, [](auto f, auto m, auto l) {});
}

void benchmark_only_first(benchmark::State& state) {
  benchmark_nth_element(state, [](auto f, auto m, auto l) {
    std::nth_element(f, m, l, [](const auto& lhs, const auto& rhs) {
      return lhs[0] < rhs[0];
    });
  });
}

void benchmark_compare_all_default(benchmark::State& state) {
  benchmark_nth_element(state, [](auto f, auto m, auto l) {
    std::nth_element(f, m, l);
  });
}

void benchmark_compare_all_custom(benchmark::State& state) {
  benchmark_nth_element(state, [](auto f, auto m, auto l) {
    std::nth_element(f, m, l, [](const auto& lhs, const auto& rhs) {
      if (lhs[0] != rhs[0]) {
        return lhs < rhs;
      }

      if (lhs[1] < rhs[1])
        return true;
      if (lhs[1] != rhs[1])
        return false;
      return lhs[2] < rhs[2];
    });
  });
}

BENCHMARK(benchmark_empty);
BENCHMARK(benchmark_only_first);
BENCHMARK(benchmark_compare_all_default);
BENCHMARK(benchmark_compare_all_custom);
}

BENCHMARK_MAIN();
