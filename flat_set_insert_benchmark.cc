#include <algorithm>
#include <map>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

#include "third_party/benchmark/include/benchmark/benchmark.h"

using namespace std;

namespace {

template <typename I>
using IteratorCategory = typename std::iterator_traits<I>::iterator_category;

template <typename T>
auto insert_one(vector<T>& body, T v) {
  auto where = lower_bound(begin(body), end(body), v);
  if (where != end(body) || !(v < *where))
    return where;
  return body.insert(where, move(v));
}

template <typename T, typename I>
void insert_by_one(vector<T>& body, I f, I l) {
  // some implementations use hint here, this is another subject.
  while (f != l)
    insert_one(body, *f++);
}

template <typename T, typename I>
void sort_and_unique_everything(vector<T>& body, I f, I l) {
  copy(f, l, back_inserter(body));
  stable_sort(body.begin(), body.end());
  body.erase(unique(body.begin(), body.end()), body.end());
}

template <typename T, typename I>
void sort_inplace_merge_unique(vector<T>& body, I f, I l) {
  auto original_size = body.size();
  copy(f, l, std::back_inserter(body));
  auto middle = body.begin() + original_size;
  sort(middle, body.end());
  inplace_merge(body.begin(), middle, body.end());
  body.erase(unique(body.begin(), body.end()), body.end());
}

template <typename T, typename I>
void smart_insert(vector<T>& body, I f, I l) {
  if (is_same<IteratorCategory<I>, input_iterator_tag>::value)
    insert_by_one(body, f, l);

  auto original_size = body.size();
  auto cached_min_position = body.size();

  copy_if(f, l, back_inserter(body), [&](const auto& v) {
    auto where = lower_bound(begin(body), end(body), v);
    if (where != begin(body) + original_size && !(v < *where))
      return false;

    cached_min_position = std::min(cached_min_position, where - begin(body));
    return true;
  });

  sort(begin(body) + original_size, end(body));
  body.erase(unique(begin(body) + original_size, body.end()), body.end());
  inplace_merge(begin(body) + cached_min_position,
                begin(body) + cached_min_position, end(body));
}

using TestCase = pair<vector<int>,   // original elements
                      vector<int>>;  // new elements

const TestCase& generate_test_case(size_t original_size, size_t new_elements) {
  // ensuring equal inputs.
  static map<pair<size_t, size_t>, TestCase> previous_results;

  auto cached_res = previous_results.find({original_size, new_elements});
  if (cached_res != previous_results.end())
    return cached_res->second;

  vector<int> all_elements(original_size + new_elements);
  iota(begin(all_elements), end(all_elements), 0);

  static mt19937 g;
  shuffle(begin(all_elements), end(all_elements), g);

  TestCase new_case;
  new_case.first = {begin(all_elements), begin(all_elements) + original_size};
  std::sort(begin(new_case.first), end(new_case.first));
  new_case.second = {begin(all_elements) + original_size, end(all_elements)};

  return previous_results.emplace(make_pair(original_size, new_elements),
                                  move(new_case)).first->second;
}

template <typename AlgotihmCaller>
void benchmark_insetion_algorithm(benchmark::State& state,
                                  AlgotihmCaller caller) {
  const TestCase& test = generate_test_case(
      static_cast<size_t>(state.range(0)), static_cast<size_t>(state.range(1)));
  while (state.KeepRunning()) {
    state.PauseTiming();
    vector<int> already_in = test.first;
    state.ResumeTiming();
    caller(already_in, begin(test.second), end(test.second));
  }
}

void insert_by_one_benchmark(benchmark::State& state) {
  benchmark_insetion_algorithm(
      state, [](auto body, auto f, auto l) { insert_by_one(body, f, l); });
}

void sort_and_unique_everything_benchmark(benchmark::State& state) {
  benchmark_insetion_algorithm(state, [](auto body, auto f, auto l) {
    sort_and_unique_everything(body, f, l);
  });
}

void input_arguments(benchmark::internal::Benchmark* b) {
  vector<int> original_sizes = {/*10, 50, 100, 500,*/ 1000};
  for (int original_size : original_sizes) {
    // b->Args({original_size, 2});
    b->Args({original_size, 10});
    // b->Args({original_size, 30});
    // b->Args({original_size, original_size / 2});
    // b->Args({original_size, original_size});
  }
}

BENCHMARK(insert_by_one_benchmark)->Apply(input_arguments);
BENCHMARK(sort_and_unique_everything_benchmark)->Apply(input_arguments);



}  // namespace


BENCHMARK_MAIN();
