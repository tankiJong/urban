#ifndef __INCLUDE_BRDF__
#define __INCLUDE_BRDF__

#include "Common.hlsli"

// from Unreal: Brdf.ush

float3 Diffuse_Lambert(float3 DiffuseColor)
{
   return DiffuseColor * (1 / PI);
}

// GGX / Trowbridge-Reitz
// [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
float D_GGX(float Roughness, float NoH)
{
   float a = Roughness * Roughness;
   float a2 = a * a;
   float d = (NoH * a2 - NoH) * NoH + 1; // 2 mad
   return a2 / (PI * d * d); // 4 mul, 1 rcp
}

// Tuned to match behavior of Vis_Smith
// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
float Vis_Schlick(float Roughness, float NoV, float NoL)
{
   float k = Square(Roughness) * 0.5;
   float Vis_SchlickV = NoV * (1 - k) + k;
   float Vis_SchlickL = NoL * (1 - k) + k;
   return 0.25 / (Vis_SchlickV * Vis_SchlickL);
}


// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
float3 F_Schlick(float3 SpecularColor, float VoH)
{
   float Fc = Pow5(1 - VoH); // 1 sub, 3 mul
	//return Fc + (1 - Fc) * SpecularColor;		// 1 add, 3 mad
	
	// Anything less than 2% is physically impossible and is instead considered to be shadowing
   return saturate(50.0 * SpecularColor.g) * Fc + (1 - Fc) * SpecularColor;
	
}

float3 F_Fresnel(float3 SpecularColor, float VoH)
{
   float3 SpecularColorSqrt = sqrt(clamp(float3(0, 0, 0), float3(0.99, 0.99, 0.99), SpecularColor));
   float3 n = (1 + SpecularColorSqrt) / (1 - SpecularColorSqrt);
   float3 g = sqrt(n * n + VoH * VoH - 1);
   return 0.5 * Square((g - VoH) / (g + VoH)) * (1 + Square(((g + VoH) * VoH - 1) / ((g - VoH) * VoH + 1)));
}

// Appoximation of joint Smith term for GGX
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
float Vis_SmithJointApprox(float Roughness, float NoV, float NoL)
{
   float a = Square(Roughness);
   float Vis_SmithV = NoL * (NoV * (1 - a) + a);
   float Vis_SmithL = NoV * (NoL * (1 - a) + a);
	// Note: will generate NaNs with Roughness = 0.  MinRoughness is used to prevent this
   return 0.5 * rcp(Vis_SmithV + Vis_SmithL);
}


#endif