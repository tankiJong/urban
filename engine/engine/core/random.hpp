#pragma once

namespace random {
float Between01()
{
   return (float)rand() / (float)RAND_MAX;
};
float Between(float fromInclusive, float toInclusive)
{
   return Between01() * (toInclusive - fromInclusive) + fromInclusive;
};
};
