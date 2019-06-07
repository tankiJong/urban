#include "pass.hlsli"

[RootSignature(RootSig)]
PSInput main( VSInput input )
{
   PSInput output;
   output.world = input.position;
   output.pos = mul(mul(proj, view), float4(input.position, 1.f));
   output.uv = input.uv;
   output.norm = input.normal;
	return output;
}