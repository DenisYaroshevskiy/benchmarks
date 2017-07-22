#pragma once

#include <limits>

namespace helpers {

auto number_of_leading_zeros(unsigned int x) {
  return __builtin_clz(x);
}

auto number_of_leading_zeros(int x) {
  return number_of_leading_zeros(static_cast<unsigned int>(x));
}

auto number_of_leading_zeros(unsigned long x) {
  return __builtin_clzl(x);
}

auto number_of_leading_zeros(long x) {
  return number_of_leading_zeros(static_cast<unsigned long>(x));
}

auto number_of_leading_zeros(unsigned long long x) {
  return __builtin_clzll(x);
}

auto number_of_leading_zeros(long long x) {
  return number_of_leading_zeros(static_cast<unsigned long long>(x));
}

template <typename N>
auto first_significant_bit_pos(N n) {
  return std::numeric_limits<N>::digits - number_of_leading_zeros(n);
}

template <typename N>
auto partition_point_biased_sentinal(N n) {
  auto sentinal_pos = first_significant_bit_pos(n) >> 1;
  return (N(1) << sentinal_pos) - 1;
}

}  // namespace helpers
