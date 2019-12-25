#define ARENA_CLIENT
#include "interface.hpp" // Provided by game master; feel free to change #include path to find this file
#include "src/agent.hpp"

static Master* gMaster;

//-----------------------------------------------------------------------------------------------
int GiveCommonInterfaceVersion() {
   return COMMON_INTERFACE_VERSION_NUMBER;
}

//-----------------------------------------------------------------------------------------------
const char* GivePlayerName() {
   // be sure to return something constant
   // and not local as the server will be using the 
   // memory directly; 
   return "Warmup try"; // your ant colony AI's name
}

//-----------------------------------------------------------------------------------------------
const char* GiveAuthorName() {
   // be sure to use something constant (see note above)
   return "Tanki Zhang"; // your (human programmer-author's) actual name
}

//-----------------------------------------------------------------------------------------------
void PreGameStartup(const StartupInfo& info) {
   gMatchInfo = info.matchInfo;
   gDebug = info.debugInterface;
   gMaster = new Master();
}

//-----------------------------------------------------------------------------------------------
void PostGameShutdown(const MatchResults& results) {
   SAFE_DELETE(gMaster);
}
//-----------------------------------------------------------------------------------------------
void PlayerThreadEntry(int threadIdx) {
   Worker* worker = gMaster->CreateWorker((eWorker)threadIdx);
   if(worker != nullptr) {
      worker->Run();
   }
}

//-----------------------------------------------------------------------------------------------
void ReceiveTurnState(const ArenaTurnStateForPlayer& state) {
   gMaster->UpdateGameState(state);
}

//-----------------------------------------------------------------------------------------------
bool TurnOrderRequest(int turnNumber, PlayerTurnOrders* ordersToFill) {
   return gMaster->SubmitOrders(ordersToFill);
}