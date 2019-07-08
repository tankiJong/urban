#include "engine/pch.h"
#include "BindingLayout.hpp"
#include <numeric>
#include "ShaderReflection.hpp"

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

uint BindingLayout::range::Prepend( std::string name, uint count )
{
   ASSERT_DIE( mBaseRegisterIndex - count < mBaseRegisterIndex );
   mAttribs.insert( mAttribs.begin(), { name, count } );
   mBaseRegisterIndex -= count;
   mTotal += count;
   return mTotal;
}

uint BindingLayout::range::Append( std::string name, uint count )
{
   mAttribs.push_back( attrib{ name, count } );
   mTotal += count;
   return mTotal;
}

const BindingLayout::attrib& BindingLayout::range::At( uint registerIndex ) const
{
   uint currentMaxRegister = mBaseRegisterIndex;

   for(size_t i = 0; i < mAttribs.size(); i++) {
      if(registerIndex < mAttribs[i].count + currentMaxRegister && registerIndex >= currentMaxRegister ) {
         return mAttribs[i];
      }
      currentMaxRegister += mAttribs[i].count;
   }

   FATAL( "Try to look up invalid register index" );
}

uint BindingLayout::table_t::ElementCount() const
{
   return std::accumulate( begin(), end(), uint( 0 ), 
                 [](uint v, const range& a) { return a.Total() + v; });
}

BindingLayout::BindingLayout( span<const table_t> ranges, const Option& op )
   : mLayout( ranges.data(), ranges.data() + ranges.size() )
   , mOptions( op )
{
}

BindingLayout::~BindingLayout()
{
}

BindingLayout BindingLayout::GetPartLayout( const Option& op ) const
{
   ASSERT_DIE( !op.includeProgramLayout || mOptions.includeProgramLayout );
   ASSERT_DIE( !op.includeReservedLayout || mOptions.includeReservedLayout );

   // double cuz samplers and views are in different tables
   ASSERT_DIE( mLayout.size() <= ShaderReflection::kMaxTableCount * 2 );


   ASSERT_DIE( ShaderReflection::kProgramRootIndexStart > ShaderReflection::kReservedRootIndexStart );

   /*        table: beg -------- mid -------- end        */
   const table_t* beg = mLayout.data();
   const table_t* end = beg + mLayout.size();
   const table_t* mid = beg + ShaderReflection::kProgramRootIndexStart;

   if( !mOptions.includeReservedLayout ) {
      mid = beg;
   }

   if (!mOptions.includeProgramLayout) {
      mid = end;
   }

   /*       
    *         IncludeReserved | IncludeProgram
    *         
    *             mid       begin
    *             
    *     mid     00          10
    *     
    *     end     01          11
    */

   const table_t* mapTable[2][2] = {
      { mid, beg },
      { mid, end },
   };

   const table_t* retBeg = mapTable[0][op.includeReservedLayout ? 1 : 0];
   const table_t* retEnd = mapTable[1][op.includeProgramLayout  ? 1 : 0];

   return BindingLayout{ span<const table_t>{ retBeg, retEnd - retBeg }, op };
}
