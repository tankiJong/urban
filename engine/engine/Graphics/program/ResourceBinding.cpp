#include "engine/pch.h"
#include "ResourceBinding.hpp"
#include "engine/graphics/Device.hpp"
#include "Program.hpp"
#include <numeric>
#include "engine/graphics/Sampler.hpp"

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


ResourceBinding::ResourceBinding( const Program* prog ) 
   : mProgram( prog )
{
   
}

ResourceBinding::~ResourceBinding()
{

}

const ResourceBinding::Flattened& ResourceBinding::GetFlattened() const
{
   if(mIsDirty) {
      RegenerateFlattened();
   }
   return mFlattenedBindings;
}

void ResourceBinding::SetSrv( const ShaderResourceView* srv, uint registerIndex, uint registerSpace )
{
   if(mIsDirty) RegenerateFlattened();

   auto& b = FindBindingItem( eDescriptorType::Srv, registerIndex, registerSpace );
   b.type = eDescriptorType::Srv;
   b.location = srv->Handle()->GetCpuHandle( 0 );
}

void ResourceBinding::SetCbv( const ConstantBufferView* cbv, uint registerIndex, uint registerSpace )
{
   if(mIsDirty) RegenerateFlattened();

   auto& b    = FindBindingItem( eDescriptorType::Cbv, registerIndex, registerSpace );
   b.type     = eDescriptorType::Cbv;
   b.location = cbv->Handle()->GetCpuHandle( 0 );
}

void ResourceBinding::SetUav( const UnorderedAccessView* uav, uint registerIndex, uint registerSpace )
{
   if(mIsDirty) RegenerateFlattened();

   auto& b    = FindBindingItem( eDescriptorType::Uav, registerIndex, registerSpace );
   b.type     = eDescriptorType::Uav;
   b.location = uav->Handle()->GetCpuHandle( 0 );
}

void ResourceBinding::SetSampler( const Sampler* sampler, uint registerIndex, uint registerSpace )
{
   if(mIsDirty) RegenerateFlattened();

   auto& b    = FindBindingItem( eDescriptorType::Sampler, registerIndex, registerSpace );
   b.type     = eDescriptorType::Sampler;
   b.location = sampler->GetCpuHandle();
}

ResourceBinding::BindingItem& ResourceBinding::FindBindingItem( eDescriptorType type, uint registerIndex, uint registerSpace )
{
   for(auto& item: mFlattenedBindings) {
      if(item.type == type && item.registerIndex == registerIndex && item.registerSpace == registerSpace) {
         auto& range = mProgram->GetBindingLayout().Data()[item.rootIndex][item.rangeIndex];

         // same mType
         ASSERT_DIE( range.Type() == type );
         // same space
         ASSERT_DIE( range.RegisterSpace() == registerSpace );
         // In range
         ASSERT_DIE( range.BaseRegisterIndex() <= registerIndex && range.Attribs().size() + range.BaseRegisterIndex() >= registerIndex );

         return item;
      }
   }

   BAD_CODE_PATH();
}

void ResourceBinding::RegenerateFlattened() const
{
   ASSERT_DIE_M( mProgram->Ready(), "Resource Binding is not usable before the program is finalized." );

   const auto& layout = mProgram->GetBindingLayout().Data();

   std::vector<std::vector<size_t>> tableSizeLayout;
   std::transform( layout.begin(), layout.end(), std::back_inserter( tableSizeLayout ),
                   []( const BindingLayout::table_t& t )
                   {
                      std::vector<size_t> rangeSize;
                      std::transform( t.begin(), t.end(), std::back_inserter( rangeSize ),
                                      []( const BindingLayout::range& r ) { return r.Attribs().size(); } );

                      return rangeSize;
                   } );

   size_t totalItemCount = 0;

   mFlattenedBindings.clear();
   for(size_t i = 0; i < tableSizeLayout.size(); i++) {
      auto&  tableSize        = tableSizeLayout[i];
      size_t currentTableSize = std::accumulate( tableSize.begin(), tableSize.end(), size_t( 0 ) );
      totalItemCount += currentTableSize;

      size_t offset = currentTableSize;
      for(size_t j = 0; j < tableSize.size(); j++) {
         const BindingLayout::range& range = layout[i][j];

         descriptor_cpu_handle_t handle;
         switch(range.Type()) {
         case eDescriptorType::Srv: 
            handle = ShaderResourceView::NullView()->Handle()->GetCpuHandle( 0 );
            break;
         case eDescriptorType::Cbv:
            handle = ConstantBufferView::NullView()->Handle()->GetCpuHandle( 0 );
            break;
         case eDescriptorType::Uav: 
            handle = UnorderedAccessView::NullView()->Handle()->GetCpuHandle( 0 );
            break;
         case eDescriptorType::Sampler:
            handle = Sampler::Nearest()->GetCpuHandle();
            break;
         case eDescriptorType::Rtv:
         case eDescriptorType::Dsv:
         case eDescriptorType::None:
         default:
         BAD_CODE_PATH();
         }

         for(size_t rangeOffset = tableSize[j], registerIndexOffset = 0; rangeOffset > 0;
             --rangeOffset, ++registerIndexOffset) {
            mFlattenedBindings.push_back( {
               handle,
               range.Type(),
               range.BaseRegisterIndex() + (uint)registerIndexOffset,
               range.RegisterSpace(),
               (uint)i,
               (uint)j,
               (uint)offset,
               (uint)rangeOffset
            } );
         }

         ASSERT_DIE( offset > 0 );
         --offset;
      }
   }

   mIsDirty = false;
}
