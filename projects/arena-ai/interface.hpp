//-----------------------------------------------------------------------------------------------
// ArenaPlayerInterface.hpp
//
// Common interface used by both the Arena Server and by each Arena Player (DLL).
// Defines enums, types, structures, and functions necessary to communicate with Arena Server.
//
#pragma once

//-----------------------------------------------------------------------------------------------
// Revision History
// 1: DLL Interface created
// 2: Separated combatPriority and sacrificePriority
// 3: Assorted changes made due to logic going in - mostly error codes 
// 4: Added nutrients lost due to faults to the report
// 5: Removed sand and gravel tile types (as well as STATE_HOLDING_SAND, etc.)
// 6: Queen suffocation penalty added
// 7: Sudden death nutrient upkeep added
// 8: New agent order for failed move due to queens sharing tile
// 9: Added a RequestPauseAfterTurn() to the debug interface; 
// 10: changed eOrderCode storage type to unsigned char (was default-int)
// 11: Removed some redudnant order result codes, and added nutrient loss information to given stats; 
// 11.1: Added new emotes - does not break old version, so not updating version
// 12: Exposed SetMoodText() function in DebugInterface struct; changes one-line player emote text
//-----------------------------------------------------------------------------------------------
constexpr int	COMMON_INTERFACE_VERSION_NUMBER=12;


//-----------------------------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------------------------
#if defined( ARENA_SERVER )
#define DLL __declspec( dllimport )
#else // ARENA_PLAYER
#define DLL __declspec( dllexport )
#endif

//-----------------------------------------------------------------------------------------------
// Common Typedefs
//-----------------------------------------------------------------------------------------------
typedef unsigned char	TeamID;		// 200+
typedef unsigned char	PlayerID;	// 100+
typedef unsigned int	AgentID;	// unique per agent, highest byte is agent's owning playerID


//-----------------------------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------------------------

// Hard limit - actual limits will be provided during
// startup, and may (typically) be much lower
constexpr short	MAX_ARENA_WIDTH=256;
constexpr int	MAX_ARENA_TILES=(MAX_ARENA_WIDTH * MAX_ARENA_WIDTH);
constexpr char	MAX_PLAYERS=32;
constexpr char	MAX_TEAMS=MAX_PLAYERS;
constexpr char	MAX_PLAYERS_PER_TEAM=MAX_PLAYERS;

constexpr int	MAX_AGENTS_PER_PLAYER=256;
constexpr int	MAX_ORDERS_PER_PLAYER=MAX_AGENTS_PER_PLAYER;
constexpr int	MAX_REPORTS_PER_PLAYER=2 * MAX_AGENTS_PER_PLAYER;
constexpr int	MAX_AGENTS_TOTAL=(MAX_PLAYERS * MAX_AGENTS_PER_PLAYER);

// special penalty values for digging/moving
constexpr int DIG_IMPOSSIBLE=-1;
constexpr int TILE_IMPASSABLE=-1;

//-----------------------------------------------------------------------------------------------
// Enums
//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
enum eFaultType: unsigned char {
   FAULT_NONE=0,
   FAULT_TURN_START_DURATION_ELAPSED,
   FAULT_ORDER_FETCH_DURATION_ELAPSED,
   FAULT_TOTAL_TURN_DURATION_ELAPSED,
   FAULT_INVALID_ORDER,
   FAULT_DOUBLE_ORDERED_AGENT,
   FAULT_DID_NOT_OWN_AGENT,
   FAULT_INVALID_AGENT_ID,
};

//-----------------------------------------------------------------------------------------------
enum eAgentType: unsigned char {
   AGENT_TYPE_SCOUT,
   AGENT_TYPE_WORKER,
   AGENT_TYPE_SOLDIER,
   AGENT_TYPE_QUEEN,

   NUM_AGENT_TYPES,
   INVALID_AGENT_TYPE=0xff
};

//-----------------------------------------------------------------------------------------------
enum eOrderCode: unsigned char {
   ORDER_HOLD=0,

   ORDER_MOVE_EAST,
   ORDER_MOVE_NORTH,
   ORDER_MOVE_WEST,
   ORDER_MOVE_SOUTH,

   ORDER_DIG_HERE, // exception: does not cause dig exhaustion!
   ORDER_DIG_EAST,
   ORDER_DIG_NORTH,
   ORDER_DIG_WEST,
   ORDER_DIG_SOUTH,

