#pragma once

struct urgba;
struct rgba {
public:
   float r = 0.f, g = 0.f, b = 0.f, a = 1.f;

   rgba(const float4& v)
   {
      *((float4*)this) = v;
   }
   rgba( const urgba& from );
   operator float4() const { return { r, g, b, a }; }
};


struct urgba
{
public:
	uint8_t r = 0, g = 0, b = 0, a = 255;
	urgba() = default;
	urgba(const rgba& v)
		: r(uint8_t(v.r * 255))
		, g(uint8_t(v.g * 255))
		, b(uint8_t(v.b * 255))
		, a(uint8_t(v.a * 255)) {}
   uint32_t Pack() const
   {
	   uint32_t ret = 0;
	   ret =
		     uint32_t(r)
		   | (uint32_t(g) << 8)
		   | (uint32_t(b) << 16)
		   | (uint32_t(a) << 24);
	   return ret;
   }


   friend urgba operator*(const urgba& lhs, float rhs)
   {
	   urgba ret = lhs;
	   ret.r = uint8_t((float)ret.r * rhs);
	   ret.g = uint8_t((float)ret.g * rhs);
	   ret.b = uint8_t((float)ret.b * rhs);
	   ret.a = uint8_t((float)ret.a * rhs);
	   return ret;
   }

   urgba& operator+(const urgba& rhs)
   {
	   r = r + rhs.r;
	   g = g + rhs.g;
	   b = b + rhs.b;
	   a = a + rhs.a;
	   return *this;
   }
   static urgba Unpack(uint32_t val)
   {
	   urgba ret;
	   ret.r = val & 0xff;
	   val >>= 8;
	   ret.g = val & 0xff;
	   val >>= 8;
	   ret.b = val & 0xff;
	   val >>= 8;
	   ret.a = val & 0xff;

	   return ret;
   }
};

inline rgba::rgba( const urgba& from )
   : r(float(from.r) / 255.f)
   , g(float(from.g) / 255.f)
   , b(float(from.b) / 255.f)
   , a(float(from.a) / 255.f) {}

