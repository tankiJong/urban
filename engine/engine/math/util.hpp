#pragma once

#include "engine/pch.h"
#include <cmath>

#define M_PI	      3.14159265358979323846f
#define M_PI_2	      1.57079632679489661923f
#define M_PI_4	      0.78539816339744830961f
#define M_1_PI	      0.31830988618379067153f
#define M_2_PI	      0.63661977236758134307f
#define M_2_SQRTPI	1.12837916709551257390f
#define M_SQRT2	   1.41421356237309504880f
#define M_SQRT1_2	   0.70710678118654752440f
#define M_E	         2.71828182845904523536f
#define M_LOG2E	   1.44269504088896340736f
#define M_LOG10E	   0.43429448190325182765f
#define M_LN2	      0.69314718055994530941f
#define M_LN10	      2.30258509299404568402f


#define D2R          (M_PI / 360.f)
#define R2D          (360.f / M_PI)

template<class T> 
const T& min(const T& a, const T& b)
{
    return (b < a) ? b : a;
}

template<class T> 
const T& max(const T& a, const T& b)
{
    return (b < a) ? a : b;
}
