﻿#pragma once
#include "engine/framework/Camera.hpp"

class MvCamera: public Camera {
public:
   void OnUpdate();

protected:
   float3 mOrbitCenter = {};
   float  mTheta = 0;
   float  mPhi = 0;
   float  mRadius = 2.f;
};
