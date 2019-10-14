#include "engine/pch.h"
#include "async.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <easy/profiler.h>
#include <fmt/color.h>
#include <fmt/printf.h>
#include "types.hpp"
#include "semantics.hpp"
#include "engine/platform/platform.hpp"

////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

class SchedulerImp;

/**
 * \brief Lock based work queue, not gonna be fast, but should be nice as the first path
 */
class JobQueue: public LockQueue<BaseJob*>
{
public:
   BaseJob* Dispatch()
   {
      BaseJob* job;
      if(!Dequeue(job)) job = nullptr;
      return job;
   }
};


////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

static thread_local eWorkerThread ThreadId;
static SchedulerImp* gScheduler;

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

LinearBuffer<1 GB> BaseJob::sBuffer;


class Worker
{
public:
   Worker(Scheduler* owner, uint id): mOwner( owner ), mId( id ) {}
   bool Init()
   {
      ThreadId = mId;
      profiler::registerThread( fmt::format( "Job Thread {}", mId ).c_str() );
      print( fg( fmt::color::blue ), "worker {} init\n", mId );
      return true;
   }
   bool Run()
   {
      fmt::print( fmt::fg(fmt::color::green), "worker {} Run\n", mId );
      while( mIsRunning ) {
         BaseJob* job = Scheduler::Get().Request( mId );
         if(job == nullptr) {
            std::this_thread::yield();
         } else {
            job->jobStatus = BaseJob::JOB_SCHEDULED;
            job->Run( mId );
            
         }
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
   SchedulerImp() { ASSERT_DIE( gScheduler == nullptr ); }
   void Start(uint workerCount)
   {
      mWorkers.reserve( workerCount );
      mWorkerThreads.reserve( workerCount );

      ThreadId = 0;

      for(uint i = 1; i <= workerCount; i++) {
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

   virtual void Issue(BaseJob* job) override
   {
      mJobQueue.Enqueue( job );
   }

   virtual BaseJob* Request( eWorkerThread id ) override
   {
      BaseJob* job = mJobQueue.Dispatch();
      return job;
   }

   std::vector<std::thread> mWorkerThreads;
   std::vector<Worker*> mWorkers;
   JobQueue mJobQueue;
};



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

void Scheduler::Wait( JobHandleArray& jobs )
{
   ASSERT_DIE( gScheduler != nullptr );

   SysEvent waitEvent;
   Job<TriggerEventJob>::Allocate( jobs, ~0 ).InitAndDispatch( &waitEvent );
   waitEvent.Wait();
}

eWorkerThread Scheduler::CurrentThreadId()
{
   return ThreadId;
}
