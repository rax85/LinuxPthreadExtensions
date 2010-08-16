/**
 * @file   tcpserver.h
 * @brief  Interface to the pooled threaded tcp server.
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

#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include "mempool.h"

/**
 * @def   TCPSERVER_SUCCESS
 * @brief Indicates that the call succeeded.
 */
#define TCPSERVER_SUCCESS		0

/**
 * @def   TCPSERVER_FAILURE
 * @brief Indicates that the call failed.
 */
#define TCPSERVER_FAILURE		-1

/**
 * @def   TCPSERVER_TIMEOUT 
 * @brief Indicates that the tcp server call timed out.

 */
#define TCPSERVER_TIMEOUT		-2

/**
 * @brief The structure that represents a tcp server.
 */
typedef struct __lpx_tcpserver_t {
    unsigned short port;	/**< The port that this server is listening on. */
} lpx_tcpserver_t;


int lpx_tcpserver_init(lpx_tcpserver_t *server, 
                       unsigned short port,
		       int num_workers,
		       int queue_length,
                       int (*handler)(int socketfd, lpx_mempool_variable_t *pool));
int lpx_tcpserver_start(lpx_tcpserver_t *server);
int lpx_tcpserver_clean_shutdown(lpx_tcpserver_t *server);
int lpx_tcpserver_hard_shutdown(lpx_tcpserver_t *server);

#endif
