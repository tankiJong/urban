#include "engine/pch.h"
#include "primitives.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

const mat44 mat44::Identity = {};



////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

float4 mat44::X() const
{
   return { ix, jx, kx, tx };
}

float4 mat44::Y() const
{
   return { iy, jy, ky, ty };
}

float4 mat44::Z() const
{
   return { iz, jz, kz, tz };
}

float4 mat44::W() const
{
   return { iw, jw, kw, tw };
}

mat44 mat44::operator*( const mat44& rhs ) const
{
   float4
    pi(rhs.ix, rhs.iy, rhs.iz, rhs.iw),
    pj(rhs.jx, rhs.jy, rhs.jz, rhs.jw),
    pk(rhs.kx, rhs.ky, rhs.kz, rhs.kw),
    pt(rhs.tx, rhs.ty, rhs.tz, rhs.tw);

  float4 _x(ix, jx, kx, tx), _y(iy, jy, ky, ty), _z(iz, jz, kz, tz), _w(iw, jw, kw, tw);

  return {
   _x.Dot(pi), _x.Dot(pj), _x.Dot(pk), _x.Dot(pt),
   _y.Dot(pi), _y.Dot(pj), _y.Dot(pk), _y.Dot(pt),
   _z.Dot(pi), _z.Dot(pj), _z.Dot(pk), _z.Dot(pt),
   _w.Dot(pi), _w.Dot(pj), _w.Dot(pk), _w.Dot(pt)
  };
}

float4 mat44::operator*( const float4& rhs ) const
{
     return {
    X().Dot(rhs),
    Y().Dot(rhs),
    Z().Dot(rhs),
    W().Dot(rhs)
  };
}

mat44 mat44::Transpose() const
{
   return {
      ix, iy, iz, iw,
      jx, jy, jz, jw,
      kx, ky, kz, kw,
      tx, ty, tz, tw,
   };
}

mat44 mat44::Inverse() const
{
   double inv[16];
   double m[16];
   uint   ii;

   for(ii = 0; ii < 16; ++ii) { m[ii] = double( data[ii] ); }

   inv[0] = m[5] * m[10] * m[15] -
            m[5] * m[11] * m[14] -
            m[9] * m[6] * m[15] +
            m[9] * m[7] * m[14] +
            m[13] * m[6] * m[11] -
            m[13] * m[7] * m[10];

   inv[4] = -m[4] * m[10] * m[15] +
            m[4] * m[11] * m[14] +
            m[8] * m[6] * m[15] -
            m[8] * m[7] * m[14] -
            m[12] * m[6] * m[11] +
            m[12] * m[7] * m[10];

   inv[8] = m[4] * m[9] * m[15] -
            m[4] * m[11] * m[13] -
            m[8] * m[5] * m[15] +
            m[8] * m[7] * m[13] +
            m[12] * m[5] * m[11] -
            m[12] * m[7] * m[9];

   inv[12] = -m[4] * m[9] * m[14] +
             m[4] * m[10] * m[13] +
             m[8] * m[5] * m[14] -
             m[8] * m[6] * m[13] -
             m[12] * m[5] * m[10] +
             m[12] * m[6] * m[9];

   inv[1] = -m[1] * m[10] * m[15] +
            m[1] * m[11] * m[14] +
            m[9] * m[2] * m[15] -
            m[9] * m[3] * m[14] -
            m[13] * m[2] * m[11] +
            m[13] * m[3] * m[10];

   inv[5] = m[0] * m[10] * m[15] -
            m[0] * m[11] * m[14] -
            m[8] * m[2] * m[15] +
            m[8] * m[3] * m[14] +
            m[12] * m[2] * m[11] -
            m[12] * m[3] * m[10];

   inv[9] = -m[0] * m[9] * m[15] +
            m[0] * m[11] * m[13] +
            m[8] * m[1] * m[15] -
            m[8] * m[3] * m[13] -
            m[12] * m[1] * m[11] +
            m[12] * m[3] * m[9];

   inv[13] = m[0] * m[9] * m[14] -
             m[0] * m[10] * m[13] -
             m[8] * m[1] * m[14] +
             m[8] * m[2] * m[13] +
             m[12] * m[1] * m[10] -
             m[12] * m[2] * m[9];

   inv[2] = m[1] * m[6] * m[15] -
            m[1] * m[7] * m[14] -
            m[5] * m[2] * m[15] +
            m[5] * m[3] * m[14] +
            m[13] * m[2] * m[7] -
            m[13] * m[3] * m[6];

   inv[6] = -m[0] * m[6] * m[15] +
            m[0] * m[7] * m[14] +
            m[4] * m[2] * m[15] -
            m[4] * m[3] * m[14] -
            m[12] * m[2] * m[7] +
            m[12] * m[3] * m[6];

   inv[10] = m[0] * m[5] * m[15] -
             m[0] * m[7] * m[13] -
             m[4] * m[1] * m[15] +
             m[4] * m[3] * m[13] +
             m[12] * m[1] * m[7] -
             m[12] * m[3] * m[5];

   inv[14] = -m[0] * m[5] * m[14] +
             m[0] * m[6] * m[13] +
             m[4] * m[1] * m[14] -
             m[4] * m[2] * m[13] -
             m[12] * m[1] * m[6] +
             m[12] * m[2] * m[5];

   inv[3] = -m[1] * m[6] * m[11] +
            m[1] * m[7] * m[10] +
            m[5] * m[2] * m[11] -
            m[5] * m[3] * m[10] -
            m[9] * m[2] * m[7] +
            m[9] * m[3] * m[6];

   inv[7] = m[0] * m[6] * m[11] -
            m[0] * m[7] * m[10] -
            m[4] * m[2] * m[11] +
            m[4] * m[3] * m[10] +
            m[8] * m[2] * m[7] -
            m[8] * m[3] * m[6];

   inv[11] = -m[0] * m[5] * m[11] +
             m[0] * m[7] * m[9] +
             m[4] * m[1] * m[11] -
             m[4] * m[3] * m[9] -
             m[8] * m[1] * m[7] +
             m[8] * m[3] * m[5];

   inv[15] = m[0] * m[5] * m[10] -
             m[0] * m[6] * m[9] -
             m[4] * m[1] * m[10] +
             m[4] * m[2] * m[9] +
             m[8] * m[1] * m[6] -
             m[8] * m[2] * m[5];

   double det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
   det = 1.0 / det;

   mat44 inver;
   for(ii = 0; ii < 16; ii++) { inver.data[ii] = (float)(inv[ii] * det); }

   return inver;
}