   ORDER_PICK_UP_FOOD,
   ORDER_PICK_UP_TILE,
   ORDER_DROP_CARRIED_OBJECT,

   ORDER_BIRTH_SCOUT,
   ORDER_BIRTH_WORKER,
   ORDER_BIRTH_SOLDIER,
   ORDER_BIRTH_QUEEN,

   ORDER_SUICIDE,

   // EMOTES - To be implemented given infinite time
   ORDER_EMOTE_HAPPY,
   ORDER_EMOTE_SAD,
   ORDER_EMOTE_ANGRY,
   ORDER_EMOTE_TAUNT,
   ORDER_EMOTE_DEPRESSED,
   ORDER_EMOTE_CONFUSED,
   ORDER_EMOTE_SCARED,
   ORDER_EMOTE_ASTONISHED,

   NUM_ORDERS
};

//-----------------------------------------------------------------------------------------------
enum eAgentOrderResult: unsigned char {
   // events
   AGENT_WAS_CREATED,
   AGENT_KILLED_BY_ENEMY,
   AGENT_KILLED_BY_WATER,
   AGENT_KILLED_BY_SUFFOCATION,
   AGENT_KILLED_BY_STARVATION,
   AGENT_KILLED_BY_PENALTY,

   // responses
   AGENT_ORDER_SUCCESS_HELD,
   AGENT_ORDER_SUCCESS_MOVED,
   AGENT_ORDER_SUCCESS_DUG,
   AGENT_ORDER_SUCCESS_PICKUP,
   AGENT_ORDER_SUCCESS_DROP,
   AGENT_ORDER_SUCCESS_GAVE_BIRTH,	// unit spawned another unit
   AGENT_ORDER_SUCCESS_SUICIDE,

   AGENT_ORDER_ERROR_BAD_ANT_ID,
   AGENT_ORDER_ERROR_EXHAUSTED,
   AGENT_ORDER_ERROR_CANT_CARRY_FOOD,
   AGENT_ORDER_ERROR_CANT_CARRY_TILE,
   AGENT_ORDER_ERROR_CANT_BIRTH,
   AGENT_ORDER_ERROR_CANT_DIG_INVALID_TILE,
   AGENT_ORDER_ERROR_CANT_DIG_WHILE_CARRYING,
   AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE,
   AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_QUEEN,	// queens can't occupy tiles with other queens
   AGENT_ORDER_ERROR_OUT_OF_BOUNDS,
   AGENT_ORDER_ERROR_NO_FOOD_PRESENT,
   AGENT_ORDER_ERROR_ALREADY_CARRYING_FOOD,
   AGENT_ORDER_ERROR_NOT_CARRYING,
   AGENT_ORDER_ERROR_INSUFFICIENT_FOOD,
   AGENT_ORDER_ERROR_MAXIMUM_POPULATION_REACHED,

   NUM_AGENT_STATUSES
};

//-----------------------------------------------------------------------------------------------
enum eAgentState: unsigned char {
   STATE_NORMAL=0,
   STATE_DEAD,
   STATE_HOLDING_FOOD,
   STATE_HOLDING_DIRT,

   NUM_AGENT_STATES
};


//-----------------------------------------------------------------------------------------------
enum eTileType: unsigned char {
   TILE_TYPE_AIR,				// open space, traversable by most/all agent types

   TILE_TYPE_DIRT,				// semi-solid; may be dug or traversed by certain ant types
   TILE_TYPE_STONE,			// impassable and indestructible

   TILE_TYPE_WATER,			// lethal; turns to a corpse bridge when ant walks onto it, killing the ant
   TILE_TYPE_CORPSE_BRIDGE,	// open space (like AIR), but can be dug to revert it back to water

   NUM_TILE_TYPES,
   TILE_TYPE_UNSEEN=0xff		// not currently visible to any ant, e.g. obscured by fog of war
};

// predeclared types;
struct DebugInterface;

//------------------------------------------------------------------------------------------------
// Structures
//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
struct Color8 {
   Color8()
      : r(0)
      , g(0)
      , b(0)
      , a(255) {}

   Color8(unsigned char redByte,
          unsigned char greenByte,
          unsigned char blueByte,
          unsigned char alphaByte=255)
      : r(redByte)
      , g(greenByte)
      , b(blueByte)
      , a(alphaByte) {}

