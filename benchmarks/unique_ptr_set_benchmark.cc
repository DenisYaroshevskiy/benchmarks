#include <algorithm>
#include <array>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <random>

#include "base/containers/flat_set.h"
#include "base/containers/flat_map.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"

namespace {

constexpr size_t kElementsSize = 100;

using ptr_array = std::array<int*, kElementsSize>;

const ptr_array& input() {
  static const ptr_array cached_res = [] {
    ptr_array ptrs;
    std::mt19937 g;
    std::uniform_int_distribution<> dis;
    std::generate(ptrs.begin(), ptrs.end(),
                  [&] { return reinterpret_cast<int*>(dis(g)); });
    return ptrs;
  }();
  return cached_res;
}

struct do_nothing {
  template <typename T>
  void operator()(T*) {
    return;
  }
};

using unique_t = std::unique_ptr<int, do_nothing>;
using fl_set = base::flat_set<unique_t>;
using fl_map = base::flat_map<int*, unique_t>;
using std_set = std::set<unique_t>;
using std_map = std::map<int*, unique_t>;

template <typename C>
class insert_unique_ptr_t {
public:
  explicit insert_unique_ptr_t(C& c) : c_(&c) {}

  void operator()(int* v) { run(*c_, v); }

 private:
  static void run(fl_set& c, int* v) { c.emplace(unique_t(v)); }

  static void run(fl_map& c, int* v) { c.emplace(v, unique_t(v)); }

  static void run(std_set& c, int* v) { c.emplace(unique_t(v)); }

  static void run(std_map& c, int* v) { c.emplace(v, unique_t(v)); }

  C* c_;
};

template <typename C>
auto insert_unique_ptr(C& c) {
  return insert_unique_ptr_t<C>{c};
}

template <typename C>
void benchmark_insert_unique_ptrs(benchmark::State& state) {
  (void)input();
  while (state.KeepRunning()) {
    C c;
    auto inserter = insert_unique_ptr(c);
    for (const auto& ptr : input())
      inserter(ptr);
  }
}

void benchmark_flat_set(benchmark::State& state) {
  benchmark_insert_unique_ptrs<fl_set>(state);
}

void benchmark_flat_map(benchmark::State& state) {
  benchmark_insert_unique_ptrs<fl_map>(state);
}

void benchmark_std_set(benchmark::State& state) {
  benchmark_insert_unique_ptrs<std_set>(state);
}

void benchmark_std_map(benchmark::State& state) {
  benchmark_insert_unique_ptrs<std_map>(state);
}

BENCHMARK(benchmark_flat_set);
BENCHMARK(benchmark_flat_map);
BENCHMARK(benchmark_std_set);
BENCHMARK(benchmark_std_map);
}

BENCHMARK_MAIN();
