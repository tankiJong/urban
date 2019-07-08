#ifndef __INCLUDE_SHADING_MODEL__
#define __INCLUDE_SHADING_MODEL__

#include "brdf.hlsli"

float3 StandardShading(float3 albedo, float3 specular, float roughness, float metallic, float3 L, float3 V, float3 N)
{
   float NdL = dot(N, L);
   float NdV = dot(N, V);
   float LdV = dot(L, V);
   float invLenH = rsqrt(2 + 2 * LdV);
   float NdH = saturate((NdL + NdV) * invLenH);
   float VdH = saturate(invLenH + invLenH * invLenH);
   NdL = saturate(NdL);
   NdV = saturate(abs(NdV) + 1e-5);

   // Generalized microfacet specular
   float D = D_GGX(roughness, NdH);
   float Vis = Vis_SmithJointApprox(roughness, NdV, NdL);
   float3 F = F_Schlick(specular, VdH);

   float3 Diffuse = Diffuse_Lambert(albedo);
   return Diffuse + (D * Vis) * F / (0.0001f + 4 * NdL * NdV).xxx;
}

#endif