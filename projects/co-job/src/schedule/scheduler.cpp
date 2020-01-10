#include "engine/pch.h"
#include "scheduler.hpp"
#include <fmt/color.h>
#include "engine/platform/platform.hpp"

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
   return gWorkerContext->mThreadIndex;
}

Scheduler::Scheduler( uint workerCount )
   : mWorkerCount( workerCount )
{
   ASSERT_DIE( workerCount > 0 );

   gWorkerContext = new Worker{ 0xff };
   mWorkerThreads.reserve( workerCount );
   mWorkerContexts = std::make_unique<Worker[]>( workerCount );
   mIsRunning = true;

   for(uint i = 0; i < workerCount; ++i) {
      mWorkerThreads.emplace_back( [this, i] { WorkerThreadEntry( i ); } );
   }
}


void Scheduler::WorkerThreadEntry( uint threadIndex )
{
   SetThreadName( mWorkerThreads[threadIndex], fmt::format( L"co worker thread {}", threadIndex ).c_str() );
   auto& context = mWorkerContexts[threadIndex];
   context.mThreadIndex = threadIndex;
   gWorkerContext = &context;
   gScheduler = this;

   while(true) {
      S<Operation> op = FetchNextJob();
      if(op == nullptr) {
         std::this_thread::yield();
      } else {
         static std::atomic_int jobID;
         op->mState.store( eOpState::Processing );
         int currentid;
         currentid = jobID.fetch_add( 1 );
         printf( "run job %i on Thread %u\n", currentid, threadIndex );
         op->resume();
         op->mState.store( eOpState::Done );
      }

      if( !IsRunning() ) break;
   }
}

S<Scheduler::Operation> Scheduler::FetchNextJob()
{
   S<Operation> op;
   mJobs.Dequeue( op );
   return op;
}

void Scheduler::EnqueueJob( const S<Operation>& op )
{
   op->mState.store( eOpState::Scheduled );
   mJobs.Enqueue( op );
}
