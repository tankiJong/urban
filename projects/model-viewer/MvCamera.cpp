#include "MvCamera.hpp"
#include "engine/core/Time.hpp"
#include "engine/application/Input.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

void MvCamera::OnUpdate()
{
   float dt = float( Clock::Main().frame.second );

   float2 deltaMouse = Input::Get().GetMouseDeltaPosition();

   if(Input::Get().IsKeyDown( MOUSE_LBUTTON )) {
      mTheta -= deltaMouse.x;
      mPhi = std::clamp(mPhi + deltaMouse.y, -170.f, 170.f);
   }


   if(Input::Get().IsKeyDown( 'A' )) {
      mOrbitCenter -= mTransform.X() * dt * 10.f;
      printf( "%f, %f, %f\n", mOrbitCenter.x, mOrbitCenter.y, mOrbitCenter.z );
   }
   if(Input::Get().IsKeyDown( 'D' )) {
      mOrbitCenter += mTransform.X() * dt * 10.f;
      printf( "%f, %f, %f\n", mOrbitCenter.x, mOrbitCenter.y, mOrbitCenter.z );
   }

   if(Input::Get().IsKeyDown( 'W' )) {
      mOrbitCenter += mTransform.Z() * dt * 10.f;
      printf( "%f, %f, %f\n", mOrbitCenter.x, mOrbitCenter.y, mOrbitCenter.z );
   }
   if(Input::Get().IsKeyDown( 'S' )) {
      mOrbitCenter -= mTransform.Z() * dt * 10.f;
      printf( "%f, %f, %f\n", mOrbitCenter.x, mOrbitCenter.y, mOrbitCenter.z );
   }

   float3 position = Spherical( mRadius, mTheta, mPhi );
   LookAt( position + mOrbitCenter, mOrbitCenter );
}
