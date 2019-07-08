#include "engine/pch.h"
#include "engine/graphics/program/BindingLayout.hpp"
#include "engine/graphics/program/ResourceBinding.hpp"
#include "engine/platform/win.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/graphics/CommandList.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function /////////////////////
////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////


void ResourceBinding::Flattened::FinalizeStaticResources()
{
   if(mInitialized) return;

   auto tables = mOwner->mLayout.Data();
   size_t tableCount = tables.size();
   mBindLocations.resize( tableCount );

   BindingItem* item = mBindingItems.data();

   for(uint i = 0; i < tableCount; i++) {
      if(tables[i].ElementCount() == 0) continue;
      if(tables[i].isStatic) {
         GpuDescriptorHeap* descriptorHeap = nullptr;
         if(tables[i][0].Type() != eDescriptorType::Sampler) {
            descriptorHeap = Device::Get().GetGpuDescriptorHeap( eDescriptorType::Srv );
         } else {
            descriptorHeap = Device::Get().GetGpuDescriptorHeap( eDescriptorType::Sampler );
         }
         mBindLocations[i] = descriptorHeap->Allocate( tables[i].ElementCount() );
         Descriptors& descriptors = mBindLocations[i];

         ASSERT_DIE( tables[i].ElementCount() == item->nextTableOffset );
         for(uint k = 0; k < tables[i].ElementCount(); k++) {
            auto cpuHandle = descriptors.GetCpuHandle( k );
            Device::Get().NativeDevice()->CopyDescriptorsSimple( 
               1, 
               cpuHandle, 
               item->location, ToD3d12HeapType( item->type ) );
            item++;
         }
      } else {
         item += item->nextTableOffset;
#ifdef _DEBUG
         if(item->rangeIndex != 0) {
            while(++i < tableCount) {
               ASSERT_DIE( tables[i].ElementCount() == 0 );
            }
         } else {
            ASSERT_DIE( item->rangeIndex == 0 || i + 1 == tableCount);
         }
#endif
      }
   }

   mInitialized = true;
}

void ResourceBinding::Flattened::BindFor( CommandList& commandList, uint startRootIndex, bool forCompute )
{
   auto   tables     = mOwner->mLayout.Data();
   size_t tableCount = tables.size();

   BindingItem* item = mBindingItems.data();

   for(uint i = 0; i < tableCount; i++) {
      if(tables[i].ElementCount() == 0) continue;
      if(!tables[i].isStatic) {
         DescriptorPool* descriptorPool = nullptr;
         if(item->type == eDescriptorType::Sampler) { descriptorPool = commandList.GpuSamplerDescriptorPool(); } else {
            descriptorPool = commandList.GpuViewDescriptorPool();
         }
         mBindLocations[i]        = descriptorPool->Allocate( tables[i].ElementCount(), false );
         Descriptors& descriptors = mBindLocations[i];

         ASSERT_DIE( tables[i].ElementCount() == item->nextTableOffset );
         for(uint k = 0; k < tables[i].ElementCount(); k++) {
            Device::Get().NativeDevice()
               ->CopyDescriptorsSimple(1, descriptors.GetCpuHandle( k ),
                                       item->location, ToD3d12HeapType( item->type ) );
            item++;
         }
      } else {
         item += item->nextTableOffset;
#ifdef _DEBUG
         if(item->rangeIndex != 0) {
            while(++i < tableCount) {
               ASSERT_DIE( tables[i].ElementCount() == 0 );
            }
         } else {
            ASSERT_DIE( item->rangeIndex == 0 || i + 1 == tableCount);
         }
#endif
      }
   }

   if(forCompute) {
      for(size_t i = 0, j = startRootIndex; i < mBindLocations.size(); i++) {
         if(!mBindLocations[i].Valid()) continue;
         commandList.Handle()->SetComputeRootDescriptorTable( j++, mBindLocations[i].GetGpuHandle( 0 ) );
      }
   } else {
      for(size_t i = 0, j = startRootIndex; i < mBindLocations.size(); i++) {
         if(!mBindLocations[i].Valid()) continue;

         auto handle =  mBindLocations[i].GetGpuHandle( 0 );
         commandList.Handle()->SetGraphicsRootDescriptorTable( j++, handle );
      }
   }
}
