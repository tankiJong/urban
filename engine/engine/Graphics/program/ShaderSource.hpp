#pragma once

#include "ShaderCompiler.hpp"

enum class eShaderCompileFlag: unsigned short;
enum class eShaderType: unsigned;
class Shader;



class ShaderSource {
public:
   ShaderSource( std::string_view source, std::string_view name = "Undefined" ): mSource( source ), mName( name ) {}

   Shader Compile(
      eShaderType                   type,
      std::string_view              entryPoint,
      std::vector<std::string_view> defineList = {},
      eShaderCompileFlag            flags      = eShaderCompileFlag::None);
protected:
   std::string mSource;
   std::string mName;
};
