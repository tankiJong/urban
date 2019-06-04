#pragma once

#include <map>

#include "engine/file/utils.hpp"
#include "Blob.hpp"

template<typename T>
struct AssetInfo {
   S<T> asset;
   fs::path path;
   bool ready = false;
};

template<typename T>
class Asset {
public:
   static S<const T> Get(std::string_view name);
   static bool LoadAndRegister(fs::path path, bool blocking);
   static bool Register(const S<T>& res, fs::path filePath = "");
   static bool Load(S<T>& res, const Blob& binary);
protected:
   Asset() = delete;
   static std::map<std::string, AssetInfo<T>, std::less<>> sDatabase;
};

template<typename T>
std::map<std::string, AssetInfo<T>, std::less<>> Asset<T>::sDatabase = {};

template< typename T > S<const T> Asset<T>::Get( std::string_view name )
{
   auto kv = sDatabase.find( name );

   if(kv == sDatabase.end()) {
      WARN( "The resource does not exist" );
      DEBUGBREAK;
      return nullptr;
   }

   return kv->second.asset;
}

template< typename T > bool Asset<T>::LoadAndRegister( fs::path path, bool blocking )
{
   Blob b = fs::Read( path );
   S<T> res;
   bool re1 = Load( res, b );
   return re1 && Register( res, path );
}

template< typename T > bool Asset<T>::Register( const S<T>& res, fs::path filePath )
{
   AssetInfo<T> asset;
   asset.asset = res;
   asset.path = filePath;
   asset.ready = true;

   auto kv = sDatabase.find( filePath );

   if(kv != sDatabase.end()) {
      WARN( "The resource already exists" );
      DEBUGBREAK;
      return false;
   }

   sDatabase[filePath.generic_string()] = asset;

   return true;
}
