﻿#include "engine/pch.h"
#include "ResourceView.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

S<RenderTargetView> RenderTargetView::sNullView = nullptr;

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

RenderTargetView* RenderTargetView::NullView()
{
   if(!sNullView) {
      sNullView = S<RenderTargetView>(new RenderTargetView());
   }

   return sNullView.get();
}