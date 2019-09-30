#pragma once

#include "es.hpp"

class Script {
public:
   Script() = default;
   
   Script( Script&& other ) noexcept = default;
   Script& operator=( Script&& other ) noexcept = default;
   
   static void FromFile( Script& inOutScript, const fs::path& file );

   v8::Global<v8::UnboundScript>& Handle() { return mHandle; }
protected:
   v8::Global<v8::UnboundScript> mHandle; 
};
