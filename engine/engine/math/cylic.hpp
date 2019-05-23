#pragma once

template<typename UintType>
struct cyclic {
   static_assert(std::is_unsigned_v<UintType>, "cyclic only works with unsigned type");

   UintType value = UintType(0);

   constexpr cyclic() = default;
   constexpr cyclic(UintType v): value(v) {}
   operator UintType() const { return value; }
};

template<typename U>
constexpr bool operator<(const cyclic<U>& a, const cyclic<U>& b)
{
   constexpr U HALF_MAX = 0x1 << (8 * sizeof(U) - 1);
   U diff = b - a;
   return diff > 0 && (diff <= HALF_MAX - 1);
}

template<typename U>
constexpr bool operator<=(const cyclic<U>& a, const cyclic<U>& b)
{
  constexpr U HALF_MAX = 0x1 << (8 * sizeof(U) - 1);
  U diff = b - a;
  return diff <= HALF_MAX - 1;
}

template<typename U>
constexpr bool operator>(const cyclic<U>& a, const cyclic<U>& b)
{
  return !(a <= b);
}

template<typename U>
constexpr bool operator>=(const cyclic<U>& a, const cyclic<U>& b)
{
  return !(a < b);
}