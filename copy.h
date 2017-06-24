#include <algorithm>
#include <iterator>
#include <type_traits>

namespace helpers {

template <typename I>
using ValueType = typename std::iterator_traits<I>::value_type;

template <typename I>
constexpr bool should_unwrap_move_iterator =
          std::is_trivially_copy_assignable<ValueType<I>>::value;


template <typename I>
std::enable_if_t<should_unwrap_move_iterator<I>, I>
 unwrap_move_iterator(std::move_iterator<I> i) {
  return i.base();
}

template <typename I>
std::enable_if_t<!should_unwrap_move_iterator<I>,
                 std::move_iterator<I>>
unwrap_move_iterator(std::move_iterator<I> i) {
  return i;
}

struct copy_impl {
  template <typename I, typename O>
  static O run_copy(I f, I l, O o) {
    return std::copy(f, l, o);
  }

  template <typename I, typename O>
  static O run_copy_backward(I f, I l, O o) {
    return std::copy_backward(f, l, o);
  }

  template <typename I, typename O>
  static O run_copy(std::move_iterator<I> f, std::move_iterator<I> l, O o) {
    return run_copy(unwrap_move_iterator(f), unwrap_move_iterator(l), o);
  }

  template <typename I, typename O>
  static O run_copy_backward(std::move_iterator<I> f, std::move_iterator<I> l, O o) {
    return run_copy_backward(unwrap_move_iterator(f), unwrap_move_iterator(l), o);
  }

  template <typename I, typename O>
  static std::reverse_iterator<O> run_copy(std::reverse_iterator<I> f,
                                           std::reverse_iterator<I> l,
                                           std::reverse_iterator<O> o) {
    return std::make_reverse_iterator(run_copy_backward(l.base(), f.base(), o.base()));
  }

  template <typename I, typename O>
  static std::reverse_iterator<O> run_copy_backward(std::reverse_iterator<I> f,
                    std::reverse_iterator<I> l,
                    std::reverse_iterator<O> o) {
    return std::make_reverse_iterator(run_copy(l.base(), f.base(), o.base()));
  }
};

template <typename I, typename O>
// requires InputIterator<I> && OutputIterator<O>
O copy(I f, I l, O o) {
  return copy_impl().run_copy(f, l, o);
}

}  // namespace helpers

#if 0  // tests for compiler explorer.

void call_mr(int* f, int* l, int* o) {
    auto rf = std::make_reverse_iterator(l);
    auto rl = std::make_reverse_iterator(f);
    auto ro = std::make_reverse_iterator(o);
    auto mrf = std::make_move_iterator(rf);
    auto mrl = std::make_move_iterator(rl);
    helpers::copy(mrf, mrl, ro);
}

void call_rm(int* f, int* l, int* o) {
    auto mf = std::make_move_iterator(f);
    auto ml = std::make_move_iterator(l);
    auto rmf = std::make_reverse_iterator(mf);
    auto rml = std::make_reverse_iterator(ml);
    auto ro = std::make_reverse_iterator(o);
    helpers::copy(rmf, rml, ro);
}


void call_rr(int* f, int* l, int* o) {
    auto rf = std::make_reverse_iterator(l);
    auto rl = std::make_reverse_iterator(f);
    auto ro = std::make_reverse_iterator(o);
    auto rrf = std::make_reverse_iterator(rl);
    auto rrl = std::make_reverse_iterator(rf);
    auto rro = std::make_reverse_iterator(ro);
    helpers::copy(rrf, rrl, rro);
}

void call_regular_copy(int* f, int* l, int* o) {
    std::copy(f, l, o);
}

#endif  // tests for compiler explorer.
