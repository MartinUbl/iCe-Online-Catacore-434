/*
 * Copyright (C) 2013 Freghar, G-Core <http://ice-wow.eu/>
 */

#ifndef GCORE_GUIDMAP_H
#define GCODE_GUIDMAP_H

class GuidMap
{
    private:
        unsigned long **slices;
        unsigned long slice_cnt;
        unsigned long slice_size;

        void setbit(unsigned long *arr, long long idx);
        long long find_empty(unsigned long *arr, unsigned long arrsize);
        int addslice();

    public:
        /* slice_bits: minimum size for one slice in bits (granularity),
         * IT HAS TO BE ALWAYS MORE THAN 0 AND LESS THAN ULONG_MAX */
        GuidMap(long long slice_bits);
        ~GuidMap();

        void SetBit(long long index);
        long long UseEmpty();
};

#endif
