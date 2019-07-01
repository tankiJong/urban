#pragma once

#include "engine/graphics/Descriptor.hpp"
#include "BindingLayout.hpp"

class ShaderReflection;
namespace ubsc
{
   ShaderReflection Reflect(const void* data, size_t size);
};

class ShaderReflection {
   friend ShaderReflection ubsc::Reflect(const void* data, size_t size);
public:

   struct InputBinding {
      std::string name;
      eDescriptorType type;

      uint registerIndex;
      uint registerSpace;

      uint count;
   };

   /**
    * \brief Build a reasonable binding layout according to the resource information, with the following layout:
    *  - Table[0] CBV_space0, CBV_space1, ...; SRV_space0, SRV_space1, ...; UAV_space0, UAV_space1, ...;
    *  - Table[1] Samplers
    * 
    * \return the layout
    */
   BindingLayout CreateBindingLayout() const;

   const InputBinding& QueryBindingByName(std::string_view name) const;

protected:
   std::map<std::string, InputBinding, std::greater<>> mBindedResources;
};
