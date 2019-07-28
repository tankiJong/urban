

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

TextureCube gEnvIrradiance : register(t0);
TextureCube gEnvSpecular : register(t1);
Texture2D<float2> gEnvSpecularLUT : register(t2);
RaytracingAccelerationStructure scene : register(t3);

RWTexture2D<float4> uOutput : register(u0);

SamplerState gSampler : register(s11);


struct Payload
{
   float3 color;
};

struct Attribute
{  
   float2 uv;
};
[shader("miss")]
void SimpleMiss(inout Payload pl)
{
   pl.color = float3(.3f, .3f, 1.f);
}

[shader("closesthit")]
void SimpleHit(inout Payload pl, Attribute attribs) 
{
   pl.color = float3(attribs.uv, 0.f);
}

[shader("raygeneration")]
void RayGen()
{
   uint2 dispatchIndex = DispatchRaysIndex().xy;
   uint2 dispatchDimensions = DispatchRaysDimensions().xy;

   float4 ndc = float4(dispatchIndex/dispatchDimensions * 2.f - 1.f, 0.f, 1.f);

   float4 world = mul(invView, mul(invProj, ndc));
   world /= world.w;
   float4 eye = mul(invView, float4(0.f.xxx, 1.f));
   eye /= eye.w;

   RayDesc ray = { eye.xyz, 0, normalize(world - eye).xyz, 100000.f };
   Payload pl = { 0.f.xxx };

   TraceRay(
		scene, // AccelerationStructure
		RAY_FLAG_NONE,
		0xFF, // ray tracing mask
		0, // Hit group index
		0, // MultiplierForGeometryContributionToShaderIndex
		0, // MissShaderIndex
		ray, // RayDesc
		pl // Payload
	);

   uOutput[dispatchIndex] = float4(pl.color, 1.f);

}