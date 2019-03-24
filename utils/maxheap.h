/*
 * Tulip Indicators
 * https://tulipindicators.org/
 * Copyright (c) 2010-2019 Tulip Charts LLC
 * Ilya Pikulin (ilya.pikulin@gmail.com)
 *
 * This file is part of Tulip Indicators.
 *
 * Tulip Indicators is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * Tulip Indicators is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Tulip Indicators.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __MAXHEAP_H__
#define __MAXHEAP_H__

#include "../indicators.h"

/* Functions were implemented as macros since this approach has shown a 10% better results compared to plain functions. */

#define ti_swap(a, b) do { \
    TI_REAL tmp = a; \
    a = b; \
    b = tmp; \
} while (0)


#define ti_maxheap_push(store, size, value) do { \
    int _i = size; \
    store[_i] = value; \
    while (store[_i] > store[_i>>1]) { \
        ti_swap(store[_i], store[_i>>1]); \
        _i >>= 1; \
    } \
} while (0)

#define ti_maxheap_pop(store, size, value) do { \
    value = store[0]; \
    store[0] = store[size-1]; \
    int _i = 0; \
    while ((_i<<1)+1 < size) { \
        int left_child_idx = (_i<<1)+1, right_child_idx = (_i<<1)+2; \
        int max_child_idx = right_child_idx < size && store[left_child_idx] < store[right_child_idx] ? \
            right_child_idx : left_child_idx; \
        if (store[_i] < store[max_child_idx]) { \
            ti_swap(store[_i], store[max_child_idx]); \
            _i = max_child_idx; \
        } else { \
            break; \
        } \
    } \
} while (0)

#endif