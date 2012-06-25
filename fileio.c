/**
 * @file   fileio.h
 * @brief  A user space implementation of a buffered file io system which uses
 *         writeback & readahead. Uses direct io to bypass linux's readahead
 *         and writeback mechanisms so that the algorithm used here can actually
 *         be evaluated.
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

#include "fileio.h"

/**
 * @brief Open a file for reading/writing.
 * @param  file     Out pointer to the file handle.
 * @param  path     The path of the file to open.
 * @param  mode     Options for opening the file.
 * @param  cache    The buffer cache to use for readahead/writeback. The entire
 *                  memory pool will be consumed.
 * @return
 */
int lpx_file_open(file_t *file, char *path, unsigned int mode,
                  lpx_mempool_variable_t *cache)
{
    return 0;
}

/**
 * @brief Close a file.
 * @param file The file handle to be closed. Files are flushed on close.
 * @return
 */
int lpx_file_close(file_t *file)
{
    return 0;
}

/**
 * @brief Read size bytes from a specified offset.
 * @param file
 * @param offset
 * @param buf
 * @param size
 * @return
 */
int lpx_file_read(file_t *file, unsigned long long offset,
                  void *buf, unsigned long size)
{
    return 0;
}

/**
 * @brief Write size bytes to a specified offset.
 * @param file
 * @param offset
 * @param buf
 * @param size
 * @return
 */
int lpx_file_write(file_t *file, unsigned long long offset,
                   void *buf, unsigned long size)
{
    return 0;
}
