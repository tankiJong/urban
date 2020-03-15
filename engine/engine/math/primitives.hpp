#pragma once

#include <stdint.h>
#include <xmmintrin.h>

///////////////////////////// vector /////////////////////////////

template< size_t N, typename T >
struct vec;

template< typename T >
struct vec<2, T> {
   template< typename U >
   using vec2 = vec<2, U>;

   T x, y;

   constexpr vec() = default;
   constexpr vec(T v): x(v), y(v) {}
   constexpr vec( T x, T y ) : x( x ), y( y ) {}

   template<typename U>
   constexpr explicit vec(const vec2<U>& v): x((T)v.x), y((T)v.y) {}

   T Dot( const vec2<T>& rhs ) const { return x * rhs.x + y * rhs.y; }
   T Len2() const { return Dot( *this ); }
   T Len()  const { return T( sqrt( double( Len2() ) ) ); }

   static const vec2<T> Zero;
   static const vec2<T> One;

};

template<typename T> const vec<2, T> vec<2, T>::Zero{ 0, 0 };
template<typename T> const vec<2, T> vec<2, T>::One { 1, 1 };

// --------------------------------------------------------------- //

template< typename T >
struct vec<3, T> {
   template< typename U >
   using vec3 = vec<3, U>;

   T x, y, z;

   constexpr vec() = default;
   constexpr vec( T v ) : x( v ) , y( v ) , z( v ) {}
   constexpr vec( T x, T y, T z ) : x( x ) , y( y ) , z( z ) {}
   constexpr vec( const vec<2, T>& xy, T z ): x( xy.x ), y( xy.y ), z( z ) {}

   template< typename U, typename = std::enable_if_t<!std::is_same_v<T, U>> >
   explicit operator vec3<U>() const { return vec3<U>{ U( x ), U( y ), U( z ) }; }

   T Dot( const vec3<T>& rhs ) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
   T Len2() const { return Dot( *this ); }
   T Len()  const { return T( sqrt( double( Len2() ) ) ); }
   vec3<T> Norm() const { return *this / vec3<T>{ Len() }; }

   // left hand, x.cross(y) == z
   vec3<T> Cross( const vec3<T>& rhs ) const;;

   vec<2, T> xy() const { return { x, y }; }
   vec<2, T> xz() const { return { x, z }; }
   vec<2, T> yz() const { return { y, z }; }

   static const vec3<T> Zero;
   static const vec3<T> One;
   static const vec3<T> X;
   static const vec3<T> Y;
   static const vec3<T> Z;
};

template< typename T > const vec<3, T> vec<3, T>::Zero{ 0, 0, 0 };
template< typename T > const vec<3, T> vec<3, T>::One { 1, 1, 1 };
template< typename T > const vec<3, T> vec<3, T>::X   { 1, 0, 0 };
template< typename T > const vec<3, T> vec<3, T>::Y   { 0, 1, 0 };
template< typename T > const vec<3, T> vec<3, T>::Z   { 0, 0, 1 };

// --------------------------------------------------------------- //

template< typename T >
struct vec<4, T> {
   template< typename U >
   using vec4 = vec<4, U>;

   T x, y, z, w;

   constexpr vec() = default;
   constexpr vec( T v ) : x( v ) , y( v ) , z( v ), w( v ) {}
   constexpr vec( T x, T y, T z, T w ) : x( x ) , y( y ) , z( z ) , w( w ) {}
   constexpr vec( const vec<3, T>& xyz, T w ): x( xyz.x ), y( xyz.y ), z( xyz.z ), w( w ) {}
   constexpr vec( const vec<2, T>& xy, T z, T w ): x( xy.x ), y( xy.y ), z( z ), w( w ) {}

   template< typename U, typename = std::enable_if_t<!std::is_same_v<T, U>> >
   explicit operator vec4<U>() const { return vec4<U>{ U( x ), U( y ), U( z ), U( w ) }; }

   T Dot( const vec4<T>& rhs ) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }
   T Len2() const { return Dot( *this ); }

   vec<3, T> xyz() const { return { x, y, z }; }
   vec<2, T> xy() const { return { x, y }; }
   vec<2, T> xz() const { return { x, z }; }
   vec<2, T> zw() const { return { z, w }; }
};

