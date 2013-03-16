#include "LCG.h"

static const uint64 genLimit = 0x100000000;

LCG::LCG(uint32 a/* = 134775813 */, uint32 c/* = 1 */): a(a), c(c)
{
}

void LCG::Seed(uint32 seed)
{
    x = seed;
}

uint32 LCG::Generate()
{
    uint64 temp = x;
    temp *= a;
    temp += c;
    temp %= genLimit;
    x = (uint32)temp;
    return x;
}

uint32 LCG::Generate(uint32 n)
{
    uint64 temp = Generate();
    temp *= n;
    temp /= genLimit;
    return (uint32)temp;
}
