﻿#include "engine/pch.h"
#include "utils.hpp"
#include "engine/core/Blob.hpp"
#include <fstream>

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

Blob fs::Read( const path& filePath )
{
   ASSERT_DIE( is_regular_file(filePath) );

   std::ifstream file( filePath.c_str(), std::ios::binary | std::ios::ate );

   std::streamsize size = file.tellg();

   if(size == -1) { return Blob(); }
   file.seekg( 0, std::ios::beg );

   char* buffer = new char[(uint)size + 1];

   file.read( buffer, size );
   buffer[size] = 0;
   Blob b( buffer, (uint)size );
   delete[] buffer;
   return b;
}

Blob fs::ReadText( const path& filePath )
{
   ASSERT_DIE( is_regular_file(filePath) );

   std::ifstream file( filePath.c_str(), std::ios::ate );

   std::streamsize size = file.tellg();

   if(size == -1) { return Blob(); }
   file.seekg( 0, std::ios::beg );

   char* buffer = new char[(uint)size];

   file.read( buffer, size );
   buffer[size - 1] = 0;
   Blob b( buffer, (uint)size+1 );
   delete[] buffer;
   return b;
}
