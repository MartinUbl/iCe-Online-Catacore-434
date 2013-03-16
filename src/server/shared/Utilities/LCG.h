/*
 * Linear congruential generator, used to generate fast random numbers without needing to warm-up
 * Copyright (C) 2013 Aliquis, G-Core <http://ice-wow.eu/>
 */

#include "Common.h"

class LCG
{
protected:
    uint32 x, a, c;

public:
    LCG(uint32 a = 134775813, uint32 b = 1);
    void Seed(uint32 seed);
    uint32 Generate();
    uint32 Generate(uint32 n);     // generates random number of discrete uniform distribution from 0 to (n-1)
};
