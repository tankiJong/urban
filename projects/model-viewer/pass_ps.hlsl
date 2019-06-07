#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(CBV(b0, numDescriptors = 2), SRV(t0, numDescriptors = 1, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
   "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_PIXEL)," 

struct VSInput {
   float3 position: POSITION;
   float2 uv: UV;
   float4 color: COLOR;
   float3 normal: NORMAL;
   float4 tangent: TANGENT;
};

struct PSInput {
   float4 pos: SV_POSITION;
   float3 world: WORLD_POS;
   float3 norm: NORMAL;
   float2 uv: UV;
};


cbuffer cCamera: register(b0) {
   float4x4 view;
   float4x4 proj;
   float4x4 invView;
   float4x4 invProj;
};

struct light_t {
   float4 position;
   float4 intensity;
};

cbuffer cLight: register(b1) {
   light_t light;
}

Texture2D<float4> gTex: register(t0);
SamplerState gSampler : register(s0);

#define M_PI 3.1415926535f


// ue4 pbr

float3 DiffuseBRDF(float3 color/*, float3 l, float3 v*/) 
{
   return color / M_PI.xxx;
}

float D(float3 n, float3 h, float roughness) 
{
   // roughness = (roughness + 1.f) * .5f;
   float alpha = roughness * roughness + 0.001f;
   
   float nh = dot(n, h);
   float k = nh * nh * (alpha * alpha - 1) + 1;
   return ( alpha * alpha ) /
          ( M_PI * k * k );

}

float G1(float3 n, float3 d, float roughness) 
{
   float k = (roughness + 1) * (roughness + 1) / 8;

   float nd = saturate(dot(n, d));
   return nd / (nd*(1 - k) + k);
}

float G(float3 l, float3 v, float3 n, float roughness) 
{
   return G1(n, l, roughness) * G1(n, v, roughness);
}

float3 F(float3 F0, float3 v, float3 h) {
   float vh = saturate(dot(v, h));
   // return vh;
   return F0 + (1-F0) * pow(1-vh, 5);
}

float3 SpecularBRDF(float3 F0, float3 l, float3 v, float3 n, float roughness) 
{
   roughness = max(0.001f, roughness);
   float3 h = normalize(l + v);
   return D(n, h, roughness);
   return ( D(n, h, roughness) * G(l, v, n, roughness) * F(F0, v, h)  ) /
          (0.00001f + 4 * saturate(dot(n, l)) * saturate(dot(n, v)) ).xxx;
}

float LightFalloff(float3 lightPosition, float3 shadingPoisition, float lightRadius) {
   float dist = distance(lightPosition, shadingPoisition);
   return pow(saturate(1 - pow(dist / lightRadius, 4)), 2) / (dist * dist + 1);
   // return 1.f / (dist * dist);
}

static const float3 Fdielectric = 0.04f.xxx;
static const float gamma     = 2.2;
static const float exposure  = 1.0;
static const float pureWhite = 1.0;

[RootSignature(RootSig)]
float4 main(PSInput input) : SV_TARGET
{ 
   input.norm = normalize(input.norm);
   // return float4(input.norm * .5f + .5f, 1.f);
   // float3 texColor = gTex.Sample(gSampler, input.uv.xy).xyz;
   float metalic = 0.f;
   float roughness = 0.f;
   float3 albedo = float3(1.f, 0.f, 0.f);
   //albedo = pow(albedo, gamma);
   float3 l = normalize(light.position.xyz - input.world);
   float3 v = normalize(mul(invView, float4(0.f.xxx, 1)).xyz - input.world);

   float3 F0 = lerp(Fdielectric, albedo, metalic);

   float3 h = normalize(l + v);
   float3 kd = lerp(float3(1, 1, 1) - F(F0, v, h), float3(0, 0, 0), metalic);
   //float3 kd = 1.f.xxx;
   float3 diffuse = kd * DiffuseBRDF(albedo);

   float3 specular = SpecularBRDF(F0, l, v, input.norm, roughness);

   float3 color = (specular + diffuse) * light.intensity.xyz * 10 * saturate(dot(input.norm, l)) 
                 * LightFalloff(light.position.xyz, input.world, 100.f);
   // color = (specular );
	
	// Reinhard tonemapping operator.
	// see: "Photographic Tone Reproduction for Digital Images", eq. 4
	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance/(pureWhite*pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	float3 mappedColor = (mappedLuminance / luminance) * color;

	// Gamma correction.
	return float4(pow(mappedColor, 1.0/gamma), 1.0);
}