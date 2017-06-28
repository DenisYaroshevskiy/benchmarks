#pragma once

#include <algorithm>
#include <iterator>
#include <vector>

#include "benchmarks/copy.h"

namespace helpers {

template <typename P>
// requires Predicate<P>
auto not_fn(P p) {
  return
      [p](auto&&... args) { return !p(std::forward<decltype(args)>(args)...); };
}

template <typename P>
// requires StrictWeakOrdering<P>
auto strict_oposite(P p) {
  return [p](const auto& x, const auto& y) { return p(y, x); };
}

template <typename P, typename T>
// requires StrictWeakOrdering<P, T>
auto less_than(P p, const T& t) {
  return [&](const auto& x) { return p(x, t); };
}

template <typename I, typename P>
// requires BidirectionalIterator<I> &&         //
//          StrictWeakOrdering<P, ValueType<I>> //
void inplace_merge_no_buffer(I f, I m, I l, P p) {
  if (f == m || m == l)
    return;

  auto m1 = std::upper_bound(f, m, *m, p);
  auto m2 = std::lower_bound(m, l, *std::prev(m), p);
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

struct copy_traits
{
  template <typename I, typename O>
  static O copy (I f, I l, O o) {
    return helpers::copy(f, l, o);
  }
};

struct stric_copy_traits
{
  template <typename I, typename O>
  static O copy (I f, I l, O o) {
    return helpers::strict_copy(f, l, o);
  }
};

template <typename Traits, typename I1, typename I2, typename O, typename P>
// requires ForwardIterator<I1> &&
//          ForwardIterator<I2> &&
//          OutputIterator<O>
//          StrictWeakOrdering<P, ValueType<I>>
std::tuple<I1, I2, O> set_union_adaptive_intersecting_parts(I1 f1,
                                                            I1 l1,
                                                            I2 f2,
                                                            I2 l2,
                                                            O o,
                                                            P p) {
  auto advance_range = [&](auto& f, auto l, const auto& v) {
    auto m = lower_bound_biased(f, l, v, p);
    o = Traits::copy(f, m, o);
    f = m;
  };

  while (true) {
    if (f2 == l2)
      break;

    advance_range(f1, l1, *f2);

    if (f1 == l1)
      break;

    advance_range(f2, l2, *f1);

    if (f2 == l2)
      break;

    if (!p(*f1, *f2))
      ++f2;
  }

  return {f1, f2, o};
}

template <typename I1, typename I2, typename P>
// requires ForwardIterator<I1> &&
//          ForwardIterator<I2> &&
//          StrictWeakOrdering<P, ValueType<I>>
std::pair<I1, I1> set_union_adaptive_into_tail(I1 buf,
                                               I1 f1,
                                               I1 l1,
                                               I2 f2,
                                               I2 l2,
                                               P p) {
  std::move_iterator<I1> move_f1;
  std::tie(move_f1, f2, buf) =
      set_union_adaptive_intersecting_parts<copy_traits>(
          std::make_move_iterator(f1),  //
          std::make_move_iterator(l1),  //
          f2, l2,                       //
          buf, p);                      //

  return {helpers::copy(f2, l2, buf), move_f1.base()};
}

template <typename I1, typename I2, typename O, typename P>
// requires ForwardIterator<I1> &&
//          ForwardIterator<I2> &&
//          OutputIterator<O>
//          StrictWeakOrdering<P, ValueType<I>>
O set_union_adaptive(I1 f1, I1 l1, I2 f2, I2 l2, O o, P p) {
  std::tie(f1, f2, o) =
      set_union_adaptive_intersecting_parts<stric_copy_traits>(f1, l1, f2, l2,
                                                               o, p);
  o = helpers::strict_copy(f1, l1, o);
  return helpers::strict_copy(f2, l2, o);
}

template <typename C, typename BufSize, typename I, typename P>
// requires Container<C> &&                             //
//          ForwardIterator<I> &&                       //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void use_end_buffer_impl(C& c, BufSize buf_size, I f, I l, P p) {
  auto new_len = std::distance(f, l);
  auto orig_len = c.size();
  c.resize(orig_len + buf_size + new_len);

  auto orig_f = c.begin();
  auto orig_l = c.begin() + orig_len;
  auto f_in = c.end() - new_len;
  auto l_in = c.end();
  auto buf = f_in;

  helpers::strict_copy(f, l, f_in);
  std::sort(f_in, l_in, p);
  l_in = std::unique(f_in, l_in, helpers::not_fn(p));

  using reverse_it = typename C::reverse_iterator;
  auto move_reverse_it =
      [](auto it) { return std::make_move_iterator(reverse_it(it)); };

  auto reverse_remainig_buf_range = helpers::set_union_adaptive_into_tail(
      reverse_it(buf),                               // buffer
      reverse_it(orig_l), reverse_it(orig_f),        // original
      move_reverse_it(l_in), move_reverse_it(f_in),  // new elements
      helpers::strict_oposite(p));                   // greater

  auto remaining_buf =
      std::make_pair(reverse_remainig_buf_range.second.base() - c.begin(),
                     reverse_remainig_buf_range.first.base() - c.begin());

  c.erase(c.end() - new_len, c.end());
  c.erase(c.begin() + remaining_buf.first, c.begin() + remaining_buf.second);
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
void full_inplace_merge(C& c, I f, I l, P p) {
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
void copy_unique_inplace_merge_cache_begin(C& c, I f, I l, P p) {
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

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          ForwardIterator<I> &&                       //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void use_end_buffer_new(C& c, I f, I l, P p) {
  helpers::use_end_buffer_impl(c, std::distance(f, l), f, l, p);
}

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          ForwardIterator<I> &&                       //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void reallocate_and_merge(C& c, I f, I l, P p) {
  C new_c;
  auto new_len = std::distance(f, l);
  new_c.resize(c.size() + new_len);
  auto orig_l =
      helpers::strict_copy(std::make_move_iterator(c.begin()),
                           std::make_move_iterator(c.end()), new_c.begin());
  c.resize(std::distance(f, l));
  helpers::strict_copy(f, l, c.begin());
  std::sort(c.begin(), c.end(), p);
  auto c_l = std::unique(c.begin(), c.end(), helpers::not_fn(p));

  using reverse_it = typename C::reverse_iterator;
  auto move_reverse_it =
      [](auto it) { return std::make_move_iterator(reverse_it(it)); };

  auto reverse_remainig_buf_range = helpers::set_union_adaptive_into_tail(
      reverse_it(new_c.end()),                               // buffer
      reverse_it(orig_l), reverse_it(new_c.begin()),         // original
      move_reverse_it(c_l), move_reverse_it(c.begin()),      // new elements
      helpers::strict_oposite(p));                           // greater

  new_c.erase(reverse_remainig_buf_range.second.base(),
              reverse_remainig_buf_range.first.base());
  c = std::move(new_c);
}

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          ForwardIterator<I> &&                       //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void use_end_buffer_new_size(C& c, I f, I l, P p) {
  helpers::use_end_buffer_impl(c, c.size() + std::distance(f, l), f, l, p);
}

}  // bulk_insert
