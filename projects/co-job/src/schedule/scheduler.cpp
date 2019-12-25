#include "engine/pch.h"
#include "scheduler.hpp"
#include "cppcoro/task.hpp"
#include "cppcoro/async_manual_reset_event.hpp"
#include <fmt/color.h>
#include "engine/platform/platform.hpp"
#include "cppcoro/async_generator.hpp"

using namespace co;

class SchedulerImp;

/**
 * \brief Lock based work queue, not gonna be fast, but should be nice as the first path
 */
class JobQueue: public LockQueue<BaseJob*>
{
public:
   task<BaseJob*> Dispatch()
   {
      return Dequeue();
   }

   task<BaseJob*> Dequeue()
   {
      BaseJob* job = nullptr;
      while(!LockQueue::Dequeue( job )) {
         // failed to dequeue, so wait
         co_await mEvent;
      }
      ASSERT_DIE( job != nullptr );
      co_return job;
   }

   size_type Enqueue( BaseJob* job )
   {
      auto count = LockQueue::Enqueue( job );
      if(count == 1) {
         mEvent.set();
      }
      return count;
   }

   Event mEvent;
};


////////////////////////////////////////////////////////////////
//////////////////////////// Static ////////////////////////////
////////////////////////////////////////////////////////////////

static thread_local eWorkerThread ThreadId;
static SchedulerImp* gScheduler;

template<typename W>
static task<> StartWorker(W* worker)
{
   ASSERT_DIE( worker->Init() );
   ASSERT_DIE( co_await worker->Run() );
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
      mIsRunning.reset();
      return true;
   }
   task<bool> Run()
   {
      fmt::print( fmt::fg(fmt::color::green), "worker {} Run\n", mId );
      for co_await(BaseJob* job: Scheduler::Get().Request( mId, mIsRunning )) {
         ASSERT_DIE( job != nullptr );
         job->jobStatus = BaseJob::JOB_SCHEDULED;
         job->Run( mId );
      }
      co_return true;
   }
   bool Stop() {
      mIsRunning.set();
      fmt::print( fmt::fg(fmt::color::red),"worker {} is requested to stop\n", mId );
      return true;
   }
   bool Exit() { printf( "worker %u exit\n", mId ); return true; }

   Scheduler* mOwner;
   uint mId;
   Event mIsRunning;
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

   virtual async_generator<BaseJob*> Request( eWorkerThread id, Event& isRunning ) override
   {
      while( !isRunning.is_set() ) co_await mJobQueue.Dispatch();
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

task<> Scheduler::Wait( JobHandleArray& jobs )
{
   ASSERT_DIE( gScheduler != nullptr );

   Event waitEvent;
   Job<TriggerEventJob>::Allocate( jobs, ~0 ).InitAndDispatch( &waitEvent );
   co_await waitEvent;
}

eWorkerThread Scheduler::CurrentThreadId()
{
   return ThreadId;
}
