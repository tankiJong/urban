
#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(SRV(t0, numDescriptors = 1), UAV(u0, numDescriptors = 1, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
   "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_ALL)," 

static const float M_PI = 3.1415926535f;
static const float M_2_PI = 2 * M_PI;

Texture2D<float4> gEquirect : register(t0);
RWTexture2DArray<float4> uCubeMap : register(u0);

SamplerState gSampler : register(s0);

float3 GetSampleVector(uint3 ThreadID)
{
	float width, height, depth;
	uCubeMap.GetDimensions(width, height, depth);

   float2 st = ThreadID.xy/float2(width, height);
   float2 uv = 2.0 * float2(st.x, 1.0-st.y) - float2(1.0, 1.0);

	float3 result;
	switch(ThreadID.z)
	{
	   case 0: result = float3(  1.0,   uv.y, -uv.x ); break;
	   case 1: result = float3( -1.0,   uv.y,  uv.x ); break;
	   case 2: result = float3(  uv.x,  1.0,  -uv.y ); break;
	   case 3: result = float3(  uv.x, -1.0,   uv.y ); break;
	   case 4: result = float3(  uv.x,  uv.y,  1.0  ); break;
	   case 5: result = float3( -uv.x,  uv.y, -1.0  ); break;
	}
   return normalize(result);
}

[numthreads(32, 32, 1)]
void main(uint3 ThreadID : SV_DispatchThreadID)
{
   float3 v = GetSampleVector(ThreadID);
	
   // Convert Cartesian direction vector to spherical coordinates.
   float phi   = atan2(v.z, v.x);
   float theta = acos(v.y);

   // Sample equirectangular texture.
   float4 color = gEquirect.SampleLevel(gSampler, float2(phi/M_2_PI, theta/M_PI), 0);

   // Write out color to output cubemap.
   uCubeMap[ThreadID] = color;
}