   unsigned char r, g, b, a;
};


//-----------------------------------------------------------------------------------------------
struct VertexPC {
   float x, y;
   Color8 rgba;
};


//-----------------------------------------------------------------------------------------------
struct AgentTypeInfo // information about a given agent type (e.g. AGENT_TYPE_WORKER)
{
   const char* name;			// proper-case name for this type, e.g. "Worker" or "Queen"
   int		costToBirth;		// colony pays this many nutrients for a queen to birth one of these
   int		exhaustAfterBirth;	// queen who birthed an agent of this type gains +exhaustion
   int		upkeepPerTurn;		// colony pays this many nutrients per turn per one of these alive
   int		visibilityRange;	// sees tiles and agents up to this far away (taxicab distance)
   int		combatStrength;		// survives a duel if enemy agent has a lower combatStrength
   int		combatPriority;		// types with highest priority fight first
   int		sacrificePriority;	// types with highest priority are killed first when a sacrifice is required (water bridge or starvation)
   bool	canCarryFood;		// can pick up & drop food / porous tiles (ORDER_PICK_UP_, ORDER_DROP_)
   bool	canCarryTiles;		// can pick up & drop food / porous tiles (ORDER_PICK_UP_, ORDER_DROP_)
   bool	canBirth;			// can birth other ants (ORDER_BIRTH_)

   int		moveExhaustPenalties[NUM_TILE_TYPES]; // exhaustion gained after moving into each tile type
   int		digExhaustPenalties[NUM_TILE_TYPES];  // exhaustion gained after remote-digging each tile type
};


//-----------------------------------------------------------------------------------------------
struct MatchInfo // information about the match about to be played
{
   int numPlayers;		// number of players (with unique PlayerIDs) in this match
   int numTeams;		// <numPlayers> in brawl, 2 for 5v5, 1 for co-op survival

   short mapWidth;		// width & height of [always square] map; tileX,tileY < mapWidth

   bool fogOfWar;				// if false, all tiles & agents are always visible
   bool teamSharedVision;		// if true, teammates share combined visibility
   bool teamSharedResources;	// if true, teammates share a single combined "nutrients" score

   int nutrientsEarnedPerFoodEatenByQueen;	// when a queen moves onto food, or food drops on her
   int nutrientLossPerAttackerStrength;	// colony loses nutrients whenever queen is attacked
   int nutrientLossForQueenSuffocation;	// when queen is suffocated - how much food damage she takes
   int numTurnsBeforeSuddenDeath;			// no new food appears after this turn, and upkeep increases
   int suddenDeathTurnsPerUpkeepIncrease;	// total upkeep increases by +1 every N turns after S.D.
   int colonyMaxPopulation;				// cannot birth new agents if population is at max
   int startingNutrients;					// each colony starts with this many nutrients

   int foodCarryExhaustPenalty;	// exhaustion gained after each move while carrying food
   int tileCarryExhaustPenalty;	// exhaustion gained after each move while carrying a porous tile

   int combatStrengthQueenAuraBonus;		// agents may gain +combatStrength near a friendly queen
   int combatStrengthQueenAuraDistance;	// num tiles away (taxicab) a queen's bonus extends

   AgentTypeInfo agentTypeInfos[NUM_AGENT_TYPES];	// stats and capabilities for each agent type
};


//------------------------------------------------------------------------------------------------
struct PlayerInfo // server-assigned information about your Player instance in this match context
{
   PlayerID		playerID;	// 100+
   TeamID			teamID;		// 200+
   unsigned char	teamSize;	// 1 in free-for-all, 5 in 5v5, <numPlayers> in co-op survival
   Color8			color;		// use this in your debug drawing, etc.
};


//-----------------------------------------------------------------------------------------------
// Structure given for each of your agents (and/or each of your orders just previously issued)
//
struct AgentReport {
   AgentID				agentID;		// your agent's unique ID #

   short				tileX;			// current tile coordinates in map; (0,0) is bottom-left
   short				tileY;
   short				exhaustion;		// number of turns inactive; non-HOLD orders fail if > 0

   short				receivedCombatDamage; // amount of damage received this turn 
   short				receivedSuffocationDamage; // suffocation damage received this turn (1 is you suffocated)

