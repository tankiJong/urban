#pragma once
#include <vector>
#include "engine/core/span.hpp"
#include "util.hpp"
#include "engine/math/util.hpp"

struct Tile
{
   int x = 0, y = 0;

   int  Index() const { return TileIndex( x, y ); }
   bool Valid() const { return x >= 0 && y >= 0 && x < gMatchInfo.mapWidth && y < gMatchInfo.mapWidth; }

   Tile North() const { return { x, y + 1 }; }
   Tile South() const { return { x, y - 1 }; }
   Tile East()  const { return { x - 1, y }; }
   Tile West()  const { return { x + 1, y }; }

   struct Iter
   {
      char index = -1;
      const Tile* tile = nullptr;

      Tile operator*() const { return (tile->*sIters[index])(); }
      Iter& operator++() { index = min<char>(index+1, 4); return *this; }
      Iter& operator--() { index = max<char>(-1, index - 1); return *this; }
      bool operator==( const Iter& rhs ) const { return tile == rhs.tile && index == rhs.index; }
      bool operator!=( const Iter& rhs ) const { return !(*this == rhs); }
   };

   Iter begin() const { return { 0, this }; }
   Iter end()   const { return { 4, this }; }
protected:
   static inline decltype(&Tile::North) sIters[] = { &Tile::North, &Tile::South, &Tile::East, &Tile::West };
};

class DistanceMap
{
public:
   static constexpr int kInfinity = -1;
   DistanceMap();
   void ResetTargets( span<Tile> tiles );
   void Update();
protected:
   std::vector<int> mDistance;
   std::vector<Tile> mTargets;
};
