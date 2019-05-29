#pragma once
#include "BindingLayout.hpp"

class ResourceBinding {
public:
   const BindingLayout& GetBindingLayout() const { return mBindingLayout; }

protected:
   BindingLayout mBindingLayout;
};
