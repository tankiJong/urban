#pragma once
#include <atomic>
#include "types.hpp"
#include "semantics.hpp"
#include <easy/profiler.h>
#include "engine/core/memory.hpp"

using eWorkerThread = uint;

struct BaseJob;
using JobHandleRef = S<class JobHandle>;
using JobHandleArray = std::vector<JobHandleRef>;
using JobList = std::vector<BaseJob*>;

class Scheduler
{
   friend struct BaseJob;

public:
   virtual void      Issue( BaseJob* job ) = 0;
   virtual BaseJob*  Request( eWorkerThread id ) = 0;
   static void       Init();
   static void       Cleanup();
   static Scheduler& Get();
   static void       Wait( JobHandleArray& jobs );
   static eWorkerThread CurrentThreadId();
};

struct BaseJob
{
   friend class Scheduler;

   enum eJobStatus
   {
      JOB_CANCELED = -1,
      JOB_CREATED,
      JOB_SETUP,
      JOB_ENQUEUE,
      JOB_SCHEDULED,
      JOB_EXECUTING,
      JOB_DONE,
   };

   static constexpr uint kMaxSubsequentCount = 1024;

   BaseJob( uint subsequentCount ): mSubsequentCount( subsequentCount ) {}
   ~BaseJob() = default;
   virtual void Run( eWorkerThread threadId ) = 0;

   void DecrementAndTryIssue()
   {
      --mSubsequentCount;
      if(mSubsequentCount.load() == 0) { Issue(); }
   }

   eJobStatus jobStatus = JOB_CREATED;
protected:
   void Issue()
   {
      ASSERT_DIE( jobStatus == JOB_SETUP );
      Scheduler::Get().Issue( this );
      jobStatus = JOB_ENQUEUE;
   }

   void CompletePresequents( uint preseqCount )
   {
      ASSERT_DIE( jobStatus == JOB_SETUP );
      uint prevCount = mSubsequentCount.fetch_sub( preseqCount );
      if(prevCount == preseqCount) { Issue(); }
   }

   std::atomic<uint> mSubsequentCount = 0;

   static LinearBuffer<1 GB> sBuffer;
};

class JobHandle
{
public:
   bool AddSubsequent( BaseJob* job ) { return mSubsequents.Enqueue( job ); }

   bool AddSubsequents( span<BaseJob*> jobs ) { return mSubsequents.Enqueue( jobs ); }

   void DispatchSubsequents()
   {
      JobList newJobs;
      mSubsequents.CloseAndFlush( newJobs );
      for(BaseJob* job: newJobs) {
         job->DecrementAndTryIssue();
      }
   }

   bool IsComplete() const { return mSubsequents.IsClosed(); }

   static S<JobHandle> Create() { return S<JobHandle>{ new JobHandle{} }; }

protected:
   ClosableLockQueue<BaseJob*> mSubsequents;
   eWorkerThread               mRunningThread;
};

template< typename TJob >
struct Job: public BaseJob, public NonCopyable
{
   class Constructor
   {
   public:
      friend struct Job<TJob>;

      Constructor( Job* owner, span<JobHandleRef> dependencies )
         : mOwner( owner )
         , mPresequents( dependencies )
      {
         ASSERT_DIE( mOwner->jobStatus == JOB_CREATED );
      }

      template< typename ...Args >
      JobHandleRef InitAndDispatch( Args&& ...args )
      {
         new( &mOwner->mStorage )TJob( std::forward<Args>( args )... );
         return mOwner->Setup( mPresequents );
      };
      Job*               mOwner;
      span<JobHandleRef> mPresequents;
   };

   virtual void Run( eWorkerThread threadId )
   {
      EASY_BLOCK( "Run Job" );
      (*((TJob*)((void*)(&mStorage))))( threadId );
      ((TJob*)((void*)(&mStorage)))->~TJob();
      mSubsequents->DispatchSubsequents();

      BaseJob* base = this;
      // delete base;
      base->~BaseJob();
   }

   static Constructor Allocate( span<JobHandleRef> dependencies, eWorkerThread threadMask )
   {
      constexpr auto size = sizeof( Job<TJob> );
      void* mem = sBuffer.Acquire( size );
      Job<TJob>* job = new (mem)Job<TJob>( JobHandle::Create(), dependencies.size() );
      return Constructor( job, dependencies );
   };

protected:

   Job( const JobHandleRef& subsequent, uint presequentCount )
      : BaseJob( presequentCount )
    , mSubsequents( subsequent ) {}

   JobHandleRef Setup( span<JobHandleRef> preseq )
   {
      // self is possible to be destructed at the same time.
      auto ret = mSubsequents;
      ASSERT_DIE( jobStatus == JOB_CREATED );
      jobStatus = JOB_SETUP;

      uint completedPreseqCount = 0;
      for(auto& jobHandle: preseq) {
         ASSERT_DIE( jobHandle != nullptr );
         if(!jobHandle->AddSubsequent( this )) { ++completedPreseqCount; }
      }

      CompletePresequents( completedPreseqCount );

      return ret;
   }

   std::aligned_storage_t<sizeof( TJob ), alignof(TJob)> mStorage;
   JobHandleRef                                          mSubsequents;
};

class TriggerEventJob
{
public:
   TriggerEventJob( SysEvent* eve )
      : mEvent( eve ) {
      ASSERT_DIE( eve );
   }

   void operator()(eWorkerThread threadId)
   {
      mEvent->Trigger();
   }
   SysEvent* mEvent;
};

template<typename F>
static JobHandleRef CreateAndDispatchFunctionJob(span<JobHandleRef> dependencies, F&& func, eWorkerThread threadMask = ~0)
{
   struct FunctionJob
   {
      FunctionJob(F&& func): mFunction( std::forward<F>( func ) ) {}
      void operator()( eWorkerThread t ) { mFunction( t ); }
      F mFunction;
   };
   return Job<FunctionJob>::Allocate( dependencies, threadMask ).InitAndDispatch( std::forward<F>( func ) );
}