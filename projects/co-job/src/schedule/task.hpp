#pragma once
#include "token.hpp"


namespace co
{
template< typename T = void >
class task: public base_token<task, T>
{
   using base_t = base_token<task, T>;
   using coro_handle_t = typename base_t::coro_handle_t;
public:
   task(coro_handle_t handle): base_t(handle, false)
   {
      handle.promise().result = &mFuture;
      base_t::Dispatch();
   }

   const T& Result()
   {
      return mFuture.Get();
   }
protected:
   future<T> mFuture;
};
}
