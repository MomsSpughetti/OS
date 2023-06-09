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






#define POP_WAITING_QUEUE pop(waitingQueue,&waitingCond,&waitingQueueLock,&waitingQueueSize,&emptyCond, &fullCond)
#define POP_WORKING_QUEUE pop(workingQueue,&workingCond,&workingQueueLock,&workingQueueSize, &canPush)

#define PUSH_WAITING_QUEUE(connfd) push(waitingQueue,(connfd),&waitingCond,&waitingQueueLock,&waitingQueueSize)
#define PUSH_WORKING_QUEUE(connfd) push(workingQueue,(connfd),&workingCond,&workingQueueLock,&workingQueueSize)

#define DELETE_NODE_WAITING_QUEUE(connfd) deleteNode(waitingQueue,(connfd),&waitingCond,&waitingQueueLock,&waitingQueueSize)
#define DELETE_NODE_WORKING_QUEUE(connfd) deleteNode(workingQueue, (connfd) ,&workingCond,&workingQueueLock,&workingQueueSize,&emptyCond, &fullCond)





//Global Vars
pthread_mutex_t waitingQueueLock = PTHREAD_MUTEX_INITIALIZER,workingQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waitingCond =PTHREAD_COND_INITIALIZER,workingCond = PTHREAD_COND_INITIALIZER
, fullCond =PTHREAD_COND_INITIALIZER, emptyCond =PTHREAD_COND_INITIALIZER;
int waitingQueueSize = 0,workingQueueSize =0;
Queue* waitingQueue = NULL;
Queue* workingQueue = NULL;

void* requestHandlerWorker(void* args){
    //printf("Waiting for a new requseet\n");
    while(1){
        threadInfo* thread = (threadInfo*)args;
        requestInfo request = POP_WAITING_QUEUE;
        struct timezone t;
        gettimeofday(&request.startingTime,&t);
        request.thread = thread;
        PUSH_WORKING_QUEUE(request);
        requestHandle(request);
        DELETE_NODE_WORKING_QUEUE(request);
        Close(request.fd);
    }
    return NULL;
}

void merge(int arr[], int left[], int leftSize, int right[], int rightSize) {
    int i = 0; // Index for left subarray
    int j = 0; // Index for right subarray
    int k = 0; // Index for merged array

    while (i < leftSize && j < rightSize) {
        if (left[i] <= right[j]) {
            arr[k] = left[i];
            i++;
        } else {
            arr[k] = right[j];
            j++;
        }
        k++;
    }

    // Copy the remaining elements of left[], if any
    while (i < leftSize) {
        arr[k] = left[i];
        i++;
        k++;
    }

    // Copy the remaining elements of right[], if any
    while (j < rightSize) {
        arr[k] = right[j];
        j++;
        k++;
    }
}

void mergeSort(int arr[], int size) {
    if (size < 2) {
        return; // Base case: Array of size 1 is already sorted
    }

    int mid = size / 2;
    int left[mid];
    int right[size - mid];

    // Split the array into left and right subarrays
    for (int i = 0; i < mid; i++) {
        left[i] = arr[i];
    }
    for (int i = mid; i < size; i++) {
        right[i - mid] = arr[i];
    }

    // Recursively sort the left and right subarrays
    mergeSort(left, mid);
    mergeSort(right, size - mid);

    // Merge the sorted subarrays
    merge(arr, left, mid, right, size - mid);
}

int getHalf(int x){
    int a = ((x%2) == 0) ? 0 : 1;
    return x/2 +a;
}

void dropRandom(int *wqSizePtr){
    int wqSize = *wqSizePtr;
    int half = getHalf(wqSize);
    int *toDrop =(int*)malloc(sizeof(int)*half);
    int *used = (int*)malloc(sizeof(int)*(wqSize));

    //choose who to delete randomly
    for (int i = 0; i < half; i++)
    {
        toDrop[i] = rand() % wqSize;
        while(used[toDrop[i]]==1)
            toDrop[i] = rand() % wqSize;
        used[toDrop[i]] = 1;
    }

    mergeSort(toDrop, half);
    Node *curr = waitingQueue->head, *tmp; // will the queue be initialized before? if not then it's segmentation fault
    int waitingQueueIndex = 0;
    int toDropIndex = 0;

//delete half of the wq
    if(toDrop[0]==0){
        _pop(waitingQueue);
        waitingQueueIndex++;
        toDropIndex++;
        curr = curr->next;
    }
    while (curr != waitingQueue->last)
    {
        if(toDrop[toDropIndex] == waitingQueueIndex){
            tmp = curr;
            curr->prev->next = curr->next;
            curr->next->prev = curr->prev;
            free(tmp);
            toDropIndex++;
            waitingQueue->size--;
        }
        waitingQueueIndex++;
        curr = curr->next;
    }
    if(toDrop[half-1]==wqSize){
        _popBack(waitingQueue);
    }
    *wqSizePtr = half; //update wq size
}

void addRequest(int* queueSizePtr,char* policy, int maxQueueSize, requestInfo newRequest) {
    pthread_mutex_lock(&waitingQueueLock);
    pthread_mutex_lock(&workingQueueLock);
    bool ignore = false;
    int queueSize = *queueSizePtr;
    printf("%d\n", waitingQueueSize + workingQueueSize);
    if(strcmp(policy,"block") == 0){
        printf("\nStuck...\n");
        while(waitingQueueSize + workingQueueSize >= queueSize) {
            printf("%d\n", waitingQueueSize + workingQueueSize);
            pthread_cond_wait(&fullCond, &waitingQueueLock);
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
            while(waitingQueueSize + workingQueueSize !=0){
                pthread_cond_wait(&emptyCond, &waitingQueueLock);
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
        if((waitingQueueSize + workingQueueSize == queueSize)){
            dropRandom(&waitingQueueSize);
        }
    }
    if(!ignore) {
        _push(waitingQueue,newRequest);
        waitingQueueSize++;
        pthread_cond_signal(&waitingCond);
    }else{
        Close(newRequest.fd);
    }
    pthread_mutex_unlock(&workingQueueLock);
    pthread_mutex_unlock(&waitingQueueLock);
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


    


 
