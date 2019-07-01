#include "engine/pch.h"
#include "ShaderReflection.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

bool OptimizeRanges(std::vector<BindingLayout::range>* outRange, const std::vector<BindingLayout::range>& ranges)
{
   return false;
}



////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

BindingLayout ShaderReflection::CreateBindingLayout() const
{
   std::vector<BindingLayout::table_t> tables;

   tables.resize( 2 );

   using range_map_t = std::map<uint, std::vector<BindingLayout::range>>;
   range_map_t cbvRanges, srvRanges, uavRanges, samplerRanges;

   // insert resources
   for(const auto& [name, res]: mBindedResources) {
      range_map_t* map = nullptr;

      switch(res.type) {
      case eDescriptorType::Srv: map = &srvRanges;
         break;
      case eDescriptorType::Uav: map = &uavRanges;
         break;
      case eDescriptorType::Cbv: map = &cbvRanges;
         break;
      case eDescriptorType::Sampler: map = &samplerRanges;
         break;
      default:
      BAD_CODE_PATH();
      }

      std::vector<BindingLayout::range>& ranges = (*map)[res.registerSpace];

      /*
       *      Given ranges, try to insert it into a proper range,
       *      |x - - - - x| |x - - - - x|... |x - - - - x|
       */

      bool inserted = false;

      // setting up the range info
      for(size_t i = 0; i < ranges.size(); i++) {
         auto& range = ranges[i];
         ASSERT_DIE( range.RegisterSpace() == res.registerSpace );
         ASSERT_DIE( range.Type() == res.type );

         // in the range
         if( res.registerIndex > range.BaseRegisterIndex() 
          && res.registerIndex < range.BaseRegisterIndex() + range.Attribs().size()) {
            BAD_CODE_PATH();
         }
            // right before the range
         else if(res.registerIndex + 1 == range.BaseRegisterIndex()) {
            range.Prepend( res.name, res.count );
            inserted = true;
            break;
         }
            // right after the range
         else if(res.registerIndex == range.EndRegisterIndex() + 1) {
            range.Append( res.name, res.count );
            inserted = true;
            break;
         }
      }

      // did not insert into any range, append a new range and insert into it
      if(!inserted) {
         BindingLayout::range range( res.type, res.registerIndex, res.registerSpace );
         range.Append( res.name, res.count );

         if(res.registerIndex < ranges[0].BaseRegisterIndex()) { ranges.insert( ranges.begin(), range ); } else {
            ranges.push_back( range );
         }
      }
   }

   // try to merge the ranges if possible
   {
      range_map_t* maps[] = {
         &cbvRanges,
         &srvRanges,
         &uavRanges,
         &samplerRanges,
      };
      for(auto&    rangeMap: maps) {
         for(auto& [spaceIndex, range]: *rangeMap) {
            std::vector<BindingLayout::range> optimizedRanges;
            if(OptimizeRanges( &optimizedRanges, range )) { range = optimizedRanges; }
         }
      }
   }

   // sorting ranges by register space
   {
      // table *0* // 
      range_map_t* maps[] = {
         &cbvRanges,
         &srvRanges,
         &uavRanges,
         &samplerRanges,
      };
      for(auto& rangeMap: maps) {
         auto iter = rangeMap->find( 0 );
         if(iter == rangeMap->end()) { iter = rangeMap->upper_bound( 0 ); }
         while(iter != rangeMap->end()) {
            tables[0].insert( tables[0].end(), iter->second.begin(), iter->second.end() );
            iter = rangeMap->upper_bound( iter->first );
         }
      }

      // table *1* // 
      {
         auto rangeMap = &samplerRanges;
         auto iter     = rangeMap->find( 0 );
         if(iter == rangeMap->end()) { iter = rangeMap->upper_bound( 0 ); }
         while(iter != rangeMap->end()) {
            tables[0].insert( tables[0].end(), iter->second.begin(), iter->second.end() );
            iter = rangeMap->upper_bound( iter->first );
         }
      }
   }

   return { tables };
}

const ShaderReflection::InputBinding& ShaderReflection::QueryBindingByName( std::string_view name ) const
{
   auto iter = mBindedResources.find( name );
   ASSERT_DIE( iter != mBindedResources.end() );
   return iter->second;
}
