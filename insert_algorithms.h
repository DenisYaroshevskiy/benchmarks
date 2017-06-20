#pragma once

#include <algorithm>
#include <iterator>

namespace helpers {

template <typename P>
// requires Predicate<P>
auto not_fn(P p) {
  return
      [p](auto&&... args) { return !p(std::forward<decltype(args)>(args)...); };
}

}  // helpers

namespace bulk_insert {

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          InputIterator<I> &&                         //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void one_at_a_time(C& c, I f, I l, P p) {
  for (; f != l; ++f) {
    auto where = std::lower_bound(c.begin(), c.end(), *f, p);
    if (where != c.end() && !p(*f, *where))
      continue;
    c.insert(where, *f);
  }
}

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          InputIterator<I> &&                         //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void stable_sort_and_unique(C& c, I f, I l, P p) {
  c.insert(c.end(), f, l);
  std::stable_sort(c.begin(), c.end(), p);
  c.erase(std::unique(c.begin(), c.end(), helpers::not_fn(p)), c.end());
}

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          InputIterator<I> &&                         //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void naive_inplace_merge(C& c, I f, I l, P p) {
  auto original_size = c.size();
  c.insert(c.end(), f, l);
  auto m = c.begin() + original_size;
  std::sort(m, c.end(), p);
  std::inplace_merge(c.begin(), m, c.end(), p);
  c.erase(std::unique(c.begin(), c.end(), helpers::not_fn(p)), c.end());
}

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          InputIterator<I> &&                         //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void copy_unique_full_inplace_merge(C& c, I f, I l, P p) {
  auto m = [&c, original_size = c.size() ] {
    return c.begin() + original_size;
  };

  std::copy_if(f, l, std::inserter(c, c.end()), [&](const auto& x) {
    auto found = std::lower_bound(c.begin(), m(), x, p);
    return found == m() || p(x, *found);
  });

  std::sort(m(), c.end(), p);
  c.erase(std::unique(m(), c.end(), helpers::not_fn(p)), c.end());
  std::inplace_merge(c.begin(), m(), c.end(), p);
}

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          InputIterator<I> &&                         //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void copy_unique_inplace_merge_begin(C& c, I f, I l, P p) {
  if (f == l)
    return;

  auto m = [&c, original_size = c.size() ] {
    return c.begin() + original_size;
  };
  auto cached_merge_begin = std::distance(c.begin(), m());

  std::copy_if(f, l, std::inserter(c, c.end()), [&](const auto& x) {
    auto found = std::lower_bound(c.begin(), m(), x, p);
    if (found != m() && !p(x, *found))
      return false;

    cached_merge_begin =
        std::min(cached_merge_begin, std::distance(c.begin(), found));
    return true;
  });

  std::sort(m(), c.end(), p);
  c.erase(std::unique(m(), c.end(), helpers::not_fn(p)), c.end());
  std::inplace_merge(c.begin() + cached_merge_begin, m(), c.end(), p);
}

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          InputIterator<I> &&                         //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void copy_unique_inplace_merge_upper_bound(C& c, I f, I l, P p) {
  auto m = [&c, original_size = c.size() ] {
    return c.begin() + original_size;
  };

  std::copy_if(f, l, std::inserter(c, c.end()), [&](const auto& x) {
    auto found = std::lower_bound(c.begin(), m(), x, p);
    return found == m() || p(x, *found);
  });

  if (m() == c.end())
    return;

  std::sort(m(), c.end(), p);
  c.erase(std::unique(m(), c.end(), helpers::not_fn(p)), c.end());
  std::inplace_merge(std::upper_bound(c.begin(), m(), *m(), p),  //
                     m(), c.end(), p);
}

}  // bulk_insert
