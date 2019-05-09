#pragma once
#include <memory>

struct unique {
  constexpr bool operator==(const unique& rhs) const { return type == rhs.type; }
protected:
  constexpr unique(size_t type): type(type) {}
  size_t type;
};

template<typename ...T>
struct tid : public unique {
protected:
  static const char _id;
  constexpr tid(): unique((size_t)&_id) {}
public:
  static const tid<T...> value;
};

template<typename ...T>
const char tid<T...>::_id = 0;

template<typename ...T>
const tid<T...> tid<T...>::value;

template<typename T>
using S = std::shared_ptr<T>;

template<typename T>
using U = std::unique_ptr<T>;

template<typename T>
using W = std::weak_ptr<T>;