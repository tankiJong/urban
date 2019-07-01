#pragma once

class Shader;
class ShaderSource;
class ShaderReflection;
class BindingLayout;

namespace ubsc
{
ShaderSource FromString(std::string_view source, std::string_view name = "Undefined");
ShaderReflection Reflect(const void* data, size_t size);
}