#pragma once

#include "engine/math/primitives.hpp"

struct transform_t {
   float3 position = { 0, 0, 0 };
   euler  rotation = { 0, 0, 0 };
   float3 scale    = { 1, 1, 1 };

   mat44 LocalToWorld() const;
   mat44 WorldToLocal() const;
   void  SetFromMatrix(const mat44& transform);
};

class Transform {
public:

   float3 X() const;
   float3 Y() const;
   float3 Z() const;

   float3 Position() const;
   euler  Rotation() const;
   float3 Scale() const;

   const float3& LocalPosition() const { return mLocalTransform.position; };
   const euler&  LocalRotation() const { return mLocalTransform.rotation; };
   const float3& LocalScale() const { return mLocalTransform.scale; };

   mat44 LocalToWorld() const;
   mat44 WorldToLocal() const;

   void TranslateLocal(const float3& translation) { mLocalTransform.position += translation; };
   void RotateLocal( const euler& rotation );
   void ScaleLocal( const float3& scale );

   void Translate( const float3& translation );
   void Rotate( const euler& rotation );
   void Scale( const float3& scale );

   void SetFromWorldTransform(mat44 transform);
   void SetFromLocalTransform(mat44 transform);

protected:
   mat44 LocalMatrix() const;
   mat44 WorldMatrix() const;

   transform_t mLocalTransform;
   const Transform* mParent = nullptr;

};
