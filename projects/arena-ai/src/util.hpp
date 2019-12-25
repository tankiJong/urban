#pragma once
#include <mutex>
#include "../interface.hpp"
#include "engine/debug/assert.hpp"

extern MatchInfo gMatchInfo;
extern DebugInterface* gDebug;
__forceinline int TileIndex( int x, int y )
{
   ASSERT_DIE(x > 0 && y > 0);
   return y * gMatchInfo.mapWidth + x;
}
