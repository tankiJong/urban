#pragma once
#include "token.hpp"


namespace co
{

template<bool Instant, typename T>
class meta_token: public base_token<Instant, meta_token, T>
{
   using base_t = base_token<Instant, meta_token, T>;
public:
   using base_t::base_t;
};

template<typename T = void>
using token = meta_token<true, T>;
template<typename T = void>
using deferred_token = meta_token<false, T>;


template<bool Instant, typename T>
class meta_task: public base_token<Instant, meta_task, T>
{
   static_assert(!std::is_void_v<T>, "T for Task should not be void");
   using base_t = base_token<Instant, meta_task, T>;
   using coro_handle_t = typename base_t::coro_handle_t;
public:
   meta_task(coro_handle_t handle): base_t(handle, &mFuture)
   {
      // this is potentially buggy because at the time when the base token was dispatched,
      // it's possible that the mFuture is not fully initialized yet, so if the job is finished *really* quick... it will blow up
   }

   meta_task(meta_task&& from) noexcept: base_t(from)
   {
      auto& promise = base_t::mHandle.promise();
      promise.futuerPtr = &mFuture;
   }

   const T& Result()
   {
      return mFuture.Get();
   }

   ~meta_task()
   {
      auto& promise = base_t::mHandle.promise();
      promise.futuerPtr = nullptr;
   }
protected:
   future<T> mFuture;
};

template<typename T = void>
using task = meta_task<true, T>;
template<typename T = void>
using deferred_task = meta_task<false, T>;

}
