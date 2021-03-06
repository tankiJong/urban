﻿#pragma once
#include "BindingLayout.hpp"
#include "engine/graphics/ResourceView.hpp"
#include "engine/graphics/ConstantBuffer.hpp"

class CommandList;
class Sampler;
class Program;

class ResourceBinding {
public:
   struct BindingItem {
      descriptor_cpu_handle_t location;
      eDescriptorType         type;
      uint                    registerIndex;
      uint                    registerSpace;

      // root index offset from the first one
      uint rootIndexOffset;
      // which range inside the table
      uint rangeIndex;

      // In an array, items[ current + items[current].nextRangeOffset ] will directly jump to the next range.
      // In case this value is 1, that means next item is the head of a range.
      uint nextTableOffset;

      // is invalid when the next item is the next table
      uint nextRangeOffset;
   };

   
   // For static resources, the descriptors are allocated upon creation of `Flattened` internally
   // `BindFor` tries to finalize for command list, allocate dynamic resources descriptors
   // The class do not 
   // be aware, this structure mea
   class Flattened {
   public:
      Flattened( const ResourceBinding& owner ): mOwner( &owner ) {}
      BindingItem& FindBindingItem( eDescriptorType type, uint registerIndex, uint registerSpace );

      void Reset()
      {
         mBindingItems.clear();
         mBindLocations.clear();
      }

      void Append( const BindingItem& b ) { mBindingItems.push_back( b ); }

      void BindFor( CommandList& commandList, uint startRootIndex, bool forCompute );
      void FinalizeStaticResources();

   protected:
      std::vector<BindingItem> mBindingItems;
      std::vector<Descriptors> mBindLocations;
      const ResourceBinding*   mOwner       = nullptr;
      bool                     mInitialized = false;
   };

   ResourceBinding() = default;
   ~ResourceBinding();

   ResourceBinding( const BindingLayout& layout ) { RegenerateFlattened( layout ); }
   ResourceBinding( BindingLayout&& layout ) { RegenerateFlattened( std::move( layout ) ); }

   void RegenerateFlattened( const BindingLayout& bindingLayout );
   void RegenerateFlattened( BindingLayout&& bindingLayout );
   void SetSrv( const ShaderResourceView* srv, uint registerIndex, uint registerSpace = 0 );
   void SetCbv( const ConstantBufferView* cbv, uint registerIndex, uint registerSpace = 0 );
   void SetUav( const UnorderedAccessView* uav, uint registerIndex, uint registerSpace = 0 );
   void SetSampler( const Sampler* sampler, uint registerIndex, uint registerSpace = 0 );

   void BindFor( CommandList& commandList, uint startRootIndex, bool forCompute ); ;
   uint RequiredTableCount();
protected:

   void          RegenerateFlattened();
   BindingLayout mLayout;
   Flattened     mFlattenedBindings{ *this };
};

class RayTracingBinding {
public:
   enum class eRecordType {
      Shader,
      HitGroup,
   };

   struct Record {
      eRecordType   type;
      uint          identifier;
      BindingLayout layout;
   };

   void SetGlobalSrv( const ShaderResourceView* srv, uint registerIndex, uint registerSpace = 0 );
   void SetGlobalCbv( const ConstantBufferView* cbv, uint registerIndex, uint registerSpace = 0 );
   void SetGlobalUav( const UnorderedAccessView* uav, uint registerIndex, uint registerSpace = 0 );
   void SetGlobalSampler( const Sampler* sampler, uint registerIndex, uint registerSpace = 0 );

   void SetLocalSrv( std::string_view recordName, const ShaderResourceView* srv, uint registerIndex, uint registerSpace = 0 );
   void SetLocalCbv( std::string_view recordName, const ConstantBufferView* cbv, uint registerIndex, uint registerSpace = 0 );
   void SetLocalUav( std::string_view recordName, const UnorderedAccessView* uav, uint registerIndex, uint registerSpace = 0 );
   void SetLocalSampler( std::string_view recordName, const Sampler* sampler, uint registerIndex, uint registerSpace = 0 );

   void BindFor( CommandList& list, int startRootIndex );
protected:
   static constexpr size_t kShaderRecordStride = 4 KB; // bytes

   bool                mIsDirty    = true;
   S<Buffer>           mGpuStorage = nullptr;
   Blob                mCpuCache;
   std::vector<Record> mShaderTable;
};
