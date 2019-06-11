

#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(SRV(t0, numDescriptors = 1), CBV(b0, numDescriptors = 1), UAV(u0, numDescriptors = 1, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
	"DescriptorTable(Sampler(s1, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL)," \
   "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_ALL)," 

cbuffer constriants : register(b0)
{
   float2 cOffset;      // offset in the dest buffer, in percent [0, 1]
   float2 cScale;       // scale of the source texture
   float  gamma;        // whether output texture is sRGB texture
};


Texture2D tInput: register(t0);
RWTexture2D<float4> uOutput: register(u0);

SamplerState sLinear: register(s1);


float4 SampleRange(float2 uvmins, float2 uvmaxs, float2 dim)
{
   float2 deltaUv = uvmaxs - uvmins;
   float2 size = ceil(deltaUv * dim);
   
   float2 duv = deltaUv / size;

   float4 color;
   float total = 0;
   for (float u = uvmins.x; u < uvmaxs.x; u += duv.x)
   {
      if(u >= 1) break;
      for (float v = uvmins.y; v < uvmaxs.y; v += duv.y)
      {
         if(v>=1) break;
         color += tInput.SampleLevel(sLinear, float2(u, v), 0);
         total++;
      }
   }
   return color / total;
}

[RootSignature(RootSig)]
[numthreads(16, 16, 1)]
void main( uint3 threadId : SV_DispatchThreadID )
{
   float2 dim;
   tInput.GetDimensions(dim.x, dim.y);

   float2 outputSize;
   uOutput.GetDimensions(outputSize.x, outputSize.y);
   float2 positionInSource = float2(threadId.xy) - cOffset * outputSize;
   if (any(positionInSource < 0)) return;

   float2 uv = positionInSource / (dim * cScale);
   if (any(uv > 1)) return;

   float4 color = tInput.SampleLevel(sLinear, uv, 0);

   uOutput[threadId.xy] = float4(pow(color.xyz, gamma ? (1.f / 2.2f) : 1.f), color.w);
}