template<typename T> vec<2, T>    operator - ( const vec<2, T>& rhs ) { return { - rhs.x, - rhs.y }; }
template<typename T> vec<2, T>    operator + ( const vec<2, T>& lhs, const vec<2, T>& rhs )  {  return { lhs.x + rhs.x, lhs.y + rhs.y }; }
template<typename T> vec<2, T>    operator - ( const vec<2, T>& lhs, const vec<2, T>& rhs )  {  return { lhs.x - rhs.x, lhs.y - rhs.y }; }
template<typename T> vec<2, T>    operator * ( const vec<2, T>& lhs, const vec<2, T>& rhs )  {  return { lhs.x * rhs.x, lhs.y * rhs.y }; }
template<typename T> vec<2, T>    operator / ( const vec<2, T>& lhs, const vec<2, T>& rhs )  {  return { lhs.x / rhs.x, lhs.y / rhs.y }; }
template<typename T> vec<2, bool> operator > ( const vec<2, T>& lhs, const vec<2, T>& rhs )  {  return { lhs.x > rhs.x, lhs.y > rhs.y }; }
template<typename T> vec<2, bool> operator < ( const vec<2, T>& lhs, const vec<2, T>& rhs )  {  return { lhs.x < rhs.x, lhs.y < rhs.y }; }
template<typename T> vec<2, bool> operator >=( const vec<2, T>& lhs, const vec<2, T>& rhs )  {  return { lhs.x >= rhs.x, lhs.y >= rhs.y }; }
template<typename T> vec<2, bool> operator <=( const vec<2, T>& lhs, const vec<2, T>& rhs )  {  return { lhs.x <= rhs.x, lhs.y <= rhs.y }; }
template<typename T> vec<2, T>&    operator += ( vec<2, T>& lhs, const vec<2, T>& rhs )  { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
template<typename T> vec<2, T>&    operator -= ( vec<2, T>& lhs, const vec<2, T>& rhs )  { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
template<typename T> vec<2, T>&    operator *= ( vec<2, T>& lhs, const vec<2, T>& rhs )  { lhs.x *= rhs.x; lhs.y *= rhs.y; return lhs; }
template<typename T> vec<2, T>&    operator /= ( vec<2, T>& lhs, const vec<2, T>& rhs )  { lhs.x /= rhs.x; lhs.y /= rhs.y; return lhs; }
template<typename T> vec<2, T>    operator + ( const vec<2, T>& lhs, T v )  {  return { lhs.x + v, lhs.y + v }; }
template<typename T> vec<2, T>    operator - ( const vec<2, T>& lhs, T v )  {  return { lhs.x - v, lhs.y - v }; }
template<typename T> vec<2, T>    operator * ( const vec<2, T>& lhs, T v )  {  return { lhs.x * v, lhs.y * v }; }
template<typename T> vec<2, T>    operator / ( const vec<2, T>& lhs, T v )  {  return { lhs.x / v, lhs.y / v }; }

template<typename T> vec<3, T>    operator -  ( const vec<3, T>& rhs ) { return { - rhs.x, - rhs.y, - rhs.z }; }
template<typename T> vec<3, T>    operator +  ( const vec<3, T>& lhs, const vec<3, T>& rhs )  {  return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z }; }
template<typename T> vec<3, T>    operator -  ( const vec<3, T>& lhs, const vec<3, T>& rhs )  {  return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z }; }
template<typename T> vec<3, T>    operator *  ( const vec<3, T>& lhs, const vec<3, T>& rhs )  {  return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z }; }
template<typename T> vec<3, T>    operator /  ( const vec<3, T>& lhs, const vec<3, T>& rhs )  {  return { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z }; }
template<typename T> vec<3, bool> operator >  ( const vec<3, T>& lhs, const vec<3, T>& rhs )  {  return { lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z }; }
template<typename T> vec<3, bool> operator <  ( const vec<3, T>& lhs, const vec<3, T>& rhs )  {  return { lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z }; }
template<typename T> vec<3, bool> operator >= ( const vec<3, T>& lhs, const vec<3, T>& rhs )  {  return { lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z }; }
template<typename T> vec<3, bool> operator <= ( const vec<3, T>& lhs, const vec<3, T>& rhs )  {  return { lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z }; }
template<typename T> vec<3, T>&   operator += ( vec<3, T>& lhs, const vec<3, T>& rhs )  { lhs.x += rhs.x; lhs.y += rhs.y; lhs.z += rhs.z; return lhs; }
template<typename T> vec<3, T>&   operator -= ( vec<3, T>& lhs, const vec<3, T>& rhs )  { lhs.x -= rhs.x; lhs.y -= rhs.y; lhs.z -= rhs.z; return lhs; }
template<typename T> vec<3, T>&   operator *= ( vec<3, T>& lhs, const vec<3, T>& rhs )  { lhs.x *= rhs.x; lhs.y *= rhs.y; lhs.z *= rhs.z; return lhs; }
template<typename T> vec<3, T>&   operator /= ( vec<3, T>& lhs, const vec<3, T>& rhs )  { lhs.x /= rhs.x; lhs.y /= rhs.y; lhs.z /= rhs.z; return lhs; }
template<typename T> vec<3, T>    operator +  ( const vec<3, T>& lhs, T v )  {  return { lhs.x + v, lhs.y + v, lhs.z + v }; }
template<typename T> vec<3, T>    operator -  ( const vec<3, T>& lhs, T v )  {  return { lhs.x - v, lhs.y - v, lhs.z - v }; }
template<typename T> vec<3, T>    operator *  ( const vec<3, T>& lhs, T v )  {  return { lhs.x * v, lhs.y * v, lhs.z * v }; }
template<typename T> vec<3, T>    operator /  ( const vec<3, T>& lhs, T v )  {  return { lhs.x / v, lhs.y / v, lhs.z / v }; }
template<typename T> vec<3, T>    operator +  ( T v, const vec<3, T>& rhs )  {  return { v + rhs.x, v + rhs.y, v + rhs.z }; }
template<typename T> vec<3, T>    operator -  ( T v, const vec<3, T>& rhs )  {  return { v - rhs.x, v - rhs.y, v - rhs.z }; }
template<typename T> vec<3, T>    operator *  ( T v, const vec<3, T>& rhs )  {  return { v * rhs.x, v * rhs.y, v * rhs.z }; }
template<typename T> vec<3, T>    operator /  ( T v, const vec<3, T>& rhs )  {  return { v / rhs.x, v / rhs.y, v / rhs.z }; }

