#pragma once

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

   /**
    * \brief wait until the fence reach the expect value.
             GPU wait is achieved by `CommandQueue::Wait`
    */
   void Wait();
  
   /**
    * \brief signal new value on from Cpu side, this will increase the value by one.
             GPU signal is achieved by `CommandQueue::Signal`
    * \return the old value
    */
   uint Signal();

   /**
    * \brief set the target value on cpu side, which will be the value later try to wait or signal
    * \param value to set
    * \return old value
    */
   uint SetExpectValue( uint value );

   /**
    * \brief increase the target value on cpu side, which will be the value later try to wait or signal
    * \param value to increase
    * \return old value
    */
   uint IncreaseExpectedValue(uint value = 1u);
   
   /**
    * \brief 
    * \return current value on GPU
    */
   uint GpuCurrentValue();
   
   /**
    * \brief 
    * \return the expected value on CPU side
    */
   uint ExpectedValue();

protected:
   uint mExpectValue = 0;
   void* mEventHandle = nullptr;
};

inline uint Fence::SetExpectValue( uint value )
{
   uint old     = mExpectValue;
   mExpectValue = value;
   return mExpectValue;
}
