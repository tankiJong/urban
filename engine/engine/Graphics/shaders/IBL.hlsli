#ifndef __INCLUDE_IBL__
#define __INCLUDE_IBL__

#include "brdf.hlsli"

float3 ComputeIBL(float3 surfaceAlbedo, float3 surfaceSpecular,
                 float3 envAlbedo, float3 filteredEnvSpecular,
                 float2 envBrdf, float metallic, float VdH 
                 ) 
{
   float3 F = F_Schlick(surfaceSpecular, VdH);
   float3 kd = lerp(1.f.xxx - F, 0.f.xxx, metallic);

   float3 specularIBL = filteredEnvSpecular * (surfaceSpecular * envBrdf.x + envBrdf.y);

   float3 ibl = envAlbedo * surfaceAlbedo * kd + specularIBL;

   return ibl;
}



#endif