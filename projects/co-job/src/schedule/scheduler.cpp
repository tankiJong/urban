#include "engine/pch.h"
#include "scheduler.hpp"
#include <fmt/color.h>
#include "engine/platform/platform.hpp"
#include <easy/profiler.h>

using namespace co;

static thread_local Worker* gWorkerContext = nullptr;
static thread_local Scheduler* gScheduler = nullptr;
static Scheduler* theScheduler = nullptr;
void Scheduler::Shutdown()
{
   mIsRunning.store( false, std::memory_order_relaxed );
}

bool Scheduler::IsRunning() const
{
   return mIsRunning.load(std::memory_order_relaxed);
}

Scheduler& Scheduler::Get()
{
   if(theScheduler == nullptr) {
      theScheduler = new Scheduler(8);
   }

   return *theScheduler;
}

Scheduler::~Scheduler()
{
   for(auto& workerThread: mWorkerThreads) {
      workerThread.join();
   }
}

uint Scheduler::GetThreadIndex() const
{

   return gWorkerContext ? gWorkerContext->threadId : Worker::kMainThread;
}

uint Scheduler::GetMainThreadIndex() const
{
   return Worker::kMainThread;
}

Scheduler::Scheduler( uint workerCount )
   : mWorkerCount( workerCount )
{
   ASSERT_DIE( workerCount > 0 );

   gWorkerContext = new Worker{  Worker::kMainThread };
   mWorkerThreads.reserve( workerCount );
   mWorkerContexts = std::make_unique<Worker[]>( workerCount );
   mIsRunning = true;

   mFreeWorkerCount = workerCount;
   for(uint i = 0; i < workerCount; ++i) {
      mWorkerThreads.emplace_back( [this, i] { WorkerThreadEntry( i ); } );
   }
}


void Scheduler::WorkerThreadEntry( uint threadIndex )
{
   SetThreadName( mWorkerThreads[threadIndex], fmt::format( L"co worker thread {}", threadIndex ).c_str() );

   profiler::registerThread( fmt::format( "co Job Thread {}", threadIndex ).c_str() );

   auto& context = mWorkerContexts[threadIndex];
   context.threadId = threadIndex;
   gWorkerContext = &context;
   gScheduler = this;

   while(true) {
      Operation* op = FetchNextJob();
      if(op == nullptr) {
         std::this_thread::yield();
      } else {
         mFreeWorkerCount--;

         {
            op->Resume();
         }
         if(op->Done()) {
            EXPECTS( op->Promise()->mState == eOpState::Done );
            ReleaseOp( op );
         } else {
            // this got suspended, it will be resumed in the await_suspended
            EXPECTS( op->Promise()->mState == eOpState::Suspended );
            // BAD_CODE_PATH();
            // op->RescheduleOp( *this );
         }

         mFreeWorkerCount++;
      }

      if( !IsRunning() ) break;
   }
}

void Scheduler::WorkerThreadyEntry( const SysEvent& exitSignal )
{
   // this path only will run when it's blocked by something,
   // so instead, it will try to run something else at the same time.
   // In that sense, we need to first register itself as a free worker
   mFreeWorkerCount++;

   while( true ) {
      Operation* op = FetchNextJob();
      if( op == nullptr ) {
         std::this_thread::yield();
      }
      else {
         mFreeWorkerCount--;

         {
            op->Resume();
         }
         if( op->Done() ) {
            EXPECTS( op->Promise()->mState == eOpState::Done );
            ReleaseOp( op );
         }
         else {
            // this got suspended, it will be resumed in the await_suspended
            EXPECTS( op->Promise()->mState == eOpState::Suspended );
            // BAD_CODE_PATH();
            // op->RescheduleOp( *this );
         }

         mFreeWorkerCount++;
      }

      if( exitSignal.IsTriggered() ) break;
   }

   mFreeWorkerCount--;
}

Scheduler::Operation* Scheduler::FetchNextJob()
{
   Operation* op = nullptr;
   mJobs.Dequeue( op );
   return op;
}

void Scheduler::EnqueueJob( Operation* op )
{
   mJobs.Enqueue( op );
}
