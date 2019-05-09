#pragma once
#include <cstdint>


using uint = uint32_t;

#define BIT_FLAG(f) (1U<<(f))

#define enum_class_operators(e_) inline e_ operator& (e_ a, e_ b){return static_cast<e_>(static_cast<int>(a)& static_cast<int>(b));}  \
    inline e_ operator| (e_ a, e_ b){return static_cast<e_>(static_cast<int>(a)| static_cast<int>(b));} \
    inline e_& operator|= (e_& a, e_ b){a = a | b; return a;};  \
    inline e_& operator&= (e_& a, e_ b) { a = a & b; return a; };   \
    inline e_  operator~ (e_ a) { return static_cast<e_>(~static_cast<int>(a));}   \
    inline bool is_any_set(e_ val, e_ flag) { return (val & flag) != (e_)0;} \
    inline bool is_all_set(e_ val, e_ flag) { return (val & flag) == flag;}   

#define UNUSED(X) (void*)&X;