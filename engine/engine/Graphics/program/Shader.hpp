#pragma once

#include "engine/graphics/utils.hpp"
#include "engine/core/Blob.hpp"
#include "BindingLayout.hpp"

class Shader {
public:
   Shader() = default;

   Shader( eShaderType type, const void* data, size_t size )
      : mType( type )
    , mBinary( data, size ) {}

   size_t GetSize() const { return mBinary.Size(); }

   void* GetDataPtr() const { return mBinary.Data(); }

   void SetBinary( const void* data, size_t size ) { mBinary.Set( data, size ); }
   void SetType( eShaderType t ) { mType = t; }
protected:
   eShaderType   mType = eShaderType::Unknown;
   Blob          mBinary;
   BindingLayout mBindingLayout;
};
