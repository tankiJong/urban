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

   if(Input::Get().IsKeyDown( MOUSE_RBUTTON )) {
      mTheta -= deltaMouse.x;
      mPhi = std::clamp(mPhi + deltaMouse.y, -170.f, 170.f);
   }

   if(Input::Get().IsKeyDown( 'A' )) {
      mOrbitCenter -= mTransform.X() * dt * 10.f;
   }
   if(Input::Get().IsKeyDown( 'D' )) {
      mOrbitCenter += mTransform.X() * dt * 10.f;
   }

   if(Input::Get().IsKeyDown( 'W' )) {
      mOrbitCenter += mTransform.Z() * dt * 10.f;
   }
   if(Input::Get().IsKeyDown( 'S' )) {
      mOrbitCenter -= mTransform.Z() * dt * 10.f;
   }

   if(Input::Get().IsKeyDown( 'Q' )) {
      mRadius += dt * 10.f;
   }
   if(Input::Get().IsKeyDown( 'E' )) {
      mRadius = std::clamp(mRadius - dt * 10.f, 1.f, mRadius);
   }
   float3 position = Spherical( mRadius, mTheta, mPhi );
   LookAt( position + mOrbitCenter, mOrbitCenter );
}
