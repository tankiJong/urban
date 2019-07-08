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

BindingLayout ShaderReflection::CreateBindingLayout( bool includeProgramBindings, bool includeReservedBindings) const
{
   std::vector<BindingLayout::table_t> tables;

   tables.resize( kMaxTableCount * 2 );

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

         if( ranges.size() == 0) {
            ranges.push_back( range );
         } else {
            if(res.registerIndex < ranges[0].BaseRegisterIndex()) {
               ranges.insert( ranges.begin(), range );
            } else {
               ranges.push_back( range );
            }
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

   uint samplerSpaceMap[kMaxTableCount] = { 1, 3, 5, 7, 9, 11 };
   uint viewSpaceMap[kMaxTableCount]    = { 0, 2, 4, 6, 8, 10 };
   {
      range_map_t* maps[] = {
         &cbvRanges,
         &srvRanges,
         &uavRanges,
      };
      for(auto& rangeMap: maps) {
         for(auto& [registerSpace, range]: *rangeMap) {
            uint tableIndex = clamp(registerSpace, 0, kMaxTableCount);
            tableIndex = viewSpaceMap[tableIndex];
            tables[tableIndex].insert( 
               tables[tableIndex].end(), 
               range.begin(), range.end() );
         }
      }
   }
   {
      auto rangeMap = &samplerRanges;
      for(auto& [registerSpace, range]: *rangeMap) {
         uint tableIndex = clamp(registerSpace, 0, kMaxTableCount);
         tableIndex = samplerSpaceMap[tableIndex];
         tables[tableIndex].insert( 
            tables[tableIndex].end(), 
            range.begin(), range.end() );
      }
   }


   auto iterBegin = tables.begin();
   auto iterEnd = tables.end();

   if(!includeReservedBindings) {
      iterBegin += kProgramRootIndexStart;
   }

   if(!includeProgramBindings) {
      iterEnd -= kProgramRootIndexStart;
   }

   span<BindingLayout::table_t> finalTables { &*iterBegin, iterEnd - iterBegin };
   BindingLayout::Option options;

   options.includeProgramLayout = includeProgramBindings;
   options.includeReservedLayout = includeReservedBindings;

   ASSERT_DIE( includeProgramBindings || includeReservedBindings );

   return { finalTables, options };
}

const ShaderReflection::InputBinding& ShaderReflection::QueryBindingByName( std::string_view name ) const
{
   auto iter = mBindedResources.find( name );
   ASSERT_DIE( iter != mBindedResources.end() );
   return iter->second;
}

bool ShaderReflection::MergeInto( ShaderReflection* target, const ShaderReflection& from )
{
   if(!AreCompatible( *target, from )) return false;

   for(auto& [name, binding]: from.mBindedResources) {
      if(target->mBindedResources.find( name ) == target->mBindedResources.end()) {
         target->mBindedResources[name] = binding;
      }
   }

   return true;
}

bool ShaderReflection::AreCompatible( const ShaderReflection& a, const ShaderReflection& b )
{
   std::vector<std::string> commonNames;
   for(auto&    [nameA, resA]: a.mBindedResources) {
      for(auto& [nameB, resB]: b.mBindedResources) {
         if(nameA == nameB && resA != resB) {
            return false;
         }
      }
   }

   return true;
}
