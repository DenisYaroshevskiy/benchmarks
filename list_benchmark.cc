#include "third_party/benchmark/include/benchmark/benchmark.h"

#include <memory>
#include <random>
#include <list>
#include <vector>

using namespace std;

template <typename T>
struct list_node {
  T val_;
  list_node* next;
};

class awful_allocator {
 public:
  virtual list_node<size_t>* make(list_node<size_t> new_node) = 0;
  virtual void free() = 0;

 protected:
  awful_allocator() = default;
  ~awful_allocator() = default;
  awful_allocator(const awful_allocator&) = delete;
  awful_allocator& operator=(const awful_allocator&) = delete;
};

class mallocator : public awful_allocator {
 public:
  explicit mallocator(size_t n) : pool_(n) {}

  list_node<size_t>* make(list_node<size_t> new_node) final {
    pool_[index_] = make_unique<list_node<size_t>>(new_node);
    return pool_[index_++].get();
  }

  void free() final {
    for (auto& elem : pool_)
      elem.reset();
    index_ = 0u;
  }

 private:
  vector<unique_ptr<list_node<size_t>>> pool_;
  // we do this instead of reserve, because reserve does capacity check.
  // and we no the size in advance.
  size_t index_ = 0u;
};

class local_pool_allocator : public awful_allocator {
 public:
  explicit local_pool_allocator(size_t n) : pool_(n) {}

  list_node<size_t>* make(list_node<size_t> new_node) final {
    return &pool_[index_];
  }

  void free() final { index_ = 0u; }

 private:
  vector<list_node<size_t>> pool_;
  size_t index_ = 0u;
};

template <typename T>
list<T> list_of_shuffeled_memory(size_t n) {
  if (!n)
    return {};
  list<T> middle;
  if (n & 1)
    middle.emplace_back();

  const size_t half = n >> 1;
  auto rhs = list_of_shuffeled_memory<T>(half);
  auto lhs = list_of_shuffeled_memory<T>(half);
  lhs.splice(lhs.end(), std::move(middle));
  lhs.splice(lhs.end(), std::move(rhs));
  return lhs;
}

class non_local_pool_allocator : public awful_allocator {
 public:
  explicit non_local_pool_allocator(size_t n)
      : pool_{list_of_shuffeled_memory<list_node<size_t>>(n)},
        pos_{pool_.begin()} {}

  list_node<size_t>* make(list_node<size_t> new_node) final {
    return &(*pos_++);
  }

  void free() final { pos_ = pool_.begin(); }

 private:
  std::list<list_node<size_t>> pool_;
  std::list<list_node<size_t>>::iterator pos_;
};

list_node<size_t>* prepend(list_node<size_t>* root,
                           size_t new_v,
                           awful_allocator& alloc) {
  return alloc.make({new_v, root});
}

__attribute__((noinline))
list_node<size_t>* generate_sequence_simple(size_t n, awful_allocator& alloc) {
  list_node<size_t>* res = nullptr;
  while (n)
    res = prepend(res, n--, alloc);
  return res;
}

__attribute__((noinline)) list_node<size_t>* generate_sequence_unrolled(
    size_t n,
    awful_allocator& alloc) {
  size_t half = n >> 1;

  list_node<size_t>* first_half = nullptr;
  list_node<size_t>* second_half =
      (n & 1) ? new list_node<size_t>{n--, nullptr} : nullptr;
  //  assert((n & 1) == 0);

  if (!half)
    return second_half;

  auto append_nodes = [&] {
    first_half = prepend(first_half, half--, alloc);
    second_half = prepend(second_half, n--, alloc);
    return first_half;
  };

  auto* last_in_the_first_half = append_nodes();
  while (half)
    append_nodes();
  last_in_the_first_half->next = second_half;

  return first_half;
}

template <typename Allocator, typename Gen>
void generate_list_benchmark(benchmark::State& state, Gen gen) {
  size_t n = static_cast<size_t>(state.range(0));
  Allocator alloc(n);
  while (state.KeepRunning()) {
    gen(n, alloc);
    state.PauseTiming();
    alloc.free();
    state.ResumeTiming();
  }
}

template <typename Allocator>
void generate_list_simple_benchmark(benchmark::State& state) {
  generate_list_benchmark<Allocator>(state, generate_sequence_simple);
}

template <typename Allocator>
void generate_list_unrolled_benchmark(benchmark::State& state) {
  generate_list_benchmark<Allocator>(state, generate_sequence_unrolled);
}

BENCHMARK_TEMPLATE(generate_list_simple_benchmark, mallocator)->Arg(1000);
BENCHMARK_TEMPLATE(generate_list_unrolled_benchmark, mallocator)->Arg(1000);

BENCHMARK_TEMPLATE(generate_list_simple_benchmark, local_pool_allocator)
    ->Arg(1000);
BENCHMARK_TEMPLATE(generate_list_unrolled_benchmark, local_pool_allocator)
    ->Arg(1000);

BENCHMARK_TEMPLATE(generate_list_simple_benchmark, non_local_pool_allocator)
    ->Arg(1000);
BENCHMARK_TEMPLATE(generate_list_unrolled_benchmark, non_local_pool_allocator)
    ->Arg(1000);

BENCHMARK_MAIN();
