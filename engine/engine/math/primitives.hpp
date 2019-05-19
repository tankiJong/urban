#pragma once

template<typename T>
struct vec2 {
   T x, y;

   constexpr vec2() {}
   constexpr vec2( T x, T y ): x( x ), y( y ) {}
   T Dot(const vec2<T>& rhs) const { return x * rhs.x + y * rhs.y; }
   T Len2() const { return Dot(*this); }

   static const vec2<T> Zero;
   static const vec2<T> One;

};

template< typename T >
struct vec3 {
   T x, y, z;

   constexpr vec3() {}

   constexpr vec3( T x, T y, T z ): x( x ), y( y ), z( z ) {}

   T Dot( const vec3<T>& rhs ) const { return x * rhs + y * rhs.y + z * rhs.z; }
   T Len2() const { return Dot( *this ); }
};

template<typename T> const vec2<T> vec2<T>::Zero{ 0, 0 };
template<typename T> const vec2<T> vec2<T>::One { 1, 1 };


using uint2 = vec2<uint32_t>;
using float2 = vec2<float>;
