#pragma once
#include "engine/graphics/Descriptor.hpp"

class BindingLayout {
public:

   struct attrib {
      std::string name;
      uint        count = 1;
   };

   struct range {

      range( eDescriptorType type, uint baseRegisterIndex, uint registerSpace )
      : mType( type )
      , mBaseRegisterIndex( baseRegisterIndex )
      , mRegisterSpace( registerSpace ) {}

      eDescriptorType Type() const { return mType; }
      uint BaseRegisterIndex() const { return mBaseRegisterIndex; }
      uint RegisterSpace() const { return mRegisterSpace; }
      uint Total() const { return mTotal; }

      uint EndRegisterIndex() const { return mBaseRegisterIndex + mTotal - 1; }
      uint Prepend(std::string name, uint count);
      uint Append(std::string name, uint count);

      const attrib& At(uint registerIndex) const;

      span<const attrib> Attribs() const;
   protected:
      eDescriptorType     mType;
      uint                mBaseRegisterIndex = 0;
      uint                mRegisterSpace = 0;
      uint                mTotal = 0;
      std::vector<attrib> mAttribs = {};
   };

   using table_t = std::vector<range>;

   BindingLayout(): BindingLayout( std::vector<table_t>{} ) {}
   BindingLayout( const std::vector<table_t>& ranges );

   const std::vector<table_t>& Data() const { return mLayout; }
   std::vector<table_t>&       Data() { return mLayout; }
   ~BindingLayout();
protected:
   std::vector<table_t> mLayout;
};


// BindingLayout l =  { {
//    // dt2
//    {
//       {
//          eDescriptorType::Cbv,
//          0, 0,
//          {
//             { "gAlbedo" },
//             { "gAlbedo" },
//             { "gAlbedo" },
//          },
//       },
//    },
// } };
