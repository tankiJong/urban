#pragma once

#include "engine/graphics/Descriptor.hpp"
#include "BindingLayout.hpp"

class ShaderReflection;

namespace ubsc
{
ShaderReflection Reflect( const void* data, size_t size );
};

class ShaderReflection {
   friend ShaderReflection ubsc::Reflect( const void* data, size_t size );
public:
   static constexpr uint kProgramRootIndexStart  = 6;
   static constexpr uint kReservedRootIndexStart = 0;
   static constexpr uint kMaxTableCount          = 6;

   struct InputBinding {
      std::string     name;
      eDescriptorType type;

      uint registerIndex;
      uint registerSpace;

      uint count;

      friend bool operator==( const InputBinding& lhs, const InputBinding& rhs );
      friend bool operator!=( const InputBinding& lhs, const InputBinding& rhs );

   };

   /**
    * \brief Build a reasonable binding layout according to the resource information.
    * Space 0: Reserved Static
    * Space 1: Reserved Mutable
    * Space 2: Reserved Dynamic
    * 
    * Space 3:  Program Static 
    * Space 4:  Program Mutable 
    * Space 5+: Program Dynamic 
    * 
    *  === Reserved ===
    * - Table[0]: Space0 - CBV, SRV, UAV
    * - Table[1]: Space0 - Sampler
    * 
    * - Table[2]: Space1 - CBV, SRV, UAV
    * - Table[3]: Space1 - Sampler
    * 
    * - Table[4]: Space2 - CBV, SRV, UAV
    * - Table[5]: Space2 - Sampler
    * 
    * === Program ===
    * - Table[6]: Space3 - CBV, SRV, UAV
    * - Table[7]: Space3 - Sampler
    * 
    * - Table[8]: Space4 - CBV, SRV, UAV
    * - Table[9]: Space4 - Sampler
    * 
    * - Table[10]: Space5+ - CBV, SRV, UAV
    * - Table[11]: Space5+ - Sampler
    *  
    * \return the layout
    */
   BindingLayout CreateBindingLayout( bool includeProgramBindings = true, bool includeReservedBindings = true ) const;

   const InputBinding& QueryBindingByName( std::string_view name ) const;

   static bool MergeInto( ShaderReflection* target, const ShaderReflection& from );
   static bool AreCompatible(const ShaderReflection& a, const ShaderReflection& b);
protected:
   std::map<std::string, InputBinding, std::greater<>> mBindedResources;
};

inline bool operator==( const ShaderReflection::InputBinding& lhs, const ShaderReflection::InputBinding& rhs )
{
   return lhs.name == rhs.name
          && lhs.type == rhs.type
          && lhs.registerIndex == rhs.registerIndex
          && lhs.registerSpace == rhs.registerSpace
          && lhs.count == rhs.count;
}

inline bool operator!=( const ShaderReflection::InputBinding& lhs, const ShaderReflection::InputBinding& rhs )
{
   return !(lhs == rhs);
}
