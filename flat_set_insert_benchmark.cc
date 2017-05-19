#include <algorithm>
#include <utility>
#include <type_traits>
#include <vector>

#include "third_party/benchmark/include/benchmark/benchmark.h"

using namespace std;

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
  if (is_same<IteratorCategory<I>, input_iterator_tag>::value) {
    insert_by_one(body, f, l);
  }

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

BENCHMARK_MAIN();
