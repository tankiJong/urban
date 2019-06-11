

#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(SRV(t0, numDescriptors = 1), CBV(b0, numDescriptors = 1), UAV(u0, numDescriptors = 1, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
	"DescriptorTable(Sampler(s1, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL)," \
   "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_ALL)," 

cbuffer constriants : register(b0)
{
   float2 cOffset; // offset in the dest buffer, in percent [0, 1]
   float2 cScale; // scale of the source texture
   float gamma; // whether output texture is sRGB texture
};



Texture2DArray tInput : register(t0);
RWTexture2DArray<float4> uOutput : register(u0);

SamplerState sLinear : register(s1);


[RootSignature(RootSig)]
[numthreads(16, 16, 1)]
void main(uint3 threadId : SV_DispatchThreadID, uint3 groupId: SV_GroupID)
{
   float3 dim;
   tInput.GetDimensions(dim.x, dim.y, dim.z);

   float3 outputSize;
   uOutput.GetDimensions(outputSize.x, outputSize.y, outputSize.z);

   float2 positionInputStart = float2(threadId.xy) - cOffset * outputSize.xy;
   if (any(positionInputStart < 0)) return;

   float2 uv = positionInputStart / ( dim.xy * cScale );
   float4 color = tInput.SampleLevel(sLinear, float3(uv, groupId.z), 0);
   uOutput[threadId] = float4(pow(color.xyz, gamma > 0 ? (1.f / 2.2f) : 1.f), color.w);
}