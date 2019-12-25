Ant Arena II 
======

## Terms
- **Arena**, **Server** - The executable running the game logic. 
- **Player**, **Client**, **Colony** - An AI DLL being loaded into the arena. 
- **Map**: Game area made up of tiles and food.
- **Match**:  A fully played out simulation.
- **Turn**:  A single simultaneous round of mechanics; see *Turn Order* below. 
- **Agent**, **Ant**:  A single entity in the simulation.
- **Nutrients**:  Resource for each colony, used to birth and feed (upkeep) ants
- **Upkeep**:  Each living ant adds to a colony upkeep, which is deducted from **Nutrients** at the end of a turn before *Starvation* is resolved. 
- **Sudden Death**:  A point in the match after which food stops spawning and an ever-increasing additional colony upkeep burden is applied.
- **Starvation**: When a colony ends a turn with fewer than 0 nutrients, it is considered starving, and will pay a starvation penalty (see *Starvation Resolution* below).


## Folder Structure
- `./Players` - Player DLLs in this folder will be pitted against each other when the game starts.  Do not submit DLLs to Perforce from here!
- `./Storehouse` - Where player dlls should be checked in.  Players will be copied from here into *Players* before a match starts.
- `./Data/AgentDefinitions.xml` - Game rules exposed through data.  Should be fairly static unless balance changes are needed.  *may be edited locally, temporarily, for testing/debugging purposes, but do not submit changes*
- `./Data/MapDefinitions.xml` - Defines map types that can be played.
- `./Data/MatchDefinitions.xml` - Defines match rules for specific match types.
- `./GameConfig.txt` - Controls general settings for the Arena executable itself.


## The Interface
The common interface, located at `../Code/Game/ArenaPlayerInterface.hpp`, defines all client-server interactions.  This header file is #included by both the server and clients, and the COMMON_INTERFACE_VERSION_NUMBER defined inside it determines whether a client is compiled with the same version of this file as (and is compatible with) the server.

### DLL Functions Called Immediately By The Server
- `GiveCommonInterfaceVersion`: Called immediately, and must report the COMMON_INTERFACE_VERSION_NUMBER version the DLL was compiled with.  If this doesn't match the server's version, the DLL may be rejected and evicted.
- `GivePlayerName`: Pass back a `char` buffer containing the player's name.  This can be whatever ASCII name you prefer (within reason).
- `GiveAuthorName`: Pass back a `char` buffer containing the author of the player AI's name.  This should be your actual name. 

### DLL Functions Called By The Server During The Match
From the clients' point of view, this is how the Arena server calls into the interface:

```cpp
   // start up the game
   foreach (player) {
      player->PreGameStartup( match_info );
      for( int threadIdx = 0; threadIdx < threadsPerPlayer; ++threadIdx ){
         player->PlayerThreadEntry( threadIdx ); 
      }
   } 

   while ( /*Match is running*/ ) {
      foreach (player) {
         // send turn state to players
         player->ReceiveTurnState( turnState );
      }

      // ...a brief amount time passes...

      foreach (player) {
         while (!RequestTurnOrder( turnNumber, &orders )) {}
      }

      // Run a turn (execute and resolve all player ant orders simultaneously)
      // ...
   }

   // inform each player how the match ended; 
   foreach (player) {
      player->PostGameShutdown( matchresults ); 
   }
```

*ToDo: Currently the interace may call `GiveCommonInterfaceVersionn`, `GivePlayerName`, and `GiveAuthorName` multiple times during setup.  This should eventually only be called once when client duplication is finalized.*

## Starvation
On any turn in which a player colony cannot pay its required nutrient upkeep (the sum of its ants' upkeep costs, plus possible sudden death upkeep burden), it is considered **Starving**.  One of the colony's ants will be killed by the server (AGENT_KILLED_BY_STARVATION, chosen based on the `sacrificePriority` defined for that agent type in AgentDefinitions.xml) each turn as long as this condition continues.  *If at least one ant was voluntarily sacrificed by the colony that turn via ORDER_SUICIDE, no additional ant will be starved that turn.*

## Losing
Once a colony has no remaining queens alive, its nutrients supply drops to zero. 

Once a colony has no remaining ants alive, it is dead (removed from the match).
*In effect this means a queen-less colony goes into a permanent state of starvation, and one remaining ant per turn will die by starvation rules until no ants remain.*

## FAQ
...
