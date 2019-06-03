
struct VSInput {
   float3 position: POSITION;
   float2 uv: UV;
   float4 color: COLOR;
   float3 normal: NORMAL;
   float3 tangent: TANGENT;
   float3 bitangent: BITANGENT;
};

struct PSInput {
   float4 pos: SV_POSITION;
   float4 color: COLOR;
};

static const float4 positions[3] = {
   float4(1., .0, 1., 1.f),
   float4(.0, 1., 1., 1.f),
   float4(.0, .0, .0, 1.f),
};

static const float4 colors[3] = {
   float4(1., .0, .0, 1.f),
   float4(.0, 1., .0, 1.f),
   float4(.0, .0, 1., 1.f),
};

#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(CBV(b0, numDescriptors = 1), SRV(t0, numDescriptors = 1, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
   "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_PIXEL)," 

cbuffer cCamera: register(b0) {
   float4x4 view;
   float4x4 proj;
   float4x4 invView;
   float4x4 invProj;
};

Texture2D<float4> gTex: register(t0);
SamplerState gSampler : register(s0);

[RootSignature(RootSig)]
PSInput main( VSInput input )
{
   PSInput output;
   output.pos = mul(mul(proj, view), float4(input.position, 1.f));
   output.color = float4(input.uv, 0.f, 1.f);
	return output;
}