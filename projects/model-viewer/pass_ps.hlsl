#define RootSig \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
	"DescriptorTable(CBV(b0, numDescriptors = 2), SRV(t0, numDescriptors = 4, flags = DATA_VOLATILE), visibility = SHADER_VISIBILITY_ALL)," \
   "DescriptorTable(Sampler(s1, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL)," \
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

struct light_t
{
   float4 position;
   float4 intensity;
   float4 albedo;
   float4 mat;
};

cbuffer cLight: register(b1) {
   light_t light;
}

Texture2D<float4> gTex: register(t0);
TextureCube gEnvIrradiance : register(t1);
TextureCube gEnvSpecular : register(t2);
Texture2D<float2> gEnvSpecularLUT : register(t3);

SamplerState gSampler : register(s0);
SamplerState gSamplerClamp : register(s1);

#define M_PI 3.1415926535f


// ue4 pbr

float3 DiffuseBRDF(float3 color/*, float3 l, float3 v*/) 
{
   return color / M_PI.xxx;
}

float D(float3 n, float3 h, float roughness) 
{
   // roughness = (roughness + 1.f) * .5f;
   float alpha = roughness * roughness;
   
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

   // https://docs.microsoft.com/en-us/windows/desktop/direct3ddxgi/converting-data-color-space
   // be aware that in d3d12 in case the SRV format is SRGB format, the driver will load in the pixel in linear space and do the gamma 2.2 correction 
   // float3 albedo = gTex.Sample(gSampler, input.uv).xyz;
   float3 albedo = light.albedo;

   // return float4(pow(albedo, 1.f/gamma), 1.f);
   // albedo *= float3(1.f, .03f, 0.f);
   float roughness = max(light.mat.x, 0.089f);
   float roughness2 = roughness * roughness;
   // roughness = roughness * roughness;
   float metalic = light.mat.y;
   //albedo = pow(albedo, gamma);
   float3 l = normalize(light.position.xyz - input.world);
   float3 v = normalize(mul(invView, float4(0.f.xxx, 1)).xyz - input.world);

   float3 F0 = lerp(Fdielectric, albedo, metalic);

   float3 h = normalize(l + v);
   float3 kd = lerp(float3(1, 1, 1) - F(F0, v, h), float3(0, 0, 0), metalic);
   //float3 kd = 1.f.xxx;
   float3 diffuse = kd * DiffuseBRDF(albedo);

   float3 specular = SpecularBRDF(F0, l, v, input.norm, roughness);

   float3 color = (specular + diffuse) * light.intensity.xyz * light.intensity.w * saturate(dot(input.norm, l))
                 * LightFalloff(light.position.xyz, input.world, 30.f);

   float3 sky = gEnvIrradiance.SampleLevel(gSampler, input.norm, 0).xyz;

   float3 kd1 = lerp(float3(1, 1, 1) - F(F0, v, input.norm), float3(0, 0, 0), metalic);

   uint specularLevels, _;
   gEnvSpecular.GetDimensions(0, _, _, specularLevels);

   float3 prefilteredSpecular = gEnvSpecular.SampleLevel(gSampler, reflect(-v, input.norm), roughness2 * specularLevels).xyz;
   float2 envBRDF = gEnvSpecularLUT.Sample(gSamplerClamp, float2(saturate(dot(input.norm, v)), roughness2));

   float3 specularIBL = prefilteredSpecular * (F0 * envBRDF.x + envBRDF.y);

   float3 ibl = sky * albedo * kd1 + specularIBL;
   color += ibl;

   // color = (specular );
	// Reinhard tonemapping operator.
	// see: "Photographic Tone Reproduction for Digital Images", eq. 4
	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance/(pureWhite*pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	float3 mappedColor = (mappedLuminance / luminance) * color;
	

   // return float4(pow(ibl, 1.f / gamma), 1.0);

   // Gamma correction.
   // In my case, the render target share the exact format as the texture, so I have to do gamma correction here
   // Optionally, it's possible to create a rtv with desired sRGB format so that the API will do it for you
   return float4(pow(mappedColor, 1.f / gamma), 1.0);
}