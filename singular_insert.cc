#include <algorithm>
#include <numeric>
#include <iostream>

#include "third_party/benchmark/include/benchmark/benchmark.h"

// Type functions ---------------------------------------------------

template <typename I>
//  requires Iterator<I>
using ValueType = typename std::iterator_traits<I>::value_type;

// Binary search variations -----------------------------------------

// This algorithms was in the original stl but was removed due to the fact that
// the committee felt there were too many already in.
// Too bad, since std::partition_point has to compute the distance anyways.
template <typename I, typename N, typename P>
// requires ForwardIterator<I> && Integer<N> && UnaryPredicate<P>
std::pair<I, N> partition_point_n(I f, N n, P p) {
  // TODO:
  I res = std::partition_point(f, std::next(f, n), p);
  return {res, std::distance(f, res)};
}

template <typename I, typename N, typename P>
// requires ForwardIterator<I> && Integer<N> && UnaryPredicate<P>
std::pair<I, N> partition_point_biased_n(I f, N n, P p) {
  // assert (std::is_partitioned(f, std::next(f, n), P)
  // assert(n >= DifferenceType<I>(0))
  N step(1);
  while (n > step) {
    I test = std::next(f, step);
    if (!p(*test)) {
      // We narrowed down our search area between to points.
      // After that we no longer have preferences.
      n = step;
      break;
    }
    f = ++test;  // We already checked test.
    n -= step + N(1);
    step <<= 1;
  }
  return partition_point_n(f, n, p);
}

template <typename I, typename P>
// requires ForwardIterator<I> && UnaryPredicate<P>
I partition_point_biased(I f, I l, P p) {
  return partition_point_biased_n(f, std::distance(f, l), p).first;
}

template <typename I, typename P>
// requires BidirectionalIterator<I> && UnaryPredicate<P>
I partition_point_hinted(I f, I h, I l, P p) {
  if (h != l && p(*h))
    return partition_point_biased(++h, l, p);
  return partition_point_biased(
             std::make_reverse_iterator(h), std::make_reverse_iterator(f),
             [&](const ValueType<I>& x) { return !p(x); }).base();
}

template <typename I, typename V, typename P>
// requires BidirectionalIterator<I> && BinaryPredicate<P>
I lower_bound_hinted(I f, I h, I l, const V& v, P p) {
  // assert(std::is_partitioned(f, l, p(..., v));
  return partition_point_hinted(f, h, l,
                                [&](const ValueType<I>& x) { return p(x, v); });
}

template <typename I, typename V>
// requires BidirectionalIterator<I> && TotallyOrdered<ValueType<I>> &&
// Comparable<V, ValueType<I>>
I lower_bound_hinted(I f, I h, I l, const V& v) {
  return lower_bound_hinted(f, h, l, v, std::less<>{});
}

template <typename Searcher>
void searcher_benchmark(benchmark::State& state, Searcher searcher) {
  std::vector<int> input(state.range(0));
  std::iota(input.begin(), input.end(), 0);
  int looking_for = state.range(1);
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(searcher(input.begin(), input.end(), looking_for));
  }
}

void lower_bound_hinted_bechmark(benchmark::State& state) {
  searcher_benchmark(state,
                     [hint = state.range(2)](auto f, auto l, auto looking_for) {
    return lower_bound_hinted(f, f + hint, l, looking_for);
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
      for (int distance : {0, 10, size / 10, size / 5, size / 2})
        add_case(size, looking_for, distance);
}

BENCHMARK(lower_bound_hinted_bechmark)->Apply(test_cases);
BENCHMARK(lower_bound_standard)->Apply(test_cases);

BENCHMARK_MAIN();
