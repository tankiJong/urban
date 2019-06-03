#include "engine/pch.h"
#include "ResourceView.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

S<RenderTargetView> RenderTargetView::sNullView = nullptr;
S<ShaderResourceView> ShaderResourceView::sNullView = nullptr;
S<ConstantBufferView> ConstantBufferView::sNullView = nullptr;

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

RenderTargetView::RenderTargetView()
   : RenderTargetView( W<const Texture>() ) {}

RenderTargetView* RenderTargetView::NullView()
{
   if(!sNullView) {
      sNullView = S<RenderTargetView>(new RenderTargetView());
   }

   return sNullView.get();
}

DepthStencilView::DepthStencilView()
   : DepthStencilView( W<const Texture>() ){}

ShaderResourceView::ShaderResourceView()
   : ShaderResourceView( W<const Texture>() ) {}

ShaderResourceView* ShaderResourceView::NullView()
{
   if(!sNullView) {
      sNullView = S<ShaderResourceView>( new ShaderResourceView() );
   }

   return sNullView.get();
}

ConstantBufferView::ConstantBufferView(): ConstantBufferView( W<const Buffer>() ) {}

ConstantBufferView* ConstantBufferView::NullView()
{
   if(!sNullView) {
      sNullView = S<ConstantBufferView>( new ConstantBufferView() );
   }

   return sNullView.get();
}
