#pragma once

#include "utils.hpp"
#include "Descriptor.hpp"

class Resource;
class Texture;

struct ViewInfo {
   uint depthOrArraySize;
   uint firstArraySlice;
   uint mostDetailedMip;
   uint mipCount;
   eDescriptorType type;

   bool operator==(const ViewInfo& rhs) const
   {
      return
         depthOrArraySize == rhs.depthOrArraySize &&
         firstArraySlice == rhs.firstArraySlice &&
         mostDetailedMip == rhs.mostDetailedMip &&
         mipCount == rhs.mipCount &&
         type == rhs.type;
   }
};

namespace std {
  template<>
  struct hash<ViewInfo> {
    size_t operator()(const ViewInfo& info) const noexcept {
      size_t 
      result = info.mostDetailedMip;

      result ^= info.mipCount;
      result <<= 1;

      result ^= info.firstArraySlice;
      result <<= 1;

      result ^= info.depthOrArraySize;
      result <<= 1;

      result ^= uint(info.type);

      return  result;
    }
  };
}

template<typename H>
class ResourceView {
public:
   static constexpr uint kMaxPossible = -1;
   using handle_t = H;

   virtual ~ResourceView() = default;

   const ViewInfo& GetViewInfo() const { return mViewInfo; }
   S<const Resource> GetResource() const { return mResource.lock(); }
   const H& GetHandle() const { return mHandle; }

   ResourceView(W<const Resource> res, uint depthOrArraySize, uint firstArraySlice, uint mostDetailedMip, uint mipCount, eDescriptorType type)
      : mViewInfo{ depthOrArraySize, firstArraySlice, mostDetailedMip, mipCount, type }
      , mResource( res ) {}

protected:

   ViewInfo mViewInfo;
   W<const Resource> mResource;
   H mHandle = H{};
};


class RenderTargetView: public ResourceView<S<Descriptors>> {
public:
   RenderTargetView(W<const Texture> tex, uint mipLevel = 0, uint firstArraySlice = 0, uint arraySize = kMaxPossible);
   RenderTargetView();


   eTextureFormat Format() const { return mFormat; }

   static RenderTargetView* NullView();
protected:
   eTextureFormat mFormat = eTextureFormat::Unknown;
   static S<RenderTargetView> sNullView;
};

class DepthStencilView: public ResourceView<S<Descriptors>> {
public:
   DepthStencilView(W<const Texture> tex, uint mipLevel = 0, uint arraySlice = 0);
   DepthStencilView();

   eTextureFormat Format() { return mFormat; }
protected:
   eTextureFormat mFormat;
   static S<DepthStencilView> sNullView;
};