   eAgentType			type;			// type of agent (permanent/unique per agent)
   eAgentState			state;			// special status of agent (carrying something, etc.)
   eAgentOrderResult	result;			// result of agent's previously issued order
};

// -----------------------------------------------------------------------------------------------
struct ObservedAgent {
   AgentID		agentID;			// another Player's agent's unique ID #
   PlayerID	playerID;			// Player who owns this agent
   TeamID		teamID;				// Team the agent's Player is on

   short		tileX;				// just observed at these tile coordinates; (0,0) is bottom-left
   short		tileY;

   short		receivedCombatDamage; // 0 or 1 for most agents, nutrient damage for queen
   short		receivedSuffocationDamage; // 0 or 1 for most agents, nutrient damage for queen

   eAgentType	type;				// observed agent's type
   eAgentState	state;				// special status of agent (carrying something, etc.)
   eOrderCode	lastObservedAction;	// what this agent just did / was trying to do (last orders)
};


//------------------------------------------------------------------------------------------------
// An order you issue for one of your agents.
// Issuing more than one order for a given agent in the same turn is an illegal FAULT.
struct AgentOrder {
   AgentID agentID;
   eOrderCode order;
};

//------------------------------------------------------------------------------------------------
// Fill this out when requested by TurnOrderRequest() to tell the server what actions each of
//	your agents is taking this turn.
//
// Orders [0] through [numberOfOrders-1] should each be a valid order for a valid, unique AgentID
struct PlayerTurnOrders {
   AgentOrder orders[MAX_ORDERS_PER_PLAYER];
   int numberOfOrders;
};


//------------------------------------------------------------------------------------------------
// Information about the server and match provided when PreGameStartup() is called by the server.
//
typedef void (*EventFunc)(const char* line);
typedef void (*RegisterEventFunc)(const char* eventName, EventFunc func);
struct StartupInfo {
   MatchInfo	matchInfo;			// info about the match itself (numPlayers, mapWidth, etc.)
   PlayerInfo	yourPlayerInfo;		// info about your Player instance in this match context

   int expectedThreadCount;		// how many calls to PlayerThreadEntry() you will get (minimum of 1, typically 2+)
   double	maxTurnSeconds;			// must respond to ReceiveTurnState/TurnOrderRequest within this period
   int freeFaultCount;				// illegal faults allowed before Player is ejected (default 0)
   int nutrientPenaltyPerFault;	// nutrients lost per illegal fault committed (default infinity)
   int agentsKilledPerFault;		// agents kills as punishment per illegal fault (default infinity)

   DebugInterface* debugInterface; // debug draw/log function interface for your Player to use

   RegisterEventFunc RegisterEvent; // call this during PreGameStartup to register event & console commands

   // #ToDo: provide a pre-allocated fixed memory pool that AI can (must?) use for "heap" allocations
   // void*	memoryPool;
   // size_t	memoryPoolSizeInBytes; 
};


// -----------------------------------------------------------------------------------------------
struct ArenaTurnStateForPlayer {
   // Information at the start of this turn; your next PlayerTurnOrders will be for this turn number
   int turnNumber;
   int currentNutrients;

   // fault reporting to clients
   int numFaults;
   int nutrientsLostDueToFault;
   int nutrientsLostDueToQueenDamage;
   int nutrientsLostDueToQueenSuffocation;

   // Status reports for each of your living agents (and/or for each order you submitted)
   AgentReport agentReports[MAX_REPORTS_PER_PLAYER];
   int numReports;

   // List of all agents (besides yours) within your current visibility
   ObservedAgent observedAgents[MAX_AGENTS_TOTAL];
   int numObservedAgents;

   // Copy of the entire map (mapWidth*mapWidth entries!); tile type is TILE_TYPE_UNSEEN for
   //	tiles not within your current visibility.  Unseen tiles always report "false" for food.
   eTileType observedTiles[MAX_ARENA_TILES];
   bool tilesThatHaveFood[MAX_ARENA_TILES];
};

// -----------------------------------------------------------------------------------------------
struct MatchResults {};


