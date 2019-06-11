#pragma once
#include "BindingLayout.hpp"
#include "engine/graphics/ResourceView.hpp"

class Sampler;
class Program;

class ResourceBinding {
public:
   struct BindingItem {
      descriptor_cpu_handle_t location;
      eDescriptorType type;
      uint registerIndex;
      uint registerSpace;

      // root index in d3d12
      uint rootIndex;
      // which range inside the table
      uint rangeIndex;

      // In an array, items[ current + items[current].nextRangeOffset ] will directly jump to the next range.
      // In case this value is 1, that means next item is the head of a range.
      uint            nextTableOffset;

      // is invalid when the next item is the next table
      uint            nextRangeOffset;
   };

   using Flattened = std::vector<BindingItem>;

   ResourceBinding() = default;
   ResourceBinding( const Program* prog );
   ~ResourceBinding();

   const Program* GetProgram() const { return mProgram; }

   const Flattened& GetFlattened() const;
   void             SetSrv( const ShaderResourceView* srv,  uint registerIndex, uint registerSpace = 0 );
   void             SetCbv( const ConstantBufferView* cbv,  uint registerIndex, uint registerSpace = 0 );
   void             SetUav( const UnorderedAccessView* uav, uint registerIndex, uint registerSpace = 0 );
   void             SetSampler( const Sampler*     sampler, uint registerIndex, uint registerSpace = 0 );

protected:

   BindingItem& FindBindingItem(eDescriptorType type, uint registerIndex, uint registerSpace);
   void RegenerateFlattened() const;

   const Program*    mProgram = nullptr;
   mutable Flattened mFlattenedBindings;
   mutable bool mIsDirty = true;
};
