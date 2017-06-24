#pragma once

#include <algorithm>
#include <iterator>
#include <vector>

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

template <bool skip_duplicates,
          typename I1,
          typename I2,
          typename O,
          typename P>
// requires ForwardIterator<I1> &&
//          ForwardIterator<I2> &&
//          OutputIterator<O>
//          StrictWeakOrdering<P, ValueType<I>>
std::tuple<I1, I2, O> set_union_unique_intersecting_parts(I1 f1,
                                                          I1 l1,
                                                          I2 f2,
                                                          I2 l2,
                                                          O o,
                                                          P p) {
  auto advance_range = [&](auto& f, auto l, const auto& v) {
    auto m = lower_bound_biased(f, l, v, p);
    o = std::copy(f, m, o);
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

    if (skip_duplicates && !p(*f1, *f2))
      ++f2;
  }

  return {f1, f2, o};
}

template <bool skip_duplicates, typename I1, typename I2, typename P>
// requires ForwardIterator<I1> &&
//          ForwardIterator<I2> &&
//          StrictWeakOrdering<P, ValueType<I>>
std::pair<I1, I1> set_union_unique_merge_into_tail(I1 buf,
                                                   I1 f1,
                                                   I1 l1,
                                                   I2 f2,
                                                   I2 l2,
                                                   P p) {
  std::move_iterator<I1> move_f1;
  std::tie(move_f1, f2, buf) =
      set_union_unique_intersecting_parts<true>(std::make_move_iterator(f1),  //
                                                std::make_move_iterator(l1),  //
                                                f2, l2,                       //
                                                buf, p);                      //

  return {std::copy(f2, l2, buf), move_f1.base()};
}

template <typename I1, typename I2, typename O, typename P>
// requires ForwardIterator<I1> &&
//          ForwardIterator<I2> &&
//          OutputIterator<O>
//          StrictWeakOrdering<P, ValueType<I>>
O set_union_unique(I1 f1, I1 l1, I2 f2, I2 l2, O o, P p) {
  std::tie(f1, f2, o) =
      set_union_unique_intersecting_parts<true>(f1, l1, f2, l2, o, p);
  o = std::copy(f1, l1, o);
  return std::copy(f2, l2, o);
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
    auto where = helpers::lower_bound_biased(c.begin(), c.end(), *f, p);
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

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          ForwardIterator<I> &&                       //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void use_end_buffer(C& c, I f, I l, P p) {
  auto new_len = std::distance(f, l);
  auto original_size = c.size();
  c.reserve(c.size() + 2 * new_len);
  c.resize(c.size() + new_len);
  c.insert(c.end(), f, l);

  auto orig_f = c.begin();
  auto orig_l = c.begin() + original_size;
  auto f_in = c.end() - new_len;
  auto l_in = c.end();
  auto buf = f_in;

  std::sort(f_in, l_in, p);
  l_in = std::unique(f_in, l_in, helpers::not_fn(p));

  using reverse_it = typename C::reverse_iterator;
  auto move_reverse_it =
      [](auto it) { return std::make_move_iterator(reverse_it(it)); };

  auto reverse_remainig_buf_range =
      helpers::set_union_unique_merge_into_tail<true>(
          reverse_it(buf),                               // buffer
          reverse_it(orig_l), reverse_it(orig_f),        // original
          move_reverse_it(l_in), move_reverse_it(f_in),  // new elements
          helpers::strict_oposite(p));                   // greater

  auto remaining_buf = std::make_pair(reverse_remainig_buf_range.second.base(),
                                      reverse_remainig_buf_range.first.base());
  c.erase(remaining_buf.first, remaining_buf.second);
  c.erase(c.end() - new_len, c.end());
}

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          ForwardIterator<I> &&                       //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void use_end_buffer_skipping_duplicates(C& c, I f, I l, P p) {
  auto new_len = std::distance(f, l);
  auto original_size = c.size();
  c.resize(c.size() + new_len * 2);

  auto orig_f = c.begin();
  auto orig_l = c.begin() + original_size;
  auto f_in = c.end() - new_len;

  auto l_in = std::copy_if(f, l, f_in, [&](const auto& x) {
    auto found = std::lower_bound(orig_f, orig_l, x, p);
    return found == orig_l || p(x, *found);
  });
  std::sort(f_in, l_in, p);
  l_in = std::unique(f_in, l_in, helpers::not_fn(p));

  using reverse_it = typename C::reverse_iterator;
  auto move_reverse_it =
      [](auto it) { return std::make_move_iterator(reverse_it(it)); };

  auto new_end = orig_l + (l_in - f_in);
  helpers::set_union_unique_merge_into_tail<false>(
      reverse_it(new_end),                           // buffer
      reverse_it(orig_l), reverse_it(orig_f),        // original
      move_reverse_it(l_in), move_reverse_it(f_in),  // new elements
      helpers::strict_oposite(p));                   // greater
  c.erase(new_end, c.end());
}

template <typename C, typename I, typename P>
// requires Container<C> &&                             //
//          ForwardIterator<I> &&                       //
//          StrictWeakOrdering<P, ValueType<C>> &&      //
//          std::is_same_v<ValueType<C>, ValueType<I>>  //
void reallocate_and_merge(C& c, I f, I l, P p) {
  auto new_len = std::distance(f, l);
  auto original_size = c.size();
  c.insert(c.end(), f, l);
  auto f1 = c.data() + original_size;
  auto l1 = c.data() + c.size();
  std::sort(f1, l1, p);
  l1 = std::unique(f1, l1, helpers::not_fn(p));

  constexpr int c_grows_factor = 2;
  C new_c;
  new_c.reserve((original_size + new_len) * c_grows_factor);

  helpers::set_union_unique(std::make_move_iterator(c.begin()),
                            std::make_move_iterator(c.begin() + original_size),
                            std::make_move_iterator(f1),
                            std::make_move_iterator(l1),
                            std::back_inserter(new_c), p);
  c = std::move(new_c);
}

}  // bulk_insert
