#include "map.hpp"
#include <queue>

DistanceMap::DistanceMap()
{
   mDistance.resize( gMatchInfo.mapWidth * gMatchInfo.mapWidth, -1 );
}

void DistanceMap::ResetTargets( span<Tile> tiles )
{
   mTargets.clear();
   for(auto& tile: tiles) {
      mTargets.push_back( tile );
   }
}

void DistanceMap::Update()
{
   std::vector<bool> dirtyMap;
   std::vector<int>  distanceMap;;
   dirtyMap.resize( mDistance.size(), true );
   distanceMap.resize( mDistance.size(), kInfinity );

   std::queue<Tile> pendingUpdate;

   for(auto& target: mTargets) {
      pendingUpdate.push( target );
      distanceMap[target.Index()] = 0;
      dirtyMap[target.Index()] = false;
   }

   while(!pendingUpdate.empty()) {
      Tile tile = pendingUpdate.front();
      int current = distanceMap[tile.Index()];

      for(Tile t: tile) {
         if( !t.Valid() ) continue;
         if(auto index = t.Index(); dirtyMap[index]) {
            pendingUpdate.push( t );
            dirtyMap[index] = false;
            distanceMap[index] = current + 1;
         } else {
            distanceMap[index] = min( current + 1, distanceMap[index] );
         }
      }
      pendingUpdate.pop();
   }


   memcpy( mDistance.data(), distanceMap.data(), sizeof( int ) * mDistance.size() );
}
