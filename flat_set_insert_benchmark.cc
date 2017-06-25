#include "benchmarks/insert_algorithms.h"

#include <algorithm>
#include <random>
#include <set>
#include <map>
#include <string>
#include <vector>

#include "third_party/benchmark/include/benchmark/benchmark.h"

namespace {

constexpr int c_set_size = 1000;
constexpr int c_distribution_size = 1000000;

constexpr int c_min_input_size = 1;
constexpr int c_max_input_size = 1000;
constexpr int c_input_step = 1;

using int_vec = std::vector<int>;

std::pair<int_vec*, int_vec*> test_input_data(int inserting_size) {
  auto random_number = [] {
    static std::mt19937 g;
    static std::uniform_int_distribution<> dis(1, c_distribution_size);
    return dis(g);
  };

  static int_vec already_in = [&] {
    assert(c_distribution_size > c_set_size);
    std::set<int> res;
    while (res.size() < c_set_size)
      res.insert(random_number());
    return int_vec(res.begin(), res.end());
  }();

  static std::map<int, std::vector<int>> inserting_cache;

  auto found = inserting_cache.find(inserting_size);
  if (found == inserting_cache.end()) {
    found = inserting_cache.insert({inserting_size,
                                    [&] {
                                      int_vec res(
                                          static_cast<size_t>(inserting_size));
                                      std::generate(res.begin(), res.end(),
                                                    random_number);
                                      return res;
                                    }()}).first;
  }

  return {&already_in, &found->second};
}

template <typename F>
// requires PureFunction<F>
void benchmark_unique_insert(benchmark::State& state, F insertion_algorithm) {
  auto input = test_input_data(state.range(0));
  while (state.KeepRunning()) {
    auto c = *input.first;
    auto f = input.second->begin();
    auto l = input.second->end();
    insertion_algorithm(c, f, l);
  }
}

void baseline(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto&&...) {});

  // hacking into benchmark's json output.
  // clang-format off
  std::string extra_data =
      /*"label" : "*/ "\",\n"                                                              //
"      \"extra_data\":  {\n"                                                                                                                        //
"        \"set_size\": " + std::to_string(c_set_size) + ",\n"
"        \"distribution_size\": " + std::to_string(c_distribution_size) + "\n"
"        },\n"
"      \"junk_field\": \""/*"*/;
  // clang-format on
  state.SetLabel(extra_data);
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

void benchmark_full_inplace_merge(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::full_inplace_merge(c, f, l, std::less<>{});
  });
}

void benchmark_copy_unique_full_inplace_merge(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::copy_unique_full_inplace_merge(c, f, l, std::less<>{});
  });
}

void benchmark_copy_unique_inplace_merge_cache_begin(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::copy_unique_inplace_merge_cache_begin(c, f, l, std::less<>{});
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

void benchmark_use_end_buffer_2_times_new(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::use_end_buffer_2_times_new(c, f, l, std::less<>{});
  });
}

void benchmark_reallocate_and_merge(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::reallocate_and_merge(c, f, l, std::less<>{});
  });
}

void benchmark_use_end_buffer_new_size(benchmark::State& state) {
  benchmark_unique_insert(state, [](auto& c, auto f, auto l) {
    bulk_insert::use_end_buffer_new_size(c, f, l, std::less<>{});
  });
}

void boost_and_eastl_solution(benchmark::State& state) {
  benchmark_one_at_a_time(state);
}

void folly_solution(benchmark::State& state) {
  benchmark_full_inplace_merge(state);
}

void chromium_solution(benchmark::State& state) {
  benchmark_copy_unique_inplace_merge_cache_begin(state);
}

void proposed_solution(benchmark::State& state) {
  benchmark_use_end_buffer_2_times_new(state);
}

void set_input_sizes(benchmark::internal::Benchmark* bench) {
  for (int i = c_min_input_size; i < c_max_input_size; i += c_input_step)
    bench->Arg(i);
}

BENCHMARK(baseline)->Apply(set_input_sizes);
BENCHMARK(benchmark_one_at_a_time)->Apply(set_input_sizes);
BENCHMARK(benchmark_stable_sort_and_unique)->Apply(set_input_sizes);
BENCHMARK(benchmark_full_inplace_merge)->Apply(set_input_sizes);
BENCHMARK(benchmark_copy_unique_full_inplace_merge)->Apply(set_input_sizes);
BENCHMARK(benchmark_copy_unique_inplace_merge_cache_begin)
    ->Apply(set_input_sizes);
BENCHMARK(benchmark_copy_unique_inplace_merge_upper_bound)
    ->Apply(set_input_sizes);
BENCHMARK(benchmark_copy_unique_inplace_merge_no_buffer)
    ->Apply(set_input_sizes);
BENCHMARK(benchmark_use_end_buffer_2_times_new)->Apply(set_input_sizes);
BENCHMARK(benchmark_reallocate_and_merge)->Apply(set_input_sizes);
BENCHMARK(benchmark_use_end_buffer_new_size)->Apply(set_input_sizes);

BENCHMARK(boost_and_eastl_solution)->Apply(set_input_sizes);
BENCHMARK(folly_solution)->Apply(set_input_sizes);
BENCHMARK(chromium_solution)->Apply(set_input_sizes);
BENCHMARK(proposed_solution)->Apply(set_input_sizes);

}  // namespace

BENCHMARK_MAIN();
