#pragma once


struct uint2 {
   uint32_t x;
   uint32_t y;

   uint32_t dot(uint2 rhs) const { return x * rhs.x + y * rhs.y; }
   uint32_t length2() const { return dot(*this); }
};

