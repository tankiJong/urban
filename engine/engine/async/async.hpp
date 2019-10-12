#pragma once

using eWorkerThread = uint;
struct BaseJob
{
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

protected:
   eJobStatus mJobStatus = JOB_CREATED;
};

class Scheduler
{
   friend class BaseJob;

public:
   virtual void Issue(BaseJob* job) = 0;
   static void Init();
   static void Cleanup();
   static Scheduler& Get();
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
      friend class Job<TJob>;
      Constructor(Job* owner) {}

      template<typename ...Args>
      Job* InitAndDispatch(Args&& ...args)
      {
         new (&mOwner->mStorage)TJob(std::forward<Args>( args ));
         Scheduler::Get().Issue( mOwner );
      };
      Job* mOwner;
   };


   virtual void Run( eWorkerThread threadId )
   {
      *((TJob*)((void*)(&mStorage)))(threadId);
      delete this;
   }

   static Constructor Allocate( eWorkerThread threadId )
   {
      Job<TJob>* job = new Job<TJob>();
      return Constructor( job );
   };
protected:
   std::aligned_storage_t<sizeof( TJob ), alignof(TJob)> mStorage;
   
};
