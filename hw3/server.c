#include "segel.h"
#include "request.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//
#define POP_WAITING_QUEUE pop(waitingQueue,&waitingCond,&waitingQueueLock,&waitingQueueSize)
#define POP_WORKING_QUEUE pop(workingQueue,&workingCond,&workingQueueLock,&workingQueueSize)

#define PUSH_WAITING_QUEUE(connfd) push(waitingQueue,(connfd),&waitingCond,&waitingQueueLock,&waitingQueueSize)
#define PUSH_WORKING_QUEUE(connfd) push(workingQueue,(connfd),&workingCond,&workingQueueLock,&workingQueueSize)

#define DELETE_NODE_WAITING_QUEUE(connfd) deleteNode(waitingQueue,(connfd),&waitingCond,&waitingQueueLock,&waitingQueueSize)
#define DELETE_NODE_WORKING_QUEUE(connfd) deleteNode(workingQueue, (connfd) ,&workingCond,&workingQueueLock,&workingQueueSize)


// HW3: Parse the new arguments too
void getargs(int *port, int argc, char *argv[])
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
}




//Global Vars
pthread_mutex_t waitingQueueLock = PTHREAD_MUTEX_INITIALIZER,workingQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waitingCond =PTHREAD_COND_INITIALIZER, workingCond = PTHREAD_COND_INITIALIZER;
int waitingQueueSize = 0,workingQueueSize =0;
Queue* waitingQueue = NULL;
Queue* workingQueue = NULL;

void* requestHandlerWorker(void* args){
    int connfd = POP_WAITING_QUEUE;
    PUSH_WORKING_QUEUE(connfd);
    requestHandle(connfd);
    Close(connfd);
    DELETE_NODE_WORKING_QUEUE(connfd);
    return NULL;
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    //Preparing Arguments.
    getargs(&port, argc, argv);
    workingQueue = initQueue();
    waitingQueue = initQueue();
    int threadsNum = atoi(argv[1]);
    pthread_t* threadsArr = (pthread_t*)malloc(threadsNum*sizeof(pthread_t));

    //Spawning Threads.
    for(int i=0;i<threadsNum;++i){
        pthread_create(&threadsArr[i],NULL,requestHandlerWorker, NULL);
    }

    //Request Accepting
    listenfd = Open_listenfd(port);
    while(1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);//Connceting to the next client.
    QueueStatus status = PUSH_WAITING_QUEUE(connfd);//Adding request to waiting Queue
    }
    destroyQueue(waitingQueue);
    destroyQueue(workingQueue);
    free(threadsArr);
}


    


 
