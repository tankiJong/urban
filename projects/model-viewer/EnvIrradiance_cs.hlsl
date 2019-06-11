#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(SRV(t0, numDescriptors = 1), UAV(u0, numDescriptors = 1, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
   "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_ALL)," 

static const float M_PI = 3.1415926535f;
static const float M_2PI = 2 * M_PI;
static const float Eps = 0.00001;

static const uint kSampleCount = 64 * 1024;
static const float kInvSampleCount = 1.f / float(kSampleCount);

TextureCube tSkyBox : register(t0);
RWTexture2DArray<float4> uEnvMap : register(u0);

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
   uEnvMap.GetDimensions(outputWidth, outputHeight, outputDepth);

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

[RootSignature(RootSig)]
[numthreads(32, 32, 1)]
void main( uint3 threadId : SV_DispatchThreadID )
{

   float3 N = GetSampleVector(threadId);

   float3 T, B;

   GenerateTBN(N, T, B);

   float3 irradiance = 0.f.xxx;
   for (uint i = 0; i < kSampleCount; i++)
   {
      float2 u = SampleHammersley(i);
      float3 local = SampleHemisphere(u);
      float3 world = TangentToWorld(local, N, T, B);
      float WdN = max(0.f, dot(world, N));

      // irradiance = C * 2 * PI / PDF
      // PDF = PI
      irradiance += tSkyBox.SampleLevel(sBilinear, world, 0).xyz * WdN * 2 /* * PI / PI */;
   }

   irradiance *= kInvSampleCount;

   uEnvMap[threadId] = float4(irradiance, 1.f);
}