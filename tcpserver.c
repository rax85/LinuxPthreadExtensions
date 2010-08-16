/**
 * @file   tcpserver.c
 * @brief  An implementation of a tcp server that uses thread pools and memory
 *         pools underneath.
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


#include "tcpserver.h"

/**
 * @brief  Create a threaded pooled tcp server.
 * @param  server The server to initialize.
 * @param  port The port that this server should listen on.
 * @param  num_workers The number of thread workers to spawn.
 * @param  queue_length The number of requests to queue. This is limited by the 
 *         maximum number of file descriptors can be opened, so make sure that the 
 *         values are set appropriately. For starters see 'man ulimit'.
 * @param  handler The handler that should be called each time a connection
 *         needs to be handled.
 * @return 0 on success, -1 on failure.
 */
int lpx_tcpserver_init(lpx_tcpserver_t *server, 
                       unsigned short port,
		       int num_workers,
		       int queue_length,
                       int (*handler)(int socketfd, lpx_mempool_variable_t *pool))
{
    return TCPSERVER_SUCCESS;
}


/**
 * @brief  Actually start up the server.
 * @param  server The server to start.
 * @return 0 on success, -1 on failure.
 */
int lpx_tcpserver_start(lpx_tcpserver_t *server)
{
    return TCPSERVER_SUCCESS;
}


/**
 * @brief  Shut down a server cleanly. This involves first stopping new connections
 *         and waiting for any in flight connections to be serviced. Once all the 
 *         outstanding connections have been serviced, the server will shutdown.
 * @param  server The server to shutdown.
 * @return 0 on success, -1 on failure.
 */
int lpx_tcpserver_clean_shutdown(lpx_tcpserver_t *server)
{
    return TCPSERVER_SUCCESS;
}


/**
 * @brief  Terminate the server immediately.
 * @param  server The server to terminate.
 * @return 0 on success, -1 on failure.
 */
int lpx_tcpserver_hard_shutdown(lpx_tcpserver_t *server)
{
    return TCPSERVER_SUCCESS;
}


