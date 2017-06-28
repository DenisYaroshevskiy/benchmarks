#include <algorithm>
#include <cstring>
#include <iterator>
#include <type_traits>

namespace helpers {

constexpr bool cpp14_fold_and() {
  return true;
}

template <typename... Bool>
constexpr bool cpp14_fold_and(bool b, Bool... bs) {
  return b && cpp14_fold_and(bs...);
}

template <typename T, typename U>
constexpr bool can_blast_bits_v =
    std::is_same<std::remove_const_t<T>, U>::value&&
        std::is_trivially_copy_assignable<U>::value;

template <typename I>
constexpr bool is_random_access_v =
    std::is_same<typename std::iterator_traits<I>::iterator_category,
                 std::random_access_iterator_tag>::value;

template <typename... I>
constexpr bool are_random_access_v = cpp14_fold_and(is_random_access_v<I>...);

template <typename I>
using ValueType = typename std::iterator_traits<I>::value_type;

template <typename I>
constexpr bool should_unwrap_move_iterator =
    std::is_trivially_copy_assignable<ValueType<I>>::value;

template <typename I>
std::enable_if_t<should_unwrap_move_iterator<I>, I> unwrap_move_iterator(
    std::move_iterator<I> i) {
  return i.base();
}

template <typename I>
std::enable_if_t<!should_unwrap_move_iterator<I>, std::move_iterator<I>>
unwrap_move_iterator(std::move_iterator<I> i) {
  return i;
}

template <typename TrivialCopyAlgorithms>
struct copy_iterator_unwrapper : TrivialCopyAlgorithms {
  using TrivialCopyAlgorithms::run_copy;
  using TrivialCopyAlgorithms::run_copy_backward;

  template <typename I, typename O>
  static O run_copy(std::move_iterator<I> f, std::move_iterator<I> l, O o) {
    return run_copy(unwrap_move_iterator(f), unwrap_move_iterator(l), o);
  }

  template <typename I, typename O>
  static O run_copy_backward(std::move_iterator<I> f,
                             std::move_iterator<I> l,
                             O o) {
    return run_copy_backward(unwrap_move_iterator(f), unwrap_move_iterator(l),
                             o);
  }

  template <typename I, typename O>
  static std::reverse_iterator<O> run_copy(std::reverse_iterator<I> f,
                                           std::reverse_iterator<I> l,
                                           std::reverse_iterator<O> o) {
    return std::make_reverse_iterator(
        run_copy_backward(l.base(), f.base(), o.base()));
  }

  template <typename I, typename O>
  static std::reverse_iterator<O> run_copy_backward(
      std::reverse_iterator<I> f,
      std::reverse_iterator<I> l,
      std::reverse_iterator<O> o) {
    return std::make_reverse_iterator(run_copy(l.base(), f.base(), o.base()));
  }
};

struct strict_copy_impl {
  template <typename T, typename U>
  static std::enable_if_t<can_blast_bits_v<T, U>, U*> run_copy(T* f,
                                                               T* l,
                                                               U* r) {
    const auto n = static_cast<std::size_t>(l - f);
    if (n > 0)
      std::memcpy(r, f, n * sizeof(U));
    return r + n;
  }

  template <typename I, typename O>
  static O run_copy(I f, I l, O r) {
    return std::copy(f, l, r);
  }

  template <typename I, typename O>
  static std::enable_if_t<are_random_access_v<I, O>, O> run_copy_backward(I f,
                                                                          I l,
                                                                          O r) {
    O f_o = r - (l - f);
    run_copy(f, l, f_o);
    return f_o;
  }

  template <typename I, typename O>
  static std::enable_if_t<!is_random_access_v<O>, O> run_copy_backward(I f,
                                                                       I l,
                                                                       O r) {
    std::copy_backward(f, l, r);
  }
};

struct std_copy_impl {
  template <typename I, typename O>
  static O run_copy(I f, I l, O o) {
    return std::copy(f, l, o);
  }

  template <typename I, typename O>
  static O run_copy_backward(I f, I l, O o) {
    return std::copy_backward(f, l, o);
  }
};

template <typename I, typename O>
O copy(I f, I l, O o) {
  return copy_iterator_unwrapper<std_copy_impl>{}.run_copy(f, l, o);
}

template <typename I, typename O>
O copy_backward(I f, I l, O o) {
  return copy_iterator_unwrapper<std_copy_impl>{}.run_copy_backward(f, l, o);
}

template <typename I, typename O>
O strict_copy(I f, I l, O o) {
  return copy_iterator_unwrapper<strict_copy_impl>{}.run_copy(f, l, o);
}

template <typename I, typename O>
O strict_copy_backward(I f, I l, O o) {
  return copy_iterator_unwrapper<strict_copy_impl>{}.run_copy_backward(f, l, o);
}

}  // namespace helpers

#if 0  // compiler explorer tests

void copy_mr(int* f, int* l, int* o) {
  auto rf = std::make_reverse_iterator(l);
  auto rl = std::make_reverse_iterator(f);
  auto ro = std::make_reverse_iterator(o);
  auto mrf = std::make_move_iterator(rf);
  auto mrl = std::make_move_iterator(rl);
  helpers::copy(mrf, mrl, ro);
}

void copy_rm(int* f, int* l, int* o) {
  auto mf = std::make_move_iterator(f);
  auto ml = std::make_move_iterator(l);
  auto rmf = std::make_reverse_iterator(mf);
  auto rml = std::make_reverse_iterator(ml);
  auto ro = std::make_reverse_iterator(o);
  helpers::copy(rmf, rml, ro);
}

void copy_rr(int* f, int* l, int* o) {
  auto rf = std::make_reverse_iterator(l);
  auto rl = std::make_reverse_iterator(f);
  auto ro = std::make_reverse_iterator(o);
  auto rrf = std::make_reverse_iterator(rl);
  auto rrl = std::make_reverse_iterator(rf);
  auto rro = std::make_reverse_iterator(ro);
  helpers::copy(rrf, rrl, rro);
}

void std_copy(int* f, int* l, int* o) {
  std::copy(f, l, o);
}

void strict_copy_mr(int* f, int* l, int* o) {
  auto rf = std::make_reverse_iterator(l);
  auto rl = std::make_reverse_iterator(f);
  auto ro = std::make_reverse_iterator(o);
  auto mrf = std::make_move_iterator(rf);
  auto mrl = std::make_move_iterator(rl);
  helpers::strict_copy(mrf, mrl, ro);
}

void strict_copy_rm(int* f, int* l, int* o) {
  auto mf = std::make_move_iterator(f);
  auto ml = std::make_move_iterator(l);
  auto rmf = std::make_reverse_iterator(mf);
  auto rml = std::make_reverse_iterator(ml);
  auto ro = std::make_reverse_iterator(o);
  helpers::strict_copy(rmf, rml, ro);
}

void strict_copy_rr(int* f, int* l, int* o) {
  auto rf = std::make_reverse_iterator(l);
  auto rl = std::make_reverse_iterator(f);
  auto ro = std::make_reverse_iterator(o);
  auto rrf = std::make_reverse_iterator(rl);
  auto rrl = std::make_reverse_iterator(rf);
  auto rro = std::make_reverse_iterator(ro);
  helpers::strict_copy(rrf, rrl, rro);
}

#endif
