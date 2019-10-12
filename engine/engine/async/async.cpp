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
#include <mutex>
////////////////////////////////////////////////////////////////
//////////////////////////// Define ////////////////////////////
////////////////////////////////////////////////////////////////

class SchedulerImp;

/**
 * \brief Lock based work queue, not gonna be fast, but should be nice as the first path
 */
class JobQueue
{
public:
   void Enqueue( BaseJob* job )
   {
      std::scoped_lock lock( mQueueLock );
      mJobs.push_back( job );
   }

   BaseJob* Dispatch()
   {
      BaseJob* job;
      {
         std::scoped_lock lock( mQueueLock );
         if( mJobs.empty() ) return nullptr;
         job = mJobs.front();
         mJobs.pop_front();
         ASSERT_DIE( job->jobStatus == BaseJob::JOB_ENQUEUE );
      }

      return job;
   }

protected:
   std::deque<BaseJob*> mJobs;
   std::mutex           mQueueLock;
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
