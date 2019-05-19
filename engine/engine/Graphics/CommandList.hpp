#pragma once

#include "engine/pch.h"
#include "utils.hpp"

class Buffer;
class Resource;

class CommandList {
public:

   // copy ------------------------------------------------------------//
   void CopyResource(Resource& from, Resource& to);

   // compute ------------------------------------------------------------//
   void Dispatch(uint groupx, uint groupy, uint groupz);

   // graphics ------------------------------------------------------------//
   void Draw(uint start, uint count);
   void DrawIndexed(uint vertStart, uint idxStart, uint count);
   void DrawInstanced(uint startVert, uint startIns, uint vertCount, uint insCount);
   void DrawIndirect(Buffer& args, uint count = 1, uint offset = 0);

protected:
   eCommandType mCommandsType = eCommandType::Empty;
};

