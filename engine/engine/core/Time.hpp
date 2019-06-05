#pragma once

using millisecond_t = uint32_t;
using hpc_t = uint64_t;
class Time {
public:

   double second = 0;
   millisecond_t millisecond = 0;
   hpc_t hpc = 0;

   Time& operator+=(const Time& rhs)
   {
      second += rhs.second;
      millisecond += rhs.millisecond;
      hpc += rhs.hpc;

      return *this;
   }
   static Time Current();
};

class Clock {
public:
   Time frame;
   Time total;
   uint64_t frameCount = 0;

   void Forward(hpc_t elapsed);
   void Forward();

   static Clock& Main();
};