#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(CBV(b0, space = 0, numDescriptors = 2), " \
                   "SRV(t0, space = 0, numDescriptors = 3, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
   "DescriptorTable(Sampler(s1, space = 0, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL)," \
   "DescriptorTable(SRV(t0, space = 1, numDescriptors = 4, flags = DATA_VOLATILE)," \
                   "CBV(b0, space = 1, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL)," \
   "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_PIXEL)" 

struct VSInput
{
   float3 position : POSITION;
   float2 uv : UV;
   float4 color : COLOR;
   float3 normal : NORMAL;
   float4 tangent : TANGENT;
};

struct PSInput
{
   float4 pos : SV_POSITION;
   float3 world : WORLD_POS;
   float3 norm : NORMAL;
   float2 uv : UV;
};


cbuffer cCamera : register(b0)
{
   float4x4 view;
   float4x4 proj;
   float4x4 invView;
   float4x4 invProj;
};

struct light_t
{
   float4 position;
   float4 intensity;
   float4 albedo;
   float4 mat;
};

cbuffer cLight : register(b1)
{
   light_t light;
}

Texture2D<float4> gTex : register(t0);
TextureCube gEnvIrradiance : register(t1);
TextureCube gEnvSpecular : register(t2);
Texture2D<float2> gEnvSpecularLUT : register(t3);

SamplerState gSampler : register(s0);
SamplerState gSamplerClamp : register(s1);

PSInput main(VSInput input)
{
   PSInput output;
   output.world = input.position;
   output.pos = mul(mul(proj, view), float4(input.position, 1.f));
   output.uv = input.uv;
   output.norm = input.normal;
   return output;
}