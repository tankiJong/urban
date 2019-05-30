
struct PSInput {
   float4 pos: SV_POSITION;
   float4 color: COLOR;
};

#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(SRV(t0, numDescriptors = 1, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
   "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_PIXEL)," 

Texture2D<float4> gTex: register(t0);
SamplerState gSampler : register(s0);

[RootSignature(RootSig)]
float4 main(PSInput input) : SV_TARGET
{
	return gTex.Sample(gSampler, input.color.xy);
}