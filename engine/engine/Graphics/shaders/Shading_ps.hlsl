#include "ShadingModels.hlsli"
#include "IBL.hlsli"                                                                                

#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(CBV(b0, space = 0, numDescriptors = 2), " \
                   "SRV(t0, space = 0, numDescriptors = 3, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
   "DescriptorTable(Sampler(s1, space = 0, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL)," \
   "DescriptorTable(SRV(t0, space = 1, numDescriptors = 4, flags = DATA_VOLATILE)," \
                   "CBV(b0, space = 1, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL)," \
   "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_PIXEL)" 

struct PSInput
{
   float4 pos : SV_POSITION;
   float3 world : WORLD_POS;
   float3 norm : NORMAL;
   float3 tangent : TANGENT;
   float2 uv : UV;
};


cbuffer cCamera : register(b0, space0)
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

cbuffer cLight : register(b1, space0)
{
   light_t light;
}

SamplerState gSampler : register(s0, space0);
SamplerState gSamplerClamp : register(s11, space0);

TextureCube gEnvIrradiance : register(t0, space0);
TextureCube gEnvSpecular : register(t1, space0);
Texture2D<float2> gEnvSpecularLUT : register(t2, space0);

Texture2D<float4> gNormal    : register(t0, space3);
Texture2D<float4> gAlbedo    : register(t1, space3);
Texture2D<float4> gRoughness : register(t2, space3);
Texture2D<float4> gMetallic  : register(t3, space3);

cbuffer cMaterial : register(b0, space3)
{
   float4 constAlbedo;
   float4 constRoughness;
   float4 constMetallic;
}

float LightFalloff(float3 lightPosition, float3 shadingPoisition, float lightRadius)
{
   float dist = distance(lightPosition, shadingPoisition);
   return pow(saturate(1 - pow(dist / lightRadius, 4)), 2) / (dist * dist + 1);
   // return 1.f / (dist * dist);
}

float4 main(PSInput input): SV_Target
{

   float3 albedo = constAlbedo.xyz;
#ifdef FIXED_ALBEDO
#else
   albedo = gAlbedo.Sample(gSampler, input.uv).xyz;
#endif
   float roughness = constRoughness.x;
#ifdef FIXED_ROUGHNESS
#else
   roughness = gRoughness.Sample(gSampler, input.uv).g;
#endif
   
   roughness = max(roughness, 0.089f);
   float roughness2 = roughness * roughness;

   float metallic = constMetallic.x;
#ifdef FIXED_METALLIC
#else
   metallic = gMetallic.Sample(gSampler, input.uv).b;
#endif

#ifdef NON_NORMAL_MAP
   float3 N = input.norm;
#else
   float3 localNormal = gNormal.Sample(gSampler, input.uv).xyz;
   float3 N = TBNToWorld(localNormal, input.norm, input.tangent);
#endif



   const float3 sFdielectric = 0.04f.xxx;
   float3 specular = lerp(sFdielectric, albedo, metallic);

   float3 L = normalize( light.position.xyz - input.world );
   float3 V = normalize(mul(invView, float4(0.f.xxx, 1)).xyz - input.world);
   float3 H = normalize(L + V);

   float VdH = saturate(dot(V, H));


   float3 surfaceIrradiance 
      = StandardShading(albedo, specular, roughness, metallic, L, V, N);

   float3 surfaceColor = surfaceIrradiance
                       * (light.intensity.xyz * light.intensity.w)
                       * saturate(dot(N, L))
                       * LightFalloff(light.position.xyz, input.world, 30.f);

   // IBL 
   float specularLevels, _;
   gEnvSpecular.GetDimensions(0, _, _, specularLevels);

   float3 envAlbedo = gEnvIrradiance.SampleLevel(gSampler, N, 0).xyz;

   float3 refl = reflect(-V, N);
   float3 reflddx = ddx(refl);
   float3 reflddy = ddy(refl);
   float delta_max_sqr = max(dot(reflddx, reflddx), dot(reflddy, reflddy));
   float mip = 0.5 * log2(delta_max_sqr) * ( 1 - roughness2 ) * specularLevels;
   float specLevel = roughness2 * specularLevels + mip;

   float3 envSpecular = gEnvSpecular.SampleLevel(gSampler,
                                                 reflect(-V, N),
                                                 specLevel).xyz;

   float2 envBRDF = gEnvSpecularLUT.Sample(gSamplerClamp, float2(saturate(dot(N, V)), roughness2));
                
   float3 envRadiance = ComputeIBL(albedo, specular, 
                                   envAlbedo, envSpecular,
                                   envBRDF, metallic, VdH);

   float3 finalColor = surfaceColor + envRadiance;

   // Toon mapping
   const float gamma = 2.2;
   const float exposure = 1.0;
   const float pureWhite = 1.0;

   float luminance = dot(finalColor, float3(0.2126, 0.7152, 0.0722));
   float mappedLuminance = (luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
   float3 mappedColor = (mappedLuminance / luminance) * finalColor;

   // qreturn float4(pow(envAlbedo, 1 / 2.2), 1.f);
   // return float4(N * .5f + .5f, 1.f);
   // Gamma correction.
   // In my case, the render target share the exact format as the texture, so I have to do gamma correction here
   // Optionally, it's possible to create a rtv with desired sRGB format so that the API will do it for you
   return float4(pow(mappedColor, 1.f / gamma), 1.0);
}