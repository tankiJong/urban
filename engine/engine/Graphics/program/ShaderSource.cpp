﻿#include "engine/pch.h"
#include "ShaderSource.hpp"
#include "engine/graphics/program/Shader.hpp"
#include "engine/graphics/utils.hpp"
#include "ShaderCompiler.hpp"

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

Shader ShaderSource::Compile(
   eShaderType type,
   std::string_view entryPoint,
   std::vector<std::string_view> defineList,
   eShaderCompileFlag flags ) {

   return ubsc::Compile( type, mSource, mName, entryPoint, defineList, flags );
}