euler mat44::Euler() const
{
   float3 ii = i.xyz().Norm();
   float3 jj = j.xyz().Norm();
   float3 kk = k.xyz().Norm();

   euler e;

   e.x = asinf( kk.y );
   // cosx = 0
   if(kk.y == 1.f || kk.y == -1.f) {
      e.y = 0;
      e.z = atan2f( jj.x, ii.x );

      return e;
   }

   e.z = atan2f( -ii.y, jj.y );
   e.y = atan2f( -kk.x, kk.z );

   e = e * euler(R2D);

   return e;
}

mat44 mat44::Perspective( float fovDeg, float aspect, float nz, float fz )
{
   float d = 1.f / tanf(D2R * fovDeg * .5f);
   return {
      d / aspect, 0, 0, 0,
      0,          d, 0, 0,
      0,          0, (fz) / (fz-nz), nz*fz/(nz-fz),
      0,          0, 1, 0
   };
}

mat44 mat44::LookAt( const float3& position, const float3& target, const float3& _up )
{
     mat44 t = mat44::Translation(position);

  float3 _forward = (target - position).Norm();

  float dot = _forward.Dot(_up);
  float3 _right;

  float3 newUp;
  if(dot == 1.f || dot == -1.f) {
    newUp = _forward.Cross(float3::X).Norm();
    _right = newUp.Cross(_forward).Norm();
  } else {
    _right = _up.Cross(_forward).Norm();
    newUp = _forward.Cross(_right);
  }

  mat44 r(float4(_right, 0),
          float4(newUp, 0),
          float4(_forward, 0));

  return t * r;
}

mat44 mat44::Translation( const float3& translation )
{
   return {
      1, 0, 0, translation.x,
      0, 1, 0, translation.y,
      0, 0, 1, translation.z,
      0, 0, 0, 1
   };
}

mat44 mat44::Rotation( const euler& rotation )
{
   float x = rotation.x * D2R, y = rotation.y * D2R, z = rotation.z * D2R;

   float cx = cosf(x), cy = cosf(y), cz = cosf(z);
   float sx = sinf(x), sy = sinf(y), sz = sinf(z);

   mat44 re{
      cz*cy - sz*sx*sy,      sz*cy + cz * sx*sy,   -cx * sy,    0,
      -sz * cx,                           cz*cx,         sx,    0,
      sz*sx*cy + cz * sy, -cz * sx*cy + sz * sy,      cx*cy,    0,
      0,                                      0,          0,    1
   };

  return re;
}

mat44 mat44::Scale( const float3& scale )
{
   return {
      scale.x,  0,       0,       0,
      0,        scale.y, 0,       0,
      0,        0,       scale.z, 0,
      0,        0,       0,       1
   };
}
