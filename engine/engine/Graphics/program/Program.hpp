#pragma once

#include <array>

#include "BindingLayout.hpp"
#include "Shader.hpp"

class Program {
public:
   Program();

   const Shader& GetStage(eShaderType type) const { return mStages[uint(type)]; }
   Shader& GetStage(eShaderType type) { return mStages[uint(type)]; }

   const BindingLayout& GetBindingLayout() const { return mBindingLayout; };

protected:
   std::array<Shader, uint(eShaderType::Total)> mStages;
   BindingLayout mBindingLayout;
};
