#pragma once

#include <array>

#include "BindingLayout.hpp"
#include "Shader.hpp"

class Program: public WithHandle<rootsignature_t> {
public:
   Program( const Program& other ) = default;
   Program( Program&& other ) noexcept = default;
   Program& operator=( const Program& other ) = default;
   Program& operator=( Program&& other ) noexcept = default;
   Program();
   ~Program();

   const Shader& GetStage(eShaderType type) const { return mStages[uint(type)]; }
   Shader& GetStage(eShaderType type) { mIsReady = false; return mStages[uint(type)]; }

   const BindingLayout& GetBindingLayout() const { return mBindingLayout; };
   const ShaderReflection& GetProgramReflection() const { return mShaderReflection; }

   void Finalize();
   bool Ready() const { return mIsReady; }

protected:
   std::array<Shader, uint(eShaderType::Total)> mStages;
   BindingLayout mBindingLayout;
   ShaderReflection mShaderReflection;
   bool mIsReady = false;
};