//------------------------------------------------------------------------------------------------
// Debug Render Interface
//
// Call debugInterface->LogText() to printf colored text to the dev console and log file
// Call debugInterface->QueueXXX() functions to request asynchronous debug draws on the server;
// Call debugInterface->FlushXXX() when finished; server will thereafter draw all queued items,
//	and continue to do so until your next call to FlushXXX(), which clears the previous items.
// !!! Items Queued for drawing will not appear until a Flush has been called afterwards. !!!
//
// Debug drawing is done in world space; each Tile is 1x1, and (x,y) is the center of tile X,Y.
// For example, tile 7,3 extends from mins(6.5,2.5) to maxs(7.5,3.5), with center at (7.0,3.0).
// All drawing is clipped to world space, i.e. mins(-.5,-.5) to maxs(mapWidth-.5,mapWidth-.5).
//-----------------------------------------------------------------------------------------------
typedef void (*RequestPauseFunc)();
typedef void (*LogTextFunc)(char const* format, ...);
typedef void (*SetMoodTextFunc)(char const* format, ...);
typedef void (*DrawVertexArrayFunc)(int count, const VertexPC* vertices);
typedef void (*DrawWorldTextFunc)(
   float posX, float posY,
   float anchorU, float anchorV,
   float height, // 1.0 would be text whose characters are one tile high
   Color8 color,
   char const* format, ...);
typedef void (*FlushQueuedDrawsFunc)();
//------------------------------------------------------------------------------------------------
struct DebugInterface {
   RequestPauseFunc		RequestPause;			// Pause the simulation (can be ignored by game)
   LogTextFunc				LogText;				// Print to dev console (and possibly log file)
   SetMoodTextFunc			SetMoodText;			// Call anytime to change "mood" text on player panel

   DrawWorldTextFunc		QueueDrawWorldText;		// Draw (aligned) overlay text in world space
   DrawVertexArrayFunc		QueueDrawVertexArray;	// Draw untextured geometry in world space
   FlushQueuedDrawsFunc	FlushQueuedDraws;		// Call after queuing to commit and show draws
};


//-----------------------------------------------------------------------------------------------
// DLL-EXE Interface
//
// All of these functions are exported by the DLL so that the server can find them using
//	LoadLibrary() and GetProcAddress() and call them.
//
// All functions should return immediately, except PlayerThreadEntry() which is called from
//	within a private thread created by the server solely for this DLL to use to do asynchronous
//	work.  PlayerThreadEntry() should loop infinitely, doing general AI work for your ant colony,
//	until the server calls PostGameShutdown(), after which you should exit your loop and finally
//	return from PlayerThreadEntry().  Note that PlayerThreadEntry() may (likely) be called 2+
//	times, once for/in each thread created for this DLL player by the server.
//-----------------------------------------------------------------------------------------------
#if !defined(ARENA_SERVER)
   // Functions exported by the DLL for the server (.EXE) to call
extern "C"
{
   // info collection
   DLL int GiveCommonInterfaceVersion();	// DLL should return COMMON_INTERFACE_VERSION_NUMBER
   DLL const char* GivePlayerName();		// DLL should return the name of the AI (can be whatever)
   DLL const char* GiveAuthorName();		// DLL should return the actual human author's name

   // setup
   DLL void PreGameStartup(const StartupInfo& info);			// Server provides player/match info
   DLL void PostGameShutdown(const MatchResults& results);	// Server signals match end; exit loops
   DLL void PlayerThreadEntry(int yourThreadIdx);			// Called in its own private thread (yours!); infinitely loop, doing async AI work, until PostGame is called

   // Turn
   DLL void ReceiveTurnState(const ArenaTurnStateForPlayer& state);				// Server tells you what you see
   DLL bool TurnOrderRequest(int turnNumber, PlayerTurnOrders* ordersToFill);	// You tell server what you do
}
#else
   // Function pointer types, used by server (only) find these exported DLL functions via GetProcAddress()
typedef int (*GiveCommandInterfaceVersionFunc)();
typedef const char* (*GivePlayerNameFunc)();
typedef const char* (*GiveAuthorNameFunc)();
typedef void (*PreGameStartupFunc)(const StartupInfo& info);
typedef void (*PostGameShutdownFunc)(const MatchResults& results);
typedef void (*PlayerThreadEntryFunc)(int yourThreadIdx);
typedef void (*ReceiveTurnStateFunc)(const ArenaTurnStateForPlayer& state);
typedef bool (*TurnOrderRequestFunc)(int turnNumber, PlayerTurnOrders* ordersToFill);
#endif
