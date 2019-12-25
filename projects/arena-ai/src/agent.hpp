#pragma once

#include "engine/pch.h"

#include <shared_mutex>
#include "../interface.hpp"
#include "util.hpp"
#include "engine/async/types.hpp"
#include "engine/async/semantics.hpp"
#include "map.hpp"

enum class ePheromone: int
{
   None = -1,
   Food,
   Danger,
   Queen,
   Total,
};

using Pheromone = std::array<uint, size_t(ePheromone::Total)>;

struct WorldState
{
   std::array<Pheromone, MAX_ARENA_TILES> pheromones;
   ArenaTurnStateForPlayer turnState;

   void IncreasePheromone(int x, int y, ePheromone type)
   {
      auto index = TileIndex( x, y );
      pheromones[index][int(type)]++;
   }

   void Decay()
   {
      for(auto& pheromone: pheromones) {
         for(auto& p: pheromone) {
            p = min( p, p - 1 );
         }
      }
   }
};


enum class eWorker: int {
   Unknown = -1,
   General = 0,
   Total,
};

class Master {
public:
   uint          IssueOrder( const AgentOrder& order );
   class Worker* CreateWorker( eWorker workerType );
   void          UpdateGameState( const ArenaTurnStateForPlayer& state );
   bool          SubmitOrders( PlayerTurnOrders* orderBuffer );
   const WorldState& ReadAndLockArenaState();
   WorldState& GetAndLockArenaState();
   void ReleaseArenaStateReadLock() { mStateLock.unlock_shared(); }
   void ReleaseArenaStateWriteLock() { mStateLock.unlock(); }
   void NotifyIdle( const Worker& worker );
   void BoardcastNewTurn() { mTurnEvent.Trigger(); }

   ~Master();
protected:
   std::shared_mutex mStateLock;
   WorldState mCurrentState;
   
   std::array<Worker*, size_t(eWorker::Total)> mWorkers;
   LockQueue<AgentOrder> mOrders;
   SysEvent mTurnEvent;
};

class Worker {
public:
   virtual ~Worker()
   {
      mTerminationEvent.Wait();
   };

   explicit Worker(Master& master): mOwner( &master ) {}

   void Run() {
      while(!mExiting) {
         if(!Execute()) {
            printf("unexpected exit");
            break;
         }
         mOwner->NotifyIdle( *this );
      }

      mTerminationEvent.Trigger();
   }

   void Terminate() { ASSERT_DIE(mExiting==false); mExiting = true; }

   const WorldState& AcquireReadCurrentState() { return mOwner->ReadAndLockArenaState(); }
   WorldState& AcquireWriteCurrentState() { return mOwner->GetAndLockArenaState(); }
   void FinishReadCurrentState() { return mOwner->ReleaseArenaStateReadLock(); }
   void FinishWriteCurrentState() { return mOwner->ReleaseArenaStateWriteLock(); }

   void Wait( SysEvent& eve ) const { eve.Wait(); }
protected:
   virtual bool Execute() = 0;

   bool mExiting = false;
   eWorker mType = eWorker::Unknown;
   Master* mOwner = nullptr;
   SysEvent mTerminationEvent;
};


class GeneralWorker: public Worker {
public:
   explicit GeneralWorker( Master& master )
      : Worker( master ) {}

   // ~GeneralWorker() override;
protected:
   bool Execute() override;

   AgentOrder CreateWorkerOrder( const AgentReport& agent, const WorldState& worldState );
};

class PathFindingWorker: public Worker
{
public:
   PathFindingWorker( Master& master )
      : Worker( master ) {}

   ~PathFindingWorker() override;
protected:
   bool Execute() override;
private:
   DistanceMap mQueenMap;
};