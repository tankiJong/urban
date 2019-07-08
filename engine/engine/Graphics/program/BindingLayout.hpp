#pragma once
#include "engine/graphics/Descriptor.hpp"

class BindingLayout {
public:
   struct Option {
      bool includeReservedLayout = true;
      bool includeProgramLayout = true;
   };

   struct attrib {
      std::string name;
      uint        count = 1;
   };

   struct range {

      range( eDescriptorType type, uint baseRegisterIndex, uint registerSpace )
      : mType( type )
      , mBaseRegisterIndex( baseRegisterIndex )
      , mRegisterSpace( registerSpace )
      , mTotal( 0 ) {}

      range( eDescriptorType type, uint baseRegisterIndex, uint registerSpace, std::vector<attrib> attribs )
      : mType( type )
      , mBaseRegisterIndex( baseRegisterIndex )
      , mRegisterSpace( registerSpace )
      , mTotal( attribs.size() )
      , mAttribs( std::move( attribs ) ) {}

      eDescriptorType Type() const { return mType; }
      uint BaseRegisterIndex() const { return mBaseRegisterIndex; }
      uint RegisterSpace() const { return mRegisterSpace; }
      uint Total() const { return mTotal; }

      uint EndRegisterIndex() const { return mBaseRegisterIndex + mTotal - 1; }

      // return current total count
      uint Prepend(std::string name, uint count);

      // return current total count
      uint Append(std::string name, uint count);

      const attrib& At(uint registerIndex) const;

      span<const attrib> Attribs() const { return mAttribs; };

   protected:
      eDescriptorType     mType;
      uint                mBaseRegisterIndex;
      uint                mRegisterSpace;
      uint                mTotal;
      std::vector<attrib> mAttribs;
   };

   struct table_t: public std::vector<range> {
      table_t() = default;
      table_t(std::vector<range> ranges, bool isStatic)
         : std::vector<range>(std::move(ranges))
         , isStatic( isStatic ) {}
      bool isStatic = false;
      uint ElementCount() const;
   };

   BindingLayout(): BindingLayout( std::vector<table_t>{}, Option{} ) {}
   BindingLayout( span<const table_t> ranges, const Option& op );
   BindingLayout( const std::vector<table_t>& ranges, const Option& op )
      : BindingLayout( span<const table_t>(ranges), op ) {}

   span<const table_t> Data() const { return mLayout; }
   span<table_t>       Data()       { return mLayout; }
   ~BindingLayout();

   BindingLayout( const BindingLayout& other ) = default;
   BindingLayout( BindingLayout&& other ) noexcept = default;
   BindingLayout& operator=( const BindingLayout& other ) = default;
   BindingLayout& operator=( BindingLayout&& other ) noexcept = default;

   BindingLayout GetPartLayout(const Option& op) const;

   void MarkTableStatic(uint index) { mLayout[index].isStatic = true; }
protected:
   std::vector<table_t> mLayout;
   Option mOptions;
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
