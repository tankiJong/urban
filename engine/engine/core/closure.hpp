#pragma once

#include "engine/pch.h"
#include "engine/core/vary.hpp"
#include "engine/core/any_func.hpp"
class arg_list {
public:
   static constexpr size_t kStorageSize = 92; // in byte

   template<typename ...Args>
   using storage_data_t = std::tuple<Args...>;

   template<typename ...Args>
   arg_list(Args&& ...args) {
      using data_t = storage_data_t<Args...>;
      mData.Set<data_t>(data_t{std::forward<Args>(args)...});
   }

   template<typename ...Args>
	storage_data_t<Args...>& get() {
      return mData.Get<storage_data_t<Args...>>();
   }

protected:
   vary mData;
};

template<
    typename T,
    typename R,
    typename... Args
  > using closure_decay = std::conditional<
    std::is_convertible<T, R(*)(Args...)>::value,
    R(*)(Args...),
    typename std::decay<T>::mType
  >;
class closure {
   public:

      template<typename ...Args, typename T, typename C = typename closure_decay<T, void, Args...>::mType>
      closure(T&& func, Args&&... args)
         : mArgs(std::forward<std::decay_t<Args>>(args)...) 
         , mFunction(func) {
         mCallback = &Invoke<std::decay_t<Args>...>;
      }

      template<typename Object, typename R, typename ...Args>
      closure(Object* object, R(Object::*func)(Args...), Args&&... args)
         : mArgs(std::forward<std::decay_t<Args>>(args)...) 
         , mFunction(object, func) {
         mCallback = &Invoke<std::decay_t<Args>...>;
      }

      template<typename Object, typename R, typename ...Args>
      closure(const Object* object, R(Object::*func)(Args...) const, Args&&... args)
         : mArgs(std::forward<std::decay_t<Args>>(args)...) 
         , mFunction(object, func) {
         mCallback = &Invoke<std::decay_t<Args>...>;
      }

      void operator()() {
         mCallback(*this);
      }
      
   protected:
      using callback_t = void(*)(closure&);
      arg_list mArgs;
      any_func mFunction;
      callback_t mCallback;

      template<typename ...Args, size_t ...Indices>
      static void TupleExecute(any_func& func, std::tuple<Args...>& args, std::index_sequence<Indices...>) {
         std::invoke(func, std::get<Indices>(args)...);
      }

      template<typename ...Args>
      static void Invoke(closure& decl) {
         arg_list::storage_data_t<Args...>& data = decl.mArgs.get<Args...>();
         TupleExecute(decl.mFunction, data, 
			            std::make_index_sequence<std::tuple_size_v<std::tuple<Args...>>>());
      }
   };
   // https://isocpp.org/wiki/faq/pointers-to-members#addr-of-memfn