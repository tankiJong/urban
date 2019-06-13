#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(CBV(b0, numDescriptors = 1), SRV(t0, numDescriptors = 1, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
   "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_PIXEL)," 


inline float4 FullScreenTriangle(in uint vertexID)
{
   float4 pos;
   pos.x = (float) (vertexID / 2) * 4.0f - 1.0f;
   pos.y = (float) (vertexID % 2) * 4.0f - 1.0f;
   pos.z = 1.0f;
   pos.w = 1.0f;

   return pos;
}

inline float2 FullScreenUV(in uint vertexID)
{
   float2 tex;
   tex.x = (float) (vertexID / 2) * 2.0f;
   tex.y = (float) (vertexID % 2) * 2.0f;
   return tex;
}

struct SkyInput
{
   float4 position : SV_POSITION;
   float2 uv : TEXCOORD0;
};


cbuffer cCamera : register(b0)
{
   float4x4 view;
   float4x4 proj;
   float4x4 invView;
   float4x4 invProj;
};


TextureCube gSkyBox : register(t0);

SamplerState gSampler : register(s0);

[RootSignature(RootSig)]
SkyInput main_vs(uint vid : SV_VertexID)
{
   SkyInput output;
   output.position = FullScreenTriangle(vid);
   output.uv = FullScreenUV(vid);
   return output;
}

[RootSignature(RootSig)]
float4 main_ps(SkyInput input) : SV_TARGET
{

   float4 ndc = float4(input.uv * 2.f - 1.f, 0.f, 1.f);

   float4 world = mul(invView, mul(invProj, ndc));
   world /= world.w;

   float4 eye = mul(invView, float4(0.f.xxx, 1.f));
   eye /= eye.w;

   float3 dir = normalize((world - eye).xyz);

   float3 skyColor = gSkyBox.Sample(gSampler, dir).xyz;

   return float4(pow(skyColor, 1.f / 2.2f), 1.f);
}