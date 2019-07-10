#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(CBV(b0, numDescriptors = 1), SRV(t0, numDescriptors = 1), UAV(u0, numDescriptors = 1, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
   "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_ALL)," 

static const float M_PI = 3.1415926535f;
static const float M_2PI = 2 * M_PI;
static const float Eps = 0.00001;

static const uint kSampleCount = 1024;
static const float kInvSampleCount = 1.f / float(kSampleCount);

cbuffer SpecularFilterConfig : register(b0)
{
   float roughness;
}

TextureCube tSkyBox : register(t0);
RWTexture2DArray<float4> uSpecular : register(u0);

SamplerState sBilinear : register(s0);

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

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float ndfGGX(float NdH, float roughness)
{
   float alpha = roughness;
   float alphaSq = alpha * alpha;

   float denom = (NdH * NdH) * (alphaSq - 1.0) + 1.0;
   return alphaSq / (M_PI * denom * denom);
}

float3 SampleHemisphere(float2 u)
{
   const float u1p = sqrt(max(0.0, 1.0 - u.x * u.x));
   return float3(cos(M_2PI * u.y) * u1p, sin(M_2PI * u.y) * u1p, u.x);
}

float3 TangentToWorld(float3 v, float3 N, float3 T, float3 B)
{
   float3x3 tbn = transpose(float3x3(T, B, N));
   return normalize(mul(tbn, v));
}

float3 GetSampleVector(uint3 threadId)
{
   float outputWidth, outputHeight, outputDepth;
   uSpecular.GetDimensions(outputWidth, outputHeight, outputDepth);

   float2 st = threadId.xy / float2(outputWidth, outputHeight);
   float2 uv = 2.0 * float2(st.x, 1.0 - st.y) - 1.0;

	// Select vector based on cubemap face index.
   float3 ret;
   switch (threadId.z)
   {
      case 0:
      ret = float3(1.0, uv.y, -uv.x);
      break;
      case 1:
      ret = float3(-1.0, uv.y, uv.x);
      break;
      case 2:
      ret = float3(uv.x, 1.0, -uv.y);
      break;
      case 3:
      ret = float3(uv.x, -1.0, uv.y);
      break;
      case 4:
      ret = float3(uv.x, uv.y, 1.0);
      break;
      case 5:
      ret = float3(-uv.x, uv.y, -1.0);
      break;
   }
   return normalize(ret);
}

void GenerateTBN(float3 N, out float3 T, out float3 B)
{
	// Branchless select non-degenerate T.
   T = cross(N, float3(0.0, 1.0, 0.0));
   T = lerp(cross(N, float3(1.0, 0.0, 0.0)), T, step(Eps, dot(T, T)));

   T = normalize(T);
   B = normalize(cross(N, T));
}

// Compute orthonormal basis for converting from tanget/shading space to world space.
void computeBasisVectors(const float3 N, out float3 S, out float3 T)
{
	// Branchless select non-degenerate T.
   T = cross(N, float3(0.0, 1.0, 0.0));
   T = lerp(cross(N, float3(1.0, 0.0, 0.0)), T, step(Eps, dot(T, T)));

   T = normalize(T);
   S = normalize(cross(N, T));
}

// Convert point from tangent/shading space to world space.
float3 tangentToWorld(const float3 v, const float3 N, const float3 S, const float3 T)
{
   return S * v.x + T * v.y + N * v.z;
}

[numthreads(32, 32, 1)]
void main( uint3 threadId : SV_DispatchThreadID )
{

   uint3 outSize;
   uSpecular.GetDimensions(outSize.x, outSize.y, outSize.z);

   if (any(threadId.xy >= outSize.xy)) return;

   float2 cubmapSize;
   uint numMipLevel;
   tSkyBox.GetDimensions(0, cubmapSize.x, cubmapSize.y, numMipLevel);

   float solidAngle = 4.f * M_PI / (6.f * cubmapSize.x * cubmapSize.y);

   // isotropic reflection - assume zero viewing angle 
   float3 N = GetSampleVector(threadId);
   float3 V = N;

   float3 S, T;

   computeBasisVectors(N, T, S);

   float3 color = 0;
   float weight = 0;
   for (uint i = 0; i < kSampleCount; ++i)
   {
      float2 u = SampleHammersley(i);
      float3 h = tangentToWorld(ImportanceSampleGGX(u.x, u.y, roughness), N, S, T);

      float3 L = reflect(-V, h);

      float NdL = dot(N, L);
      if (NdL > 0)
      {
         float Ndh = max(dot(N, h), 0.f);

         // GGX normal distribution function (D term) probability density function.
			// Scaling by 1/4 is due to change of density in terms of Lh to Li (and since N=V, rest of the scaling factor cancels out).
         float pdf = ndfGGX(Ndh, roughness) * .25f;

         float sampleSolidAngle = 1.f / (kSampleCount * pdf);

         float sampleMip = max(.5f * log2(sampleSolidAngle / solidAngle) + 1.f, 0.f);

         color += tSkyBox.SampleLevel(sBilinear, L, sampleMip).xyz * NdL;
         //color += (L * .5f + .5f) * NdL;
         weight += NdL;
      }
   }

   color /= weight;

   uSpecular[threadId] = float4(color, 1.f);

}