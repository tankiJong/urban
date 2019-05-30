#pragma once
#include "engine/graphics/Descriptor.hpp"

class BindingLayout {
public:

   struct attrib {
      std::string name;
      uint        count = 1;
   };

   struct range {
      eDescriptorType     type;
      uint                baseRegisterIndex;
      uint                registerSpace;
      std::vector<attrib> attribs;
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
