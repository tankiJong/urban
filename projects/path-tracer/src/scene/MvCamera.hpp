﻿#pragma once
#include "engine/framework/Camera.hpp"

class MvCamera: public Camera {
public:
   void OnUpdate();

protected:
   float3 mOrbitCenter = .002f * float3{ 250, 250, 250 };
   float  mTheta = -90;
   float  mPhi = 0;
   float  mRadius = 1.3f;
};
