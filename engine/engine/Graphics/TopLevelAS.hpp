#pragma once
#include "Buffer.hpp"

class Mesh;

class TopLevelAS {
public:
   uint AddInstance( const Mesh& blas, uint8_t instanceMask = 0xFF, const mat44& transform = mat44::Identity );
   void Finalize(CommandList* commandList = nullptr);

   resource_handle_t Handle() const { ASSERT_DIE( mTlas != nullptr ); return mTlas->Handle(); };

protected:

   struct Instance {
      const Mesh* blas = nullptr;
      uint        instanceId = 0; // should only use low 24 bits
      uint8_t     instanceMask = 0; // 8 bits
      mat44       transform = mat44::Identity;
   };

   S<Buffer>             mTlas    = nullptr;
   bool                  mIsReady = false;
   std::vector<Instance> mInstances;
};
