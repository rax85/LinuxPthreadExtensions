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
#include "sem.h"
#include "pcQueue.h"
#include "threadPool.h"
#include <sys/socket.h>
#include <netinet/in.h>

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

struct __lpx_connection_data_t;

/**
 * @brief The structure that represents a tcp server.
 */
typedef struct __lpx_tcpserver_t {
    int numWorkers; 		     /**< Number of workers. */
    lpx_thread_future_t **futures;   /**< Futures for each of the threads. */
    lpx_thread_future_t *dispatcher; /**< Future of the dispatcher thread. */
    lpx_pcq_t requestQueue;	     /**< Queue of work to do. */
    unsigned short port;	     /**< The port that this server is listening on. */
    int listener;                    /**< The fd of the listening socket. */
    lpx_threadpool_t *workers;       /**< The pool of workers */
    lpx_mempool_fixed_t cDataPool;   /**< Memory pool for connection data objects */
    void *workerData;  		     /**< Worker data to be released later. */
    int (*handler)(struct __lpx_connection_data_t *, int); /**< The handler that is invoked on each socket. */
} lpx_tcpserver_t;

/**
 * @brief A structure to contain data passed to each worker.
 */
typedef struct __lpx_tcpworker_data_t {
    int threadNum;                /**< The thread number of this thread. */
    lpx_tcpserver_t *server;      /**< Back pointer to the parent server. */
} lpx_tcpworker_data_t;

/**
 * @brief A structure to enqueue into the request queue. */
typedef struct __lpx_connection_data_t {
    struct sockaddr_in peer;     /**< Who is on the other side of this socket. */
    int fd;			 /**< File descriptor of the socket. */
    struct timespec timestamp;   /**< The time when this connection arrived. */
    long age_milliseconds;       /**< Request age in milliseconds. */
}lpx_connection_data_t;


int lpx_tcpserver_init(lpx_tcpserver_t *server, 
                       unsigned short port,
		       int num_workers,
		       int queue_length,
                       int (*handler)(lpx_connection_data_t *request, int worker_index));
int lpx_tcpserver_start(lpx_tcpserver_t *server);
int lpx_tcpserver_clean_shutdown(lpx_tcpserver_t *server);

#endif
