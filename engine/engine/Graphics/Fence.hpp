#pragma once

#include <atomic>
#include "utils.hpp"
class CommandQueue;

/**
 * \brief 
 * Scenario 1 -- GPU produces and CPU consumes
   1. `CommandQueue::Signal` inserts a command into the queue. This command will be executed by the GPU LATER (in order, with other (draw) commands) and at that point, GPU will change the fence value.
   2. On CPU side, use `Fence::gpuValue` to returns immediately the current value, use `Fence::wait()` to stall until it reach the expected value.
   \n Good for reclaiming/reusing/recycling/destroying resources used by the GPU. Once GPU actually signals, CPU can be sure that GPU is done with what it's been doing and can do whatever with the resources.
   \n
   
   Scenario 2 -- CPU produces and GPU consumes
   1. `CommandQueue::Wait` inserts a command to into the queue. This command will be later executed by GPU and will cause its command processor to stall (and not launch any draws/dispatches) until the fence has the specified value.
   2. `Fence::Signal` immediately sets the fence value from the CPU and 'unblocks' the GPU.
   \n Good for async loading of stuff - GPU is running on its own, CPU is producing stuff. The wait ensures that GPU won't run ahead of CPU.
 */
class Fence: public WithHandle<fence_t> {
public:
   Fence();
   /**
    * \brief wait until the fence reach the expect value.
             GPU wait is achieved by `CommandQueue::Wait`
    */
   void Wait();

   void Wait(uint64_t val);
  
   /**
    * \brief signal new value on from Cpu side, this will increase the value by one.
             GPU signal is achieved by `CommandQueue::Signal`
    * \return the old value
    */
   uint64_t Signal();

   /**
    * \brief set the target value on cpu side, which will be the value later try to wait or signal
    * \param value to set
    * \return old value
    */
   uint64_t SetExpectValue( uint64_t value );

   /**
    * \brief increase the target value on cpu side, which will be the value later try to wait or signal
    * \param value to increase
    * \return old value
    */
   uint64_t IncreaseExpectedValue(uint64_t value = 1u) { mExpectValue += value; return mExpectValue; };
   
   /**
    * \brief 
    * \return current value on GPU
    */
   uint64_t GpuCurrentValue() const;
   
   /**
    * \brief 
    * \return the expected value on CPU side
    */
   uint64_t ExpectedValue() const { return mExpectValue; };


   static void WaitAll(Fence* fences, size_t count);

   ~Fence();
protected:
   std::atomic<uint64_t> mExpectValue = 0;
   void* mEventHandle = nullptr;
};

inline uint64_t Fence::SetExpectValue( uint64_t value )
{
   uint64_t old     = mExpectValue;
   mExpectValue = value;
   return mExpectValue;
}
