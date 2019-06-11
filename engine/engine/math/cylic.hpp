#pragma once

template<typename UintType>
struct cyclic {
   static_assert(std::is_unsigned_v<UintType>, "cyclic only works with unsigned type");

   UintType value = UintType(0);

   cyclic& operator++() { ++value; return *this; };
   cyclic  operator++(int) { value++; return *this; };
   constexpr cyclic() = default;
   constexpr cyclic(UintType v): value(v) {}
   operator UintType() const { return value; }
};

template<typename U>
constexpr bool operator<(const cyclic<U>& a, const cyclic<U>& b)
{
   constexpr size_t HALF_MAX = 0x1ui64 << (8 * sizeof(U) - 1);
   size_t diff = b - a;
   return diff > 0u && (diff <= HALF_MAX - 1);
}

template<typename U>
constexpr bool operator<=(const cyclic<U>& a, const cyclic<U>& b)
{
  constexpr size_t HALF_MAX = 0x1ui64 << (8 * sizeof(U) - 1);
  size_t diff = b - a;
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