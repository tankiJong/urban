#include "agent.hpp"
#include "util.hpp"
#include "engine/core/random.hpp"

bool HasFood(const ArenaTurnStateForPlayer& turnState, int x, int y)
{
   auto tileIndex = TileIndex( x, y );
   return turnState.tilesThatHaveFood[tileIndex];
}

bool CanDig(const ArenaTurnStateForPlayer& turnState, int x, int y)
{
   auto tileIndex = TileIndex( x, y );
   return turnState.observedTiles[tileIndex] == TILE_TYPE_DIRT;
}

uint Master::IssueOrder( const AgentOrder& order )
{
   uint index = mOrders.Enqueue( order );
   ENSURES( index < MAX_ORDERS_PER_PLAYER );
   return index;
}

Worker* Master::CreateWorker(eWorker workerType) {
   Worker* worker = nullptr;
   if( int( workerType ) >= int( eWorker::Total ) ) return worker;

   if(workerType == eWorker::General) {
      worker = new GeneralWorker{*this};
   }

   mWorkers[(int)workerType] = worker;

   return worker;
}

void Master::UpdateGameState(const ArenaTurnStateForPlayer& state)
{
   std::scoped_lock lock( mStateLock );
   mCurrentState.turnState = state;
   BoardcastNewTurn();
}

bool Master::SubmitOrders(PlayerTurnOrders* orderBuffer)
{
   std::vector<AgentOrder> orders;
   orders.reserve( MAX_ORDERS_PER_PLAYER );
   mOrders.FlushAndClear( orders );

   EXPECTS( orders.size() <= MAX_ORDERS_PER_PLAYER );
   memcpy( orderBuffer->orders, orders.data(), sizeof( AgentOrder ) * orders.size() );
   orderBuffer->numberOfOrders = orders.size();

   return true;
}

const WorldState& Master::ReadAndLockArenaState()
{
   mStateLock.lock_shared();
   return mCurrentState;
}

WorldState& Master::GetAndLockArenaState()
{
   mStateLock.lock();
   return mCurrentState;
}

void Master::NotifyIdle( const Worker& worker )
{
   worker.Wait( mTurnEvent );
}

Master::~Master()
{
   for(Worker*& worker: mWorkers) {
      if(worker != nullptr) {
         worker->Terminate();
      }
   }

   mTurnEvent.Trigger();

   for(Worker*& worker: mWorkers) {
      SAFE_DELETE( worker );
   }
}

bool GeneralWorker::Execute()
{
   auto& state = AcquireReadCurrentState();

   for(int i = 0; i < state.turnState.numReports; i++) {
      auto& agentReport = state.turnState.agentReports[i];
      if( agentReport.state == STATE_DEAD ) continue;
      AgentOrder order;
      switch(agentReport.type) {
      case AGENT_TYPE_SCOUT: break;
      case AGENT_TYPE_WORKER: order = CreateWorkerOrder( agentReport, state ); break;
      case AGENT_TYPE_SOLDIER: break;
      case AGENT_TYPE_QUEEN: break;
      case NUM_AGENT_TYPES: 
      case INVALID_AGENT_TYPE:
      default: BAD_CODE_PATH();
      }
      mOwner->IssueOrder( order );
   }
   FinishReadCurrentState();
}

static_assert(ORDER_MOVE_EAST + 1 == ORDER_MOVE_NORTH);
static_assert(ORDER_MOVE_NORTH + 1 == ORDER_MOVE_WEST);
static_assert(ORDER_MOVE_WEST + 1 == ORDER_MOVE_SOUTH);
AgentOrder GeneralWorker::CreateWorkerOrder( const AgentReport& agent, const WorldState& worldState )
{
   EXPECTS( agent.type == AGENT_TYPE_WORKER );

   AgentOrder order;
   order.agentID = agent.agentID;
   order.order = ORDER_EMOTE_CONFUSED;
   if( agent.exhaustion > 0 ) {
      order.order = ORDER_HOLD;
      return order;
   }
   ASSERT_RECOVERABLE( agent.result < AGENT_ORDER_ERROR_BAD_ANT_ID || agent.result == AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE);

   auto actionSearchFood = [&]()
   {
      if(HasFood( worldState.turnState, agent.tileX, agent.tileY )) {
         order.order = ORDER_PICK_UP_FOOD;
         return;
      }
      if(CanDig( worldState.turnState, agent.tileX, agent.tileY )) {
         order.order = ORDER_DIG_HERE;
         return;
      }

      int dir = roundf( random::Between( ORDER_MOVE_EAST, ORDER_MOVE_SOUTH ) );
      order.order = eOrderCode( dir );
   };

   auto actionTransportFood = [&]()
   {
      int dir = roundf( random::Between( ORDER_MOVE_EAST, ORDER_MOVE_SOUTH ) );
      order.order = eOrderCode( dir );
   };

   auto actionTransportDirt = [&]() {};

   switch(agent.state) {
   case STATE_NORMAL: actionSearchFood(); break;
   case STATE_HOLDING_FOOD: actionTransportFood(); break;
   case STATE_HOLDING_DIRT: actionTransportDirt(); break;
   case STATE_DEAD:
   case NUM_AGENT_STATES:
   default:
   BAD_CODE_PATH();
   }

   return order;
}

bool PathFindingWorker::Execute()
{
   auto& state = AcquireReadCurrentState();

   std::vector<Tile> queens;
   for(int i = 0; i < state.turnState.numReports; i++) {
      auto& agentReport = state.turnState.agentReports[i];
      if(agentReport.state == STATE_DEAD)
         continue;
      if(agentReport.type == AGENT_TYPE_QUEEN) {
         queens.push_back( Tile{ agentReport.tileX, agentReport.tileY } );
      }
   }

   FinishReadCurrentState();

   mQueenMap.ResetTargets( queens );
}
