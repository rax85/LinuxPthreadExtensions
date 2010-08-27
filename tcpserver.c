/**
 * @file   tcpserver.c
 * @brief  An implementation of a tcp server that uses thread pools underneath.
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

static void *worker(void *param);
static void *dispatcher(void *param);
static void destroyWorkers(lpx_tcpserver_t *server);

volatile static int shutDownServer;

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
                       int (*handler)(lpx_connection_data_t *request, int worker_index))
{
    int i = 0;
    lpx_tcpworker_data_t *workerData = NULL;

    if (server == NULL || handler == NULL) {
        return TCPSERVER_FAILURE;
    }

    if (queue_length <= 0 || num_workers <= 0) {
        return TCPSERVER_FAILURE;
    }

    server->handler = handler;
    server->port = port;
    server->numWorkers = num_workers;
    server->futures = 
            (lpx_thread_future_t **)malloc(sizeof(lpx_thread_future_t *) * num_workers);
    if (server->futures == NULL) {
        return TCPSERVER_FAILURE;
    }

    // Allocate the workerdata array.
    workerData = 
             (lpx_tcpworker_data_t *)malloc(sizeof(lpx_tcpworker_data_t) * num_workers);
    if (NULL == workerData) {
        goto server_destroy1;
    }
    server->workerData = (void *)workerData;

    // Create thread pool - with an extra one for the dispatcher.
    server->workers = 
              lpx_threadpool_init(num_workers + 1, num_workers + 1, THREAD_POOL_FIXED);
    if (server->workers == NULL) {
        goto server_destroy2;
    }

    // Create the request queue.
    if (0 != lpx_pcq_init(&server->requestQueue, queue_length)) {
        goto server_destroy3;
    }

    // Spawn off a worker with a pointer to this struct and the thread number.
    for (i = 0; i < num_workers; i++) {
        workerData[i].threadNum = i;
	workerData[i].server = server;
	server->futures[i] = 
	             lpx_threadpool_execute(server->workers, worker, &workerData[i]);

        if (server->futures[i] == NULL) {
	    goto server_destroy4;
	}
    }
    
    // Create a memory pool for the request objects.
    if (0 != lpx_mempool_create_fixed_pool(&server->cDataPool, 
                                   sizeof(lpx_connection_data_t), 
				   queue_length + num_workers, MEMPOOL_PROTECTED)) {
        goto server_destroy4;
    }

    // Ok, we're all set here to start the server.
    return TCPSERVER_SUCCESS;

server_destroy4: destroyWorkers(server); 
                 lpx_threadpool_destroy(server->workers);
server_destroy3: lpx_pcq_destroy(&server->requestQueue);
server_destroy2: free(workerData);
server_destroy1: free(server->futures);
    return TCPSERVER_FAILURE;
}


/**
 * @brief  Actually start up the server.
 * @param  server The server to start.
 * @return 0 on success, -1 on failure.
 */
int lpx_tcpserver_start(lpx_tcpserver_t *server)
{
    struct sockaddr_in serverAddr;
    int reuseAddr = 1;

    if (server == NULL) {
        return TCPSERVER_FAILURE;
    }

    // First, create the server socket.
    server->listener = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listener < 0) {
        return TCPSERVER_FAILURE;
    }
    
    // Get rid of any address already in use messages.
    setsockopt(server->listener, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(int));

    // Bind it to the port that we care about.
    memset(&serverAddr, 0, sizeof(struct sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(server->port);
    if (bind(server->listener, (struct sockaddr *)&serverAddr, 
                                            sizeof(serverAddr)) != 0) {
        goto destroy_start1;
    }

    // Fire up the dispatcher thread to server the connection.
    server->dispatcher = lpx_threadpool_execute(server->workers, dispatcher, server);
    if (server->dispatcher == NULL) {
        goto destroy_start1;
    }

    return TCPSERVER_SUCCESS;

destroy_start1: close(server->listener);
    return TCPSERVER_FAILURE;
}


/**
 * @brief Release any resources the server might have allocated.
 * @param server The server to destroy.
 */
static void releaseServerResources(lpx_tcpserver_t *server)
{
    lpx_threadpool_destroy(server->workers);
    lpx_mempool_destroy_fixed_pool(&server->cDataPool);
    lpx_pcq_destroy(&server->requestQueue);
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
    // Shut down all the workers.
    destroyWorkers(server);
    
    // Destroy all server resources.
    releaseServerResources(server);

    return TCPSERVER_SUCCESS;
}

/**
 * @brief Tries to shut down all workers cleanly.
 * @param The server to stop.
 */
void destroyWorkers(lpx_tcpserver_t *server)
{
    int i = 0;
    void *shutdownRequest = NULL;
    void *retval = NULL;

    // First, tell the listener thread to shut down and close the listening socket.
    shutDownServer = 1;
    lpx_threadpool_join(server->dispatcher, &retval);
    
    // Start enqueueing NULLs into the queue till all the threads exit. Each
    // worker will ever only dequeue a single NULL from the queue as it dies
    // when it sees one.
    for(i = 0; i < server->numWorkers; i++) {
        lpx_pcq_enqueue(&server->requestQueue, shutdownRequest);
    }
    
    // Join on all the threads.
    for(i = 0; i < server->numWorkers; i++) {
	lpx_threadpool_join(server->futures[i], &retval);
    }

    return;
}



/**
 * @brief  The worker that dispatches requests that have been accepted.
 * @param  param A handle to the thread number and parent server.
 * @return Ignored.
 */
static void *worker(void *param)
{
    lpx_tcpworker_data_t *data = (lpx_tcpworker_data_t *)data;
    lpx_tcpserver_t *parent = data->server;
    int threadNumber = data->threadNum;
    lpx_connection_data_t *request = NULL;
    struct timespec now;

    while(1) {
        request = NULL;
	if (0 == lpx_pcq_dequeue(&parent->requestQueue, (void **)&request)) {
	    
	    // Enqueuing a NULL into the queue is a way to tell the worker to exit.
	    if (request == NULL) {
	        break;
            }

	    // Get the time since we got this request.
	    clock_gettime(CLOCK_REALTIME, &now);
	    request->age_milliseconds = timespecDiffMillis(now, request->timestamp);
	    parent->handler(request, threadNumber);
	    close(request->fd);
	    lpx_mempool_fixed_free(request);
	}
    }

    return NULL;
}

/**
 * @brief The thread that listens for requests and dispatches them.
 * @param param A handle to the parent server.
 * @return Ignored.
 */
static void *dispatcher(void *param)
{
    lpx_tcpserver_t *server = (lpx_tcpserver_t *)param;
    lpx_connection_data_t *request = NULL;
    socklen_t addrLen = 0;

    while(!shutDownServer) {
        // Allocate a new object.
        request = lpx_mempool_fixed_alloc(&server->cDataPool);
	memset(request, 0, sizeof(lpx_connection_data_t));
        addrLen = sizeof(struct sockaddr_in);

	// Accept the request.
	request->fd = accept(server->listener, (void *)&request->peer, &addrLen); 

	// Timestamp the accept.
	clock_gettime(CLOCK_REALTIME, &request->timestamp);

	// Enqueue the request into the queue.
	if (0 != lpx_pcq_enqueue(&server->requestQueue, request)) {
	    lpx_mempool_fixed_free(request);
	}
    }

    return NULL;
}


