/**
 * @file   asmopt.h
 * @brief  Memory pools to segregate the heap. The intention of this is to have
 *         different heaps for different threads. This solves a few use cases, if
 *         each thread uses its own heap, it can use the non thread safe versions of 
 *         the allocators. Even if pools are shared, it still reduces contention for
 *         the global heap of the program. Also, releasing the entire pool at thread
 *         exit allows for any leaked memory to be reclaimed.
 * @author Rakesh Iyer.
 * @bug    Not tested for performance.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __ASMOPT_H__
#define __ASMOPT_H__

#ifdef USE_PREFETCH
#define PREFETCHNTA(x) asm volatile ("prefetchnta %0"::"m" ((x)):)
#else
#define PREFETCHNTA(x) do { } while (0)
#endif

#ifdef USE_PREDICTOR_HINTS
#define LIKELY(x)    __builtin_expect((x), 1)
#define UNLIKELY(x)  __builtin_expect((x), 0)
#else
#define LIKELY(x)    (x)
#define UNLIKELY(x)  (x)
#endif

#endif
