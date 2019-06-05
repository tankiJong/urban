#include "engine/pch.h"
#include "Transform.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

mat44 transform_t::LocalToWorld() const
{
   mat44 r = mat44::Rotation(rotation);
   mat44 s = mat44::Scale(scale);
   mat44 t = mat44::Translation(position);

   return t * r * s;
}

mat44 transform_t::WorldToLocal() const
{
   mat44 r = mat44::Rotation(rotation).Transpose();
   mat44 s = mat44::Scale({1.f / scale.x, 1.f / scale.y, 1.f / scale.z});
   mat44 t = mat44::Translation(-position);

   return s * r * t;
}

void transform_t::SetFromMatrix(const mat44& transform)
{
   position = transform.t.xyz();
   rotation = transform.Euler();
   scale = { transform.i.xyz().Len(), transform.j.xyz().Len(), transform.k.xyz().Len() };
}

float3 Transform::X() const
{
   return (LocalToWorld() * float4(float3::X, 0.f)).xyz();
}

float3 Transform::Y() const
{
   return (LocalToWorld() * float4(float3::Y, 0.f)).xyz();
}

float3 Transform::Z() const
{
   return (LocalToWorld() * float4(float3::Z, 0.f)).xyz();
}

mat44 Transform::LocalToWorld() const
{
   return WorldMatrix() * mLocalTransform.LocalToWorld();
}

mat44 Transform::WorldToLocal() const
{
   return mLocalTransform.WorldToLocal() * LocalMatrix();
}

void Transform::SetFromWorldTransform( mat44 transform )
{
     mat44 mat =
    (mParent == nullptr)
    ? transform
    : mParent->WorldToLocal() * transform;
    SetFromLocalTransform(mat);
}

void Transform::SetFromLocalTransform( mat44 transform )
{
   mLocalTransform.SetFromMatrix( transform );
}

mat44 Transform::LocalMatrix() const
{
   return (mParent == nullptr) ? mat44::Identity : mParent->WorldToLocal();
}
mat44 Transform::WorldMatrix() const
{
   return (mParent == nullptr) ? mat44::Identity : mParent->LocalToWorld();
}
