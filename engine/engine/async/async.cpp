#include "engine/pch.h"
#include "async.hpp"
#include "engine/platform/platform.hpp"
#include "engine/core/string.hpp"
#include <thread>
#include <atomic>
#include <deque>
#include <easy/profiler.h>
#include <fmt/color.h>
#include <fmt/printf.h>
////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

class SchedulerImp;

/**
 * \brief Lock based work queue, not gonna be fast, but should be nice as the first path
 */
class JobQueue
{

   std::deque<BaseJob*> mJobs;
};


////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

template<typename W>
static void StartWorker(W* worker)
{
   ASSERT_DIE( worker->Init() );
   ASSERT_DIE( worker->Run() );
   ASSERT_DIE( worker->Exit() );
}

////////////////////////////////////////////////////////////////
/////////////////////// Standalone Function ////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
///////////////////////// Member Function //////////////////////
////////////////////////////////////////////////////////////////



class Worker
{
public:
   Worker(Scheduler* owner, uint id): mOwner( owner ), mId( id ) {}
   bool Init()
   {
      profiler::registerThread( fmt::format( "Job Thread {}", mId ).c_str() );

      fmt::print( fmt::fg( fmt::color::blue ), "worker {} init\n", mId );
      return true;
   }
   bool Run()
   {
      using namespace std::chrono_literals;
      fmt::print( fmt::fg(fmt::color::green), "worker {} Run\n", mId );
      while( mIsRunning ) {
         std::this_thread::sleep_for( 1000ms );
      }
      return true;
   }
   bool Stop() {
      mIsRunning = false;
      fmt::print( fmt::fg(fmt::color::red),"worker {} is requested to stop\n", mId );
      return true;
   }
   bool Exit() { printf( "worker %u exit\n", mId ); return true; }

   Scheduler* mOwner;
   uint mId;
   std::atomic<bool> mIsRunning = true;
};


class SchedulerImp: public Scheduler
{
public:
   void Start(uint workerCount)
   {
      mWorkers.reserve( workerCount );
      mWorkerThreads.reserve( workerCount );

      for(uint i = 0; i < workerCount; i++) {
         mWorkers.emplace_back( new Worker(this, i) );
         mWorkerThreads.emplace_back( StartWorker<Worker>, mWorkers.back());
         SetThreadName( mWorkerThreads.back(), fmt::format( L"Job Thread {}", i ).c_str() );
      }
   };
   void Terminate()
   {
      for(auto& worker: mWorkers) {
         worker->Stop();
      }
      for(auto& workerThread: mWorkerThreads) {
         workerThread.join();
      }

      for(auto& worker: mWorkers) {
         SAFE_DELETE( worker );
      }
   }
   virtual void Issue(BaseJob* job)
   {
      
   }
   std::vector<std::thread> mWorkerThreads;
   std::vector<Worker*> mWorkers;
};




SchedulerImp* gScheduler;

void Scheduler::Init()
{
   gScheduler = new SchedulerImp();
   gScheduler->Start( QuerySystemCoreCount() );
}
void Scheduler::Cleanup()
{
   gScheduler->Terminate();
}
Scheduler& Scheduler::Get() { return *gScheduler; }
