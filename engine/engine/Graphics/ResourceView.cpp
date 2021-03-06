﻿#include "engine/pch.h"
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
S<DepthStencilView> DepthStencilView::sNullView = nullptr;
S<UnorderedAccessView> UnorderedAccessView::sNullView = nullptr;

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

DepthStencilView* DepthStencilView::NullView()
{
   if(!sNullView) {
      sNullView = S<DepthStencilView>(new DepthStencilView());
   }
   return sNullView.get();
}

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

UnorderedAccessView::UnorderedAccessView(): UnorderedAccessView( W<const Texture>() ) {}

UnorderedAccessView* UnorderedAccessView::NullView()
{
   if(!sNullView) {
      sNullView = S<UnorderedAccessView>( new UnorderedAccessView() );
   }

   return sNullView.get();
}
