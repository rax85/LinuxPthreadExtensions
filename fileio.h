/**
 * @file   fileio.h
 * @brief  A user space implementation of a buffered file io system which uses
 *         writeback & readahead.
 * @author Rakesh Iyer.
 * @bug    Unimplemented.
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

#ifndef __FILEIO_H__
#define __FILEIO_H__

#include "mempool.h"

typedef struct __file_t {
    int fd;
} file_t;

int lpx_file_open(file_t *file, char *path, unsigned int mode,
                  lpx_mempool_variable_t *cache);
int lpx_file_close(file_t *file);
int lpx_file_read(file_t *file, unsigned long long offset,
                  void *buf, unsigned long size);
int lpx_file_write(file_t *file, unsigned long long offset,
                   void *buf, unsigned long size);
#endif
