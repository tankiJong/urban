#pragma once

namespace random {
inline float Between01()
{
   return (float)rand() / (float)RAND_MAX;
};
inline float Between(float fromInclusive, float toInclusive)
{
   return Between01() * (toInclusive - fromInclusive) + fromInclusive;
};
};
