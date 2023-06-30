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
// HW3: Parse the new arguments too






#define POP_WAITING_QUEUE pop(waitingQueue,&waitingCond,&global,&waitingQueueSize,&emptyCond, &fullCond)
#define POP_WORKING_QUEUE pop(workingQueue,&workingCond,&global,&workingQueueSize, &canPush)

#define PUSH_WAITING_QUEUE(connfd) push(waitingQueue,(connfd),&waitingCond,&global,&waitingQueueSize)
#define PUSH_WORKING_QUEUE(connfd) push(workingQueue,(connfd),&workingCond,&global,&workingQueueSize)

#define DELETE_NODE_WAITING_QUEUE(connfd) deleteNode(waitingQueue,(connfd),&waitingCond,&global,&waitingQueueSize)
#define DELETE_NODE_WORKING_QUEUE(connfd) deleteNode(workingQueue, (connfd) ,&workingCond,&global,&workingQueueSize,&emptyCond, &fullCond)

#define MICRO_TO_SEC 1000000



//Global Vars
pthread_mutex_t global = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waitingCond =PTHREAD_COND_INITIALIZER,workingCond = PTHREAD_COND_INITIALIZER
        , fullCond =PTHREAD_COND_INITIALIZER, emptyCond =PTHREAD_COND_INITIALIZER;
int waitingQueueSize = 0,workingQueueSize =0;
Queue* waitingQueue = NULL;
Queue* workingQueue = NULL;

void* requestHandlerWorker(void* args){
    //printf("Waiting for a new requseet\n");
    while(1){
        threadInfo* thread = (threadInfo*)args;
        //printf("Push Node from working Queue\n");
        requestInfo request = POP_WAITING_QUEUE;
        struct timezone t;
        gettimeofday(&request.startingTime,&t);
        long t1 =  request.startingTime.tv_sec*MICRO_TO_SEC +  request.startingTime.tv_usec;
        long t2  =  request.arrivingTime.tv_sec*MICRO_TO_SEC +  request.arrivingTime.tv_usec;
        long delta = t1-t2;
        request.startingTime.tv_sec = delta/MICRO_TO_SEC;
        request.startingTime.tv_usec = delta % MICRO_TO_SEC;
        request.thread = thread;
      //  printf("Push Node from working Queue\n");
        PUSH_WORKING_QUEUE(request);
        requestHandle(request);
        //printf("Removing Node from working Queue\n");
        DELETE_NODE_WORKING_QUEUE(request);
        Close(request.fd);
    }
    return NULL;
}

int cmp(const void* x, const void* y){
    if(*(int*)x > *(int*)y){
        return 1;
    }
    return -1;
}

int getHalf(int x){
    int a = ((x%2) == 0) ? 0 : 1;
    return x/2 +a;
}

void dropRandom(){
    printf("Im here\n");
    int half = getHalf(waitingQueueSize);
    int *toDrop =(int*)malloc(sizeof(int)*half);
    bool *used = (bool*)malloc(sizeof(bool)*(waitingQueueSize));
    for(int i=0;i<waitingQueueSize;++i){
        used[i]=false;
    }
    //choose who to delete randomly
    for (int i = 0; i < half; i++)
    {
        toDrop[i] = rand() % waitingQueueSize;
        while(used[toDrop[i]])
            toDrop[i] = rand() % waitingQueueSize;
        used[toDrop[i]] = true;
    }

    qsort(toDrop,half,sizeof(int),cmp);
    for(int i=0;i<half;++i){
        printf("%d,",toDrop[i]);
    }
    printf("\n");
    int waitingQueueIndex = 0;
    int toDropIndex = 0;
    requestInfo request;
//delete half of the wq
    Node *curr = waitingQueue->head,*temp;
    while (curr != waitingQueue->last)
    {
        if(toDrop[toDropIndex] == waitingQueueIndex){
            temp = curr;
            curr= curr->next;
            request = removeNode(waitingQueue,temp);
            Close(request.fd);
            toDropIndex++;
            waitingQueue->size--;
        }else{
            curr = curr->next;
        }
        waitingQueueIndex++;
    }
    waitingQueueSize -= half; //update wq size
    free(toDrop);
    free(used);
}

void addRequest(int* queueSizePtr,char* policy, int maxQueueSize, requestInfo newRequest) {
    pthread_mutex_lock(&global);
    bool ignore = false;
    int queueSize = *queueSizePtr;
    if(strcmp(policy,"block") == 0){
        while(waitingQueueSize + workingQueueSize >= queueSize) {
          //  printf("\n stuck...\n");
            pthread_cond_wait(&fullCond, &global);
           // printf("\n after wait...\n");
        }
    }
    if(strcmp(policy,"dt") == 0){
        if(waitingQueueSize + workingQueueSize >= queueSize ) {
            ignore = true;
        }
    }
    if(strcmp(policy,"dh") == 0){
        while(waitingQueueSize + workingQueueSize == queueSize) {
            _pop(waitingQueue);
            waitingQueueSize--;
        }
    }
    if(strcmp(policy,"bf") == 0){
        while(waitingQueueSize + workingQueueSize >= queueSize) {
            ignore = true;
            while(waitingQueueSize + workingQueueSize !=0){
                pthread_cond_wait(&emptyCond, &global);
            }
        }
    }
    if(strcmp(policy,"dynamic") == 0){
        if(waitingQueueSize + workingQueueSize == queueSize) {
            ignore = true;
            if (queueSize < maxQueueSize) (*queueSizePtr)++;
        }
    }
    if(strcmp(policy,"random") == 0) {
        if((waitingQueueSize + workingQueueSize >= queueSize)){
            dropRandom();
        }
    }
    if(!ignore) {
        _push(waitingQueue,newRequest);
        waitingQueueSize++;
        pthread_cond_signal(&waitingCond);
    }else{
        Close(newRequest.fd);
    }
    pthread_mutex_unlock(&global);
}



void getargs(int *port, int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    //./server [portnum] [threads] [queue_size] [schedalg] [max_size(optional)]
    //Preparing Arguments.
    getargs(&port, argc, argv);
    workingQueue = initQueue();
    waitingQueue = initQueue();
    int threadsNum = atoi(argv[2]);
    int queueSize = atoi(argv[3]);
    char* policy =malloc(strlen(argv[4])*sizeof(char));
    strcpy(policy,argv[4]);

    int queueMaxSize =(strcmp(policy,"dynamic") == 0) ? atoi(argv[5]): queueSize;
    pthread_t* threadsArr = (pthread_t*)malloc(threadsNum*sizeof(pthread_t));
    //Spawning Threads.
    threadInfo* threadInfoArr =malloc(threadsNum*sizeof(threadInfo));
    for(int i=0;i<threadsNum;++i){
        threadInfoArr[i].thisThread = &threadsArr[i];
        threadInfoArr[i].id= i;
        threadInfoArr[i].count = 0;
        threadInfoArr[i].dynamicCount = 0;
        threadInfoArr[i].staticCount = 0;
        pthread_create(&threadsArr[i],NULL,requestHandlerWorker,(void*)&threadInfoArr[i]);
    }

    //Request Accepting
    listenfd = Open_listenfd(port);
    while(1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t *) &clientlen);//Connceting to the next client.
        requestInfo newRequest;
        struct timezone t;
        gettimeofday(&newRequest.arrivingTime,&t);
        newRequest.fd =connfd;
        addRequest(&queueSize,policy,queueMaxSize, newRequest);
    }
    destroyQueue(waitingQueue);
    destroyQueue(workingQueue);
    free(threadsArr);
}


    


 
