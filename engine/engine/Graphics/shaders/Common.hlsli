#ifndef __INCLUDE_COMMON__
#define __INCLUDE_COMMON__

const static float PI = 3.1415926535897932f;

float Square(float x)
{
   return x * x;
}

float2 Square(float2 x)
{
   return x * x;
}

float3 Square(float3 x)
{
   return x * x;
}

float4 Square(float4 x)
{
   return x * x;
}


#define Pow2(x) ((x)*(x))
#define Pow3(x) ((x)*(x)*(x))

float Pow4(float x)
{
   float xx = x * x;
   return xx * xx;
}

float2 Pow4(float2 x)
{
   float2 xx = x * x;
   return xx * xx;
}

float3 Pow4(float3 x)
{
   float3 xx = x * x;
   return xx * xx;
}

float4 Pow4(float4 x)
{
   float4 xx = x * x;
   return xx * xx;
}

float Pow5(float x)
{
   float xx = x * x;
   return xx * xx * x;
}

float2 Pow5(float2 x)
{
   float2 xx = x * x;
   return xx * xx * x;
}

float3 Pow5(float3 x)
{
   float3 xx = x * x;
   return xx * xx * x;
}

float4 Pow5(float4 x)
{
   float4 xx = x * x;
   return xx * xx * x;
}

float Pow6(float x)
{
   float xx = x * x;
   return xx * xx * xx;
}

float2 Pow6(float2 x)
{
   float2 xx = x * x;
   return xx * xx * xx;
}

float3 Pow6(float3 x)
{
   float3 xx = x * x;
   return xx * xx * xx;
}

float4 Pow6(float4 x)
{
   float4 xx = x * x;
   return xx * xx * xx;
}

float3 TBNToWorld(float3 position, float3 normal, float3 tangent)
{
   float3x3 tbn = transpose(
      float3x3(tangent, normalize(cross(normal, tangent)), normal)
   );

   return normalize(mul(tbn, position));

}


#endif