#pragma once

#include "utils.hpp"
#include "Descriptor.hpp"
#include "Texture.hpp"

class Resource;

struct ViewInfo {
   
};

template<typename H>
class ResourceView {
public:
   static constexpr uint kMaxPossible = -1;
   using handle_t = H;

   virtual ~ResourceView() = default;

   const ViewInfo& GetViewInfo() const { return mViewInfo; }
   S<Resource> GetResource() const { return mResource.lock(); }
   const H& GetHandle() const { return mHandle; }
   
protected:

   ResourceView( const ViewInfo& viewInfo, const W<Resource>& resource, const H& handle )
      : mViewInfo( viewInfo )
    , mResource( resource )
    , mHandle( handle ) {}

   ViewInfo mViewInfo;
   W<Resource> mResource;
   H mHandle;
};


class RenderTargetView: public ResourceView<Descriptors> {
public:
   RenderTargetView(W<const Texture> tex, uint mipLevel = 0, uint arraySlice = 0);
   RenderTargetView();

   eTextureFormat Format() const { return mFormat; }
protected:
   eTextureFormat mFormat;
   static S<RenderTargetView> sNullView;
};

class DepthStencilView: public ResourceView<Descriptors> {
public:
   DepthStencilView(W<const Texture> tex, uint mipLevel = 0, uint arraySlice = 0);
   DepthStencilView();

   eTextureFormat Format() { return mFormat; }
protected:
   eTextureFormat mFormat;
   static S<DepthStencilView> sNullView;
};