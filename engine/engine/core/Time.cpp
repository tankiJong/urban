#include "engine/pch.h"
#include "Time.hpp"
#include "engine/platform/win.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

struct TimeInfo {
   TimeInfo();
   uint64_t freq;
   double secPerCount;
};

////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

static TimeInfo gTimeInfo;
static Clock    gClock;

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

double InitializeTime( LARGE_INTEGER& out_initialTime )
{
	LARGE_INTEGER countsPerSecond;
	QueryPerformanceFrequency( &countsPerSecond );
	QueryPerformanceCounter( &out_initialTime );
	return( 1.0 / static_cast< double >( countsPerSecond.QuadPart ) );
}

double GetCurrentTimeSeconds()
{
   static LARGE_INTEGER initialTime;
	static double secondsPerCount = InitializeTime( initialTime );
	LARGE_INTEGER currentCount;
	QueryPerformanceCounter( &currentCount );
	LONGLONG elapsedCountsSinceInitialTime = currentCount.QuadPart - initialTime.QuadPart;

	double currentSeconds = static_cast< double >( elapsedCountsSinceInitialTime ) * secondsPerCount;
	return currentSeconds;
};

uint64_t __fastcall GetPerformanceCounter() {
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);

  return *(uint64_t*)&li;
}

double PerformanceCountToSecond(uint64_t count) {
  return (double)count * gTimeInfo.secPerCount;
}


////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////

TimeInfo::TimeInfo()
{
   LARGE_INTEGER li;
   QueryPerformanceFrequency( &li );
   freq        = *(uint64_t*)&li;
   secPerCount = 1.0 / (double)freq;
}

Time Time::Current()
{
   Time time;

   time.second = GetCurrentTimeSeconds();
   time.hpc = GetPerformanceCounter();
   time.millisecond = uint32_t(time.second * 1000);

   return time;
}

void Clock::Forward( hpc_t elapsed )
{
   ++frameCount;

   double elapsedSec = PerformanceCountToSecond( elapsed );

   frame.second = elapsedSec;
   frame.hpc = elapsed;
   frame.millisecond = millisecond_t(frame.second * 1000);

   total += frame;
}

void Clock::Forward()
{
   Forward( GetPerformanceCounter() - total.hpc );
}

Clock& Clock::Main()
{
   return gClock;
}
