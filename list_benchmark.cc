#include "third_party/benchmark/include/benchmark/benchmark.h"

#include <utility>
#include <cassert>

using namespace std;

template <typename T>
struct list_node {
  T val_;
  list_node* next;
};

namespace {
template <typename T>
list_node<T>* prepend(list_node<T>* root, T new_v) {
  return new list_node<T>{new_v, root};
}

template <typename T>
void delete_list(list_node<T>* root) {
  while (root) {
    list_node<T>* prev = root;
    root = root->next;
    delete prev;
  }
}

}  // namespace

list_node<size_t>* generate_sequence_simple(size_t n) {
  list_node<size_t>* res = nullptr;
  while (n) res = prepend(res, n--);
  return res;
}


list_node<size_t>* generate_sequence_unrolled(size_t n) {
  size_t half = n >> 1;

  list_node<size_t> *first_half = nullptr;
  list_node<size_t> *second_half = (n & 1) ? new list_node<size_t>{n--, nullptr} : nullptr;
 //  assert((n & 1) == 0);

  if (!half)
    return second_half;

  auto append_nodes = [&] {
    first_half = prepend(first_half, half--);
    second_half = prepend(second_half, n--);
    return first_half;
  };

  auto* last_in_the_first_half = append_nodes();
  while (half) append_nodes();
  last_in_the_first_half->next = second_half;

  return first_half;
}


template <typename Gen>
void generate_list_benchmark(benchmark::State& state, Gen gen) {
  while (state.KeepRunning()) {
    auto* res = gen(static_cast<size_t>(state.range(0)));
    state.PauseTiming();
    delete_list(res);
    state.ResumeTiming();
  }
}

void generate_list_simple_benchmark(benchmark::State& state) {
  generate_list_benchmark(state, generate_sequence_simple);
}

void generate_list_unrolled_benchmark(benchmark::State& state) {
  generate_list_benchmark(state, generate_sequence_unrolled);
}

BENCHMARK(generate_list_unrolled_benchmark) -> Arg(1000);
BENCHMARK(generate_list_simple_benchmark) -> Arg(1000);

BENCHMARK_MAIN();

