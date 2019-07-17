#include "engine/pch.h"
#include "engine/graphics/TopLevelAS.hpp"
#include "engine/graphics/model/Mesh.hpp"
#include "engine/graphics/CommandList.hpp"
#include "engine/graphics/Device.hpp"
#include "engine/graphics/CommandQueue.hpp"

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

void TopLevelAS::Finalize(CommandList* commandList)
{
   CommandList* list = nullptr;
   if(commandList == nullptr) {
      list = (CommandList*)_alloca( sizeof(CommandList) );
      new (list)CommandList( eQueueType::Compute );
   } else {
      list = commandList;
   }

   std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances;
   instances.reserve( mInstances.size() );

   for(uint i = 0; i < (uint)mInstances.size(); i++) {
      auto& inst = mInstances[i];
      ASSERT_DIE( inst.instanceId == i );
      D3D12_RAYTRACING_INSTANCE_DESC desc = {
         {
            { inst.transform.ix, inst.transform.jx, inst.transform.kx, inst.transform.tx },
            { inst.transform.iy, inst.transform.jy, inst.transform.ky, inst.transform.ty },
            { inst.transform.iz, inst.transform.jz, inst.transform.kz, inst.transform.tz },
         },
      }; // abusing initial list to init transform
      desc.InstanceID = inst.instanceId;
      desc.InstanceMask = inst.instanceMask;
      desc.InstanceContributionToHitGroupIndex = 0;
      desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
      ASSERT_DIE( inst.blas->GetBottomLevelAS() != nullptr );
      desc.AccelerationStructure = inst.blas->GetBottomLevelAS()->GpuStartAddress();
      instances.push_back( desc );
   }

   S<StructuredBuffer> instanceDesc 
      = StructuredBuffer::Create(sizeof(D3D12_RAYTRACING_INSTANCE_DESC), instances.size(), eBindingFlag::None, eAllocationType::Temporary);

   instanceDesc->SetCache( 0, instances.data(), instances.size() );
   instanceDesc->UploadGpu( list );

   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs = {};
   asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
   asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
   asInputs.InstanceDescs = instanceDesc->GpuStartAddress();
   asInputs.NumDescs = instanceDesc->GetElementCount();
   asInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;


   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO asBuildInfo = {};
   Device::Get().NativeDevice()->GetRaytracingAccelerationStructurePrebuildInfo( &asInputs, &asBuildInfo );

   S<Buffer> scratchBuffer = Buffer::Create( asBuildInfo.ScratchDataSizeInBytes, eBindingFlag::UnorderedAccess, Buffer::eBufferUsage::Default, eAllocationType::Temporary );
   mTlas = Buffer::Create( asBuildInfo.ResultDataMaxSizeInBytes, eBindingFlag::AccelerationStructure, Buffer::eBufferUsage::Default, eAllocationType::General );

   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
   desc.Inputs = asInputs;
   desc.ScratchAccelerationStructureData = scratchBuffer->GpuStartAddress();
   desc.DestAccelerationStructureData = mTlas->GpuStartAddress();

   list->Handle()->BuildRaytracingAccelerationStructure( &desc, 0, nullptr );

   if(commandList == nullptr) {
      Device::Get().GetMainQueue( eQueueType::Compute )->IssueCommandList( *list );
      list->~CommandList();
   }
}
