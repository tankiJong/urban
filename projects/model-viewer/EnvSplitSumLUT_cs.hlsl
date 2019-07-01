#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(UAV(u0, numDescriptors = 1, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
   "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_ALL)," 

static const float M_PI = 3.1415926535f;
static const float M_2PI = 2 * M_PI;
static const float Eps = 0.00001;

static const uint kSampleCount = 1024;
static const float kInvSampleCount = 1.f / float(kSampleCount);

RWTexture2D<float2> uLUT : register(u0);

// Compute Van der Corput radical inverse
// See: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float RadicalInverse_VdC(uint bits)
{
   bits = (bits << 16u) | (bits >> 16u);
   bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
   bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
   bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
   bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
   return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Sample i-th point from Hammersley point set of NumSamples points total.
float2 SampleHammersley(uint i)
{
   return float2(i * kInvSampleCount, RadicalInverse_VdC(i));
}

// Importance sample GGX normal distribution function for a fixed roughness value.
// This returns normalized half-vector between Li & Lo.
// For derivation see: http://blog.tobias-franke.eu/2014/03/30/notes_on_importance_sampling.html
// PBRT 13.3 has very detailed explanation about the inversion method
float3 ImportanceSampleGGX(float u1, float u2, float roughness)
{
   float alpha = roughness;

   float cosTheta = sqrt((1.0 - u2) / (1.0 + (alpha * alpha - 1.0) * u2));
   float sinTheta = sqrt(1.0 - cosTheta * cosTheta); // Trig. identity
   float phi = M_2PI * u1;

	// Convert to Cartesian upon return.
   return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float3 TangentToWorld(float3 v, float3 N, float3 T, float3 B)
{
   float3x3 tbn = transpose(float3x3(T, B, N));
   return normalize(mul(tbn, v));
}

// Single term for separable Schlick-GGX below.
float SchlickG1(float cosTheta, float k)
{
   return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method (IBL version).
float SchlickGGX_IBL(float cosLi, float cosLo, float roughness)
{
   float r = roughness;
   float k = (r * r) / 2.0; // Epic suggests using this roughness remapping for IBL lighting.
   return SchlickG1(cosLi, k) * SchlickG1(cosLo, k);
}


[RootSignature(RootSig)]
[numthreads(32, 32, 1)]
void main( uint3 threadId : SV_DispatchThreadID )
{

   float2 outSize;
   uLUT.GetDimensions(outSize.x, outSize.y);

   float NdV = float(threadId.x) / outSize.x;
   float roughness = float(threadId.y) / outSize.y;

   NdV = max(NdV, Eps);
   
   // assuming view angle is along the positive Z.
   float3 V;
   V.x = sqrt(1 - NdV * NdV);
   V.y = 0;
   V.z = NdV;

   float A = 0, B = 0;

   for (uint i = 0; i < kSampleCount; ++i)
   {
      float2 u = SampleHammersley(i);
      float3 H = ImportanceSampleGGX(u.x, u.y, roughness);

      float3 L = reflect(-V, H);

      float NdL = saturate(L.z);
      float NdH = max(H.z, Eps);
      float VdH = saturate(dot(V, H));

      if (NdL > 0)
      {
         float G = SchlickGGX_IBL(NdV, NdL, roughness);

         float GVis = G * VdH / (NdH * NdV);
         float Fc = pow(1 - VdH, 5);
         
         A += (1 - Fc) * GVis;
         B += Fc * GVis;
      }
   }


   uLUT[threadId.xy] = float2(A, B) * kInvSampleCount;

}