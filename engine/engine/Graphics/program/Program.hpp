#pragma once

#include <array>

#include "BindingLayout.hpp"
#include "Shader.hpp"

class Program: public WithHandle<rootsignature_t> {
public:
   Program();
   ~Program();
   const Shader& GetStage(eShaderType type) const { return mStages[uint(type)]; }
   Shader& GetStage(eShaderType type) { mIsReady = false; return mStages[uint(type)]; }

   const BindingLayout& GetBindingLayout() const { return mBindingLayout; };

   void Finalize();
   bool Ready() const { return mIsReady; }
protected:
   std::array<Shader, uint(eShaderType::Total)> mStages;
   BindingLayout mBindingLayout;
   bool mIsReady = false;
};
