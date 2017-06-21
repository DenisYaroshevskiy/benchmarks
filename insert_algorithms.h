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

template <typename P, typename T>
// requires StrictWeakOrdering<P, T>
auto less_than(P p, const T& t) {
  return [&](const auto& x) {
    return p(x, t);
  };
}

template <typename I, typename P>
// requires BidirectionalIterator<I> &&         //
//          StrictWeakOrdering<P, ValueType<I>> //
void inplace_merge_no_buffer(I f, I m, I l, P p) {
  if (f == m || m == l)
    return;

  auto m1 = std::upper_bound(f, m, *m, p);
  auto m2 = std::lower_bound(m , l, *std::prev(m), p);
  m = std::rotate(m1, m, m2);
  inplace_merge_no_buffer(f, m1, m, p);
  inplace_merge_no_buffer(m, m2, l, p);
}


template <typename I, typename P>
// requires ForwardIterator<I> && UnaryPredicate<P, ValueType<I>>
I partition_point_biased(I f, I l, P p) {
  if (f == l || !p(*f))
    return f;
  ++f;
  auto len = std::distance(f, l);
  auto step = 1;
  while (len > step) {
    I m = std::next(f, step);
    if (!p(*m)) {
      l = m;
      break;
    }
    f = ++m;
    len -= step + 1;
    step <<= 1;
  }
  return std::partition_point(f, l, p);
}

template <typename I, typename V, typename P>
// requires ForwardIterator<I> && StrictWeakOrdering<P, ValueType<I>>
I lower_bound_biased(I f, I l, const V& v, P p) {
  return partition_point_biased(f, l, less_than(p, v));
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

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          InputIterator<I> &&                         //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void copy_unique_inplace_merge_no_buffer(C& c, I f, I l, P p) {
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
  helpers::inplace_merge_no_buffer(c.begin(), m(), c.end(), p);
}

}  // bulk_insert
