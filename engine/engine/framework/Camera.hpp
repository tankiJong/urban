#pragma once
#include "Transform.hpp"

struct camera_t {
   mat44 view;
   mat44 proj;
   mat44 invView;
   mat44 invProj;
};

class Camera {
public:

   camera_t ComputeCameraBlock() const;
   void     LookAt( const float3& position, const float3& target, const float3& up = float3::Y );
   void     SetProjection( const mat44& proj ) { mProjection = proj; };
   float3   WorldPosition() const { return mTransform.Position(); } 
   const Transform &WorldTransform() const { return mTransform; }
protected:
   Transform mTransform;
   mat44     mProjection;
};