template<typename T> vec<4, T>    operator - ( const vec<4, T>& rhs ) { return { - rhs.x, - rhs.y, - rhs.z, - rhs.w }; }
template<typename T> vec<4, T>    operator + ( const vec<4, T>& lhs, const vec<4, T>& rhs )  {  return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w }; }
template<typename T> vec<4, T>    operator - ( const vec<4, T>& lhs, const vec<4, T>& rhs )  {  return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w }; }
template<typename T> vec<4, T>    operator * ( const vec<4, T>& lhs, const vec<4, T>& rhs )  {  return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w }; }
template<typename T> vec<4, T>    operator / ( const vec<4, T>& lhs, const vec<4, T>& rhs )  {  return { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w }; }
template<typename T> vec<4, bool> operator > ( const vec<4, T>& lhs, const vec<4, T>& rhs )  {  return { lhs.x > rhs.x, lhs.y > rhs.y, lhs.z > rhs.z, lhs.w > rhs.w }; }
template<typename T> vec<4, bool> operator < ( const vec<4, T>& lhs, const vec<4, T>& rhs )  {  return { lhs.x < rhs.x, lhs.y < rhs.y, lhs.z < rhs.z, lhs.w < rhs.w }; }
template<typename T> vec<4, bool> operator >=( const vec<4, T>& lhs, const vec<4, T>& rhs )  {  return { lhs.x >= rhs.x, lhs.y >= rhs.y, lhs.z >= rhs.z, lhs.w >= rhs.w }; }
template<typename T> vec<4, bool> operator <=( const vec<4, T>& lhs, const vec<4, T>& rhs )  {  return { lhs.x <= rhs.x, lhs.y <= rhs.y, lhs.z <= rhs.z, lhs.w <= rhs.w }; }
template<typename T> vec<4, T>&   operator += ( vec<4, T>& lhs, const vec<4, T>& rhs )  { lhs.x += rhs.x; lhs.y += rhs.y; lhs.z += rhs.z; lhs.w += rhs.w; return lhs; }
template<typename T> vec<4, T>&   operator -= ( vec<4, T>& lhs, const vec<4, T>& rhs )  { lhs.x -= rhs.x; lhs.y -= rhs.y; lhs.z -= rhs.z; lhs.w -= rhs.w; return lhs; }
template<typename T> vec<4, T>&   operator *= ( vec<4, T>& lhs, const vec<4, T>& rhs )  { lhs.x *= rhs.x; lhs.y *= rhs.y; lhs.z *= rhs.z; lhs.w *= rhs.w; return lhs; }
template<typename T> vec<4, T>&   operator /= ( vec<4, T>& lhs, const vec<4, T>& rhs )  { lhs.x /= rhs.x; lhs.y /= rhs.y; lhs.z /= rhs.z; lhs.w /= rhs.w; return lhs; }
template<typename T> vec<4, T>    operator + ( const vec<4, T>& lhs, T v )  {  return { lhs.x + v, lhs.y + v, lhs.z + v, lhs.w + v }; }
template<typename T> vec<4, T>    operator - ( const vec<4, T>& lhs, T v )  {  return { lhs.x - v, lhs.y - v, lhs.z - v, lhs.w - v }; }
template<typename T> vec<4, T>    operator * ( const vec<4, T>& lhs, T v )  {  return { lhs.x * v, lhs.y * v, lhs.z * v, lhs.w * v }; }
template<typename T> vec<4, T>    operator / ( const vec<4, T>& lhs, T v )  {  return { lhs.x / v, lhs.y / v, lhs.z / v, lhs.w / v }; }

