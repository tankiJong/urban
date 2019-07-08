#pragma once

#include "engine/graphics/utils.hpp"
#include "engine/core/Blob.hpp"
#include "BindingLayout.hpp"
#include "ShaderReflection.hpp"

class Shader {
public:
   Shader() = default;

   Shader( eShaderType type, const void* data, size_t size )
      : mType( type )
    , mBinary( data, size ) {}

   size_t GetSize() const { return mBinary.Size(); }
   void* GetDataPtr() const { return mBinary.Data(); }
   void SetBinary( const void* data, size_t size );

   const BindingLayout& GetBindingLayout() const { return mBindingLayout; }
   const ShaderReflection& GetShaderReflection() const { return mReflection; }
   void SetType( eShaderType t ) { mType = t; }

   bool Valid() const { return mBinary.Size() > 0; } 
protected:

   void SetupBindingLayout();
   eShaderType   mType = eShaderType::Unknown;
   Blob          mBinary;
   BindingLayout mBindingLayout;
   ShaderReflection mReflection;
};
