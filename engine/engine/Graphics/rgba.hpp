#pragma once

struct rgba {
public:
   float r, g, b, a = 1.f;

   operator float4() const { return { r, g, b, a }; }
};
