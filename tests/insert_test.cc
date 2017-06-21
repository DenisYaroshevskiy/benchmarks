#include "benchmarks/insert_algorithms.h"

#include <algorithm>
#include <functional>

#define CATCH_CONFIG_MAIN
#include "third_party/catch/catch.h"

template <typename F>
// requires PureFunction<F>
void test_unique_insert(F insertion_algorithm) {
  using C = std::vector<int>;
  C c;

  auto insert = [&](const C& values) {
    insertion_algorithm(c, std::begin(values), std::end(values));
  };

  insert({});
  CHECK(c.empty());
  insert({1, 2, 3});
  CHECK(c == C({1, 2, 3}));
  insert({});
  CHECK(c == C({1, 2, 3}));
  insert({1, 2});
  CHECK(c == C({1, 2, 3}));
  insert({6, 7});
  CHECK(c == C({1, 2, 3, 6, 7}));
  insert({4, 6});
  CHECK(c == C({1, 2, 3, 4, 6, 7}));
  insert({5, 1, 2});
  CHECK(c == C({1, 2, 3, 4, 5, 6, 7}));
}

TEST_CASE("one_at_a_time", "[multiple_insertions]") {
  test_unique_insert([](auto& c, auto f, auto l) {
    bulk_insert::one_at_a_time(c, f, l, std::less<>{});
  });
}

TEST_CASE("stable_sort_and_unique", "[multiple_insertions]") {
  test_unique_insert([](auto& c, auto f, auto l) {
    bulk_insert::stable_sort_and_unique(c, f, l, std::less<>{});
  });
}

TEST_CASE("naive_inplace_merge", "[multiple_insertions]") {
  test_unique_insert([](auto& c, auto f, auto l) {
    bulk_insert::naive_inplace_merge(c, f, l, std::less<>{});
  });
}

TEST_CASE("copy_unique_full_inplace_merge", "[multiple_insertions]") {
  test_unique_insert([](auto& c, auto f, auto l) {
    bulk_insert::copy_unique_full_inplace_merge(c, f, l, std::less<>{});
  });
}

TEST_CASE("copy_unique_inplace_merge_begin", "[multiple_insertions]") {
  test_unique_insert([](auto& c, auto f, auto l) {
    bulk_insert::copy_unique_inplace_merge_begin(c, f, l, std::less<>{});
  });
}

TEST_CASE("copy_unique_inplace_merge_upper_bound", "[multiple_insertions]") {
  test_unique_insert([](auto& c, auto f, auto l) {
    bulk_insert::copy_unique_inplace_merge_upper_bound(c, f, l, std::less<>{});
  });
}

TEST_CASE("copy_unique_inplace_merge_no_buffer", "[multiple_insertions]") {
  test_unique_insert([](auto& c, auto f, auto l) {
    bulk_insert::copy_unique_inplace_merge_no_buffer(c, f, l, std::less<>{});
  });
}

TEST_CASE("lower_bound_biased", "[binary_search]") {
  auto test = [](const std::vector<int>& c, const auto& v) {
    auto biased =
        helpers::lower_bound_biased(c.begin(), c.end(), v, std::less<>{});
    auto std = std::lower_bound(c.begin(), c.end(), v);
    CHECK(biased == std);
  };

  test({}, 1); // empty range.

  // odd number of elements.
  for (int i = 0; i <= 10; ++i)
    test({1, 2, 3, 4, 5, 6, 7, 8, 9}, i);

  // even number of elements.
  for (int i = 0; i <= 11; ++i)
    test({1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, i);
}
