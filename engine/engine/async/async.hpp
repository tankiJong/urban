#pragma once

using eWorkerThread = uint;

struct BaseJob;

class Scheduler
{
   friend struct BaseJob;

public:
   virtual void Issue(BaseJob* job) = 0;
   virtual BaseJob* Request( eWorkerThread id ) = 0;
   static void Init();
   static void Cleanup();
   static Scheduler& Get();
};

struct BaseJob
{
   friend class Scheduler;
   enum eJobStatus
   {
      JOB_CANCELED = -1,
      JOB_CREATED,
      JOB_ENQUEUE,
      JOB_SCHEDULED,
      JOB_EXECUTING,
      JOB_DONE,
   };

   virtual void Run( eWorkerThread threadId ) = 0;

   eJobStatus jobStatus = JOB_CREATED;
protected:
   void IssueSelf()
   {
      ASSERT_DIE( jobStatus == JOB_CREATED );
      Scheduler::Get().Issue( this );
      jobStatus = JOB_ENQUEUE;
   }

};



template<typename T>
struct CallableJob: public BaseJob
{
   CallableJob(T&& func): T(std::forward<T>( func )) {}
   virtual void Run( eWorkerThread threadId /*CompleteEvent*/ )
   {
      mFunc(threadId);
   }


protected:
   T mFunc;
};

template<typename TJob>
struct Job: public BaseJob, public NonCopyable
{
  
   class Constructor
   {
   public:
      friend struct Job<TJob>;
      Constructor(Job* owner): mOwner( owner )
      {
         ASSERT_DIE( mOwner->jobStatus == JOB_CREATED );
      }

      template<typename ...Args>
      Job* InitAndDispatch(Args&& ...args)
      {
         new (&mOwner->mStorage)TJob(std::forward<Args>(args)...);
         mOwner->IssueSelf();
         return mOwner;
      };
      Job* mOwner;
   };

   virtual void Run( eWorkerThread threadId )
   {
      (*((TJob*)((void*)(&mStorage))))(threadId);
      delete this;
   }
   static Constructor Allocate( eWorkerThread threadMask )
   {
      Job<TJob>* job = new Job<TJob>();
      return Constructor( job );
   };

protected:
   std::aligned_storage_t<sizeof( TJob ), alignof(TJob)> mStorage;
   
};
