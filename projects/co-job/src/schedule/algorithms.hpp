#pragma once
#include "token.hpp"
#include <vector>

namespace co
{
template<typename T>
void sequential_all(const std::vector<token<T>>& tokens)
{
   for(const auto& token: tokens) {
      co_await token;
   }
}
}
