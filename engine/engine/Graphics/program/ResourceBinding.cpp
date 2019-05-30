#include "engine/pch.h"
#include "ResourceBinding.hpp"
#include "engine/graphics/Device.hpp"
#include "Program.hpp"
#include <numeric>

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

void ResourceBinding::SetSrv( ShaderResourceView* srv, uint registerIndex, uint registerSpace )
{
   if(mIsDirty) RegenerateFlattened();

   auto& b = FindBindingItem( eDescriptorType::Srv, registerIndex, registerSpace );
   b.type = eDescriptorType::Srv;
   b.location = srv->Handle()->GetCpuHandle( 0 );
}

ResourceBinding::BindingItem& ResourceBinding::FindBindingItem( eDescriptorType type, uint registerIndex, uint registerSpace )
{
   for(auto& item: mFlattenedBindings) {
      if(item.type == type && item.registerIndex == registerIndex && item.registerSpace == registerSpace) {
         auto& range = mProgram->GetBindingLayout().Data()[item.rootIndex][item.rangeIndex];

         // same type
         ASSERT_DIE( range.type == type );
         // same space
         ASSERT_DIE( range.registerSpace == registerSpace );
         // In range
         ASSERT_DIE( range.baseRegisterIndex <= registerIndex && range.attribs.size() + range.baseRegisterIndex >= registerIndex );

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
                                      []( const BindingLayout::range& r ) { return r.attribs.size(); } );

                      return rangeSize;
                   } );

   size_t totalItemCount = 0;

   mFlattenedBindings.clear();
   for(uint i = 0; i < tableSizeLayout.size(); i++) {
      auto&  tableSize        = tableSizeLayout[i];
      uint currentTableSize = std::accumulate( tableSize.begin(), tableSize.end(), uint( 0 ) );
      totalItemCount += currentTableSize;

      uint offset = currentTableSize;
      for(uint j = 0; j < tableSize.size(); j++) {
         const BindingLayout::range& range = layout[i][j];

         descriptor_cpu_handle_t handle;
         switch(range.type) {
         case eDescriptorType::Srv: handle = ShaderResourceView::NullView()->Handle()->GetCpuHandle( 0 );
            break;
         case eDescriptorType::Cbv:
            // handle = ConstantBufferView::NullView()->Handle()->GetCpuHandle( 0 );
            // break;
         case eDescriptorType::Uav: 
            // handle = UnorderedAccessView::NullView()->Handle()->GetCpuHandle( 0 );
            //
            // break;
         case eDescriptorType::Sampler:
         case eDescriptorType::Rtv:
         case eDescriptorType::Dsv:
         case eDescriptorType::None:
         default:
         BAD_CODE_PATH();
         }

         for(uint rangeOffset = tableSize[j], registerIndexOffset = 0; rangeOffset > 0;
             --rangeOffset, ++registerIndexOffset) {
            mFlattenedBindings.push_back( {
               handle,
               range.type,
               range.baseRegisterIndex + registerIndexOffset,
               range.registerSpace,
               i,
               i,
               offset,
               rangeOffset
            } );
         }

         ASSERT_DIE( offset > 0 );
         --offset;
      }
   }

   mIsDirty = false;
}
