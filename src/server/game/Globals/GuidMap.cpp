#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "gamePCH.h"
#include "GuidMap.h"

#define UL_BITS (sizeof(unsigned long)*CHAR_BIT)


/* private */

inline void GuidMap::setbit(unsigned long *arr, long long idx)
{
    arr[idx/UL_BITS] |= ((unsigned long)1<<(idx%UL_BITS));
}

long long GuidMap::find_empty(unsigned long *arr, unsigned long arrsize)
{
    unsigned long i, j;

    /* fast compare using long comparison,
     * precise search using bit shift */

    /* skip completely full longs */
    for (i = 0; i < arrsize; i++)
        if (arr[i] != ~(unsigned long)0)
            break;

    /* the array is completely full, result invalid */
    if (i == arrsize)
        return -1;

    /* in the current non-full long, find exact bit offset
     * of the free bit */
    for (j = 0; j < UL_BITS; j++)
        if (~arr[i] & (unsigned long)1<<j)
            break;

    /* resulting bit offset = i in bits + j */
    return i*UL_BITS + j;
}

int GuidMap::addslice()
{
    unsigned long array_num;
    unsigned long *newarray, **arrays;

    /* allocate new slice separately, with zeros */
    newarray = (unsigned long*)
                   calloc(this->slice_size, sizeof(unsigned long));
    if (newarray == NULL)
        return -1;

    /* resize slices array */
    array_num = this->slice_cnt + 1;
    arrays = (unsigned long**)
                   realloc(this->slices, array_num*sizeof(unsigned long *));
    if (arrays == NULL) {
        free(newarray);
        return -1;
    }

    /* add new slice */
    arrays[array_num-1] = newarray;

    /* update bitarray struct only when all allocations passed */
    this->slices = arrays;
    this->slice_cnt++;

    return 0;
}


/* public */

GuidMap::GuidMap(long long slice_bits)
{
    unsigned long long slice_size;

    /* calculate size in unsigned longs */
    slice_size = slice_bits / UL_BITS;
    slice_size += !!(slice_bits % UL_BITS);

    /* unspecified struct bitarray members need to be 0 */
    memset(this, 0, sizeof(GuidMap));
    this->slice_size = slice_size;
}

GuidMap::~GuidMap()
{
    unsigned long i;

    for (i = 0; i < this->slice_cnt; i++)
        free(this->slices[i]);

    free(this->slices);
}

void GuidMap::SetBit(long long index)
{
    unsigned long slice_offset;

    slice_offset = index / (this->slice_size*UL_BITS);
    index = index % (this->slice_size*UL_BITS);

    /* if slice_offset outreaches allocated slices, allocate more
     * (comparing offset and count, can't do count-1 on unsigned, hence +1) */
    while (slice_offset+1 > this->slice_cnt)
        addslice();

    setbit(this->slices[slice_offset], index);
}

long long GuidMap::UseEmpty()
{
    unsigned long i;
    long long ret;

    /* try to find empty bit in current slice set */
    for (i = 0; i < this->slice_cnt; i++) {
        ret = find_empty(this->slices[i], this->slice_size);
        if (ret != -1) {
            setbit(this->slices[i], ret);
            return i*this->slice_size*UL_BITS + ret;
        }
    }

    /* all slices full, allocate another one */
    addslice();
    setbit(this->slices[i], 0);
    return i*this->slice_size*UL_BITS;
}
