#pragma once

enum class eShaderType: unsigned;
class Shader;
class ShaderSource;
class ShaderReflection;
class BindingLayout;

enum class eShaderCompileFlag: uint16_t {
   None = 0,
};

enum_class_operators( eShaderCompileFlag );

namespace ubsc
{
ShaderSource FromString( std::string_view source, std::string_view name = "Undefined" );

Shader Compile(
   eShaderType                   type,
   std::string_view              source,
   std::string_view              name,
   std::string_view              entryPoint,
   std::vector<std::string_view> defineList = {},
   eShaderCompileFlag            flags      = eShaderCompileFlag::None );

Shader CompileFromFile(
   fs::path                      filePath,
   eShaderType                   type,
   std::string_view              entryPoint,
   std::vector<std::string_view> defineList = {},
   eShaderCompileFlag            flags      = eShaderCompileFlag::None);

ShaderReflection Reflect( const void* data, size_t size );
}