inline vec<2, bool> operator &&( const vec<2, bool>& lhs, const vec<2, bool>& rhs ) {  return { lhs.x && rhs.x, lhs.y && rhs.y }; }
inline vec<2, bool> operator ||( const vec<2, bool>& lhs, const vec<2, bool>& rhs ) {  return { lhs.x || rhs.x, lhs.y || rhs.y }; }

inline vec<3, bool> operator &&( const vec<3, bool>& lhs, const vec<3, bool>& rhs ) {  return { lhs.x && rhs.x, lhs.y && rhs.y, lhs.z && rhs.z }; }
inline vec<3, bool> operator ||( const vec<3, bool>& lhs, const vec<3, bool>& rhs ) {  return { lhs.x || rhs.x, lhs.y || rhs.y, lhs.z || rhs.z }; }

inline vec<4, bool> operator &&( const vec<4, bool>& lhs, const vec<4, bool>& rhs ) {  return { lhs.x && rhs.x, lhs.y && rhs.y, lhs.z && rhs.z, lhs.w && rhs.w }; }
inline vec<4, bool> operator ||( const vec<4, bool>& lhs, const vec<4, bool>& rhs ) {  return { lhs.x || rhs.x, lhs.y || rhs.y, lhs.z || rhs.z, lhs.w || rhs.w }; }



template<size_t N, typename T> 
inline T Dot(const vec<N, T>& a, const vec<N, T>& b)
{
	return a.Dot(b);
}

template<typename T>
inline T Dot(const T& a, const T& b)
{
   return a * b;
}

template<typename T>
vec<3,T> Cross(const vec<3, T>& lhs, const vec<3, T>& rhs)
{
   return {
      lhs.y * rhs.z - lhs.z * rhs.y,
      lhs.z * rhs.x - lhs.x * rhs.z,
      lhs.x * rhs.y - lhs.y * rhs.x,
   };
}

// --------------------------------------------------------------- //

using int2   = vec<2, int32_t>;
using uint2  = vec<2, uint32_t>;
using float2 = vec<2, float>;

using uint3  = vec<3, uint32_t>;
using float3 = vec<3, float>;
using euler  = float3;

using uint4  = vec<4, uint32_t>;
using float4 = vec<4, float>;
using uchar4 = vec<4, uint8_t>;

template< typename T > __forceinline vec<3, T> vec<3, T>::Cross( const vec3<T>& rhs ) const { return ::Cross( *this, rhs ); }

///////////////////////////// matrix /////////////////////////////

struct mat44;
using float4x4 = mat44;

struct mat44 {
   union {
      struct { float
         ix, iy, iz, iw,
         jx, jy, jz, jw,
         kx, ky, kz, kw,
         tx, ty, tz, tw;
      };

      struct {
         float4 i, j, k, t;
      };

      float data[16] = {};
   };

   constexpr mat44()
   {
      ix = 1, iy = 0, iz = 0, iw = 0,
      jx = 0, jy = 1, jz = 0, jw = 0,
      kx = 0, ky = 0, kz = 1, kw = 0,
      tx = 0, ty = 0, tz = 0, tw = 1;
   }

   constexpr mat44(float ix, float jx, float kx, float tx,
         float iy, float jy, float ky, float ty,
         float iz, float jz, float kz, float tz,
         float iw, float jw, float kw, float tw)
      : ix( ix ), jx( jx ), kx( kx ), tx( tx )
      , iy( iy ), jy( jy ), ky( ky ), ty( ty )
      , iz( iz ), jz( jz ), kz( kz ), tz( tz )
      , iw( iw ), jw( jw ), kw( kw ), tw( tw ) {}

   constexpr explicit mat44( float4 col0, float4 col1, float4 col2, float4 col3 = { 0, 0, 0, 1 } )
      : i( col0 )
    , j( col1 )
    , k( col2 )
    , t( col3 ) {}

   constexpr explicit mat44( const float3& right, const float3& up, const float3& forward, const float3& translation )
      : i( right, 0 )
    , j( up, 0 )
    , k( forward, 0 )
    , t( translation, 1 ) {}

   float4 X() const;
   float4 Y() const;
   float4 Z() const;
   float4 W() const;

   mat44  operator*( const mat44& rhs ) const;
   float4 operator*( const float4& rhs ) const;
   bool   operator==( const mat44& rhs ) const;

   mat44 Transpose() const;
   mat44 Inverse() const;
   euler Euler() const;

   static mat44       Perspective( float fovDeg, float aspect, float nz, float fz );
   static mat44       Perspective( float fovDeg, float width, float height, float nz, float fz );
   static mat44       LookAt( const float3& position, const float3& target, const float3& up = float3::Y );

   static mat44       Translation( const float3& translation );
   // ZXY Rotation
   static mat44       Rotation( const euler& translation );
   static mat44       Scale( const float3& scale );
   static const mat44 Identity;
};

