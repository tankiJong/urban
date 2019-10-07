#pragma once

class JobBase
{
   enum
   {
      JOB_CANCELED = -1,
      JOB_CREATED,
      JOB_ENQUEUE,
      JOB_SCHEDULED,
      JOB_EXECUTING,
      JOB_DONE,
   };
};

class Scheduler
{
   friend class JobBase;

   virtual void Issue(JobBase* job) = 0;
public:
   static void Init();
   static void Cleanup();
   static Scheduler& Get();
};