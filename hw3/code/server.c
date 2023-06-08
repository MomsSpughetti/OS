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


void pushTail(int connfd, int maxQueueSize){

    while (waitingQueueSize = maxQueueSize){
        pthread_cond_wait(&waitingCond, &waitingQueueLock);
    }

}

void pushDynamic(int connfd, int *buffers, int maxBuffers){
    if(*buffers < maxBuffers){
        //no need for this as explained
        //push(&waitingQueue, connfd, &waitingCond, &waitingQueueLock, &waitingQueueSize);
        (*buffers)++;
    } else {
        //dropTail();
    }
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


int ceil(double num){
    int toint = num;
    double a = num - toint;
    if(a < 0) return toint + 1;
    else return num;
}
void dropRandom(int connfd, int *wqSize){
    pthread_mutex_lock(&waitingQueueLock);


    int half = ceil(0.5 * *wqSize);
    int *toDrop = malloc(sizeof(int)*half);
    int *used = malloc(sizeof(int)*(*wqSize));

    //choose who to delete randomly
    for (int i = 0; i < half; i++)
    {
        toDrop[i] = rand() % *wqSize;
        while(used[toDrop[i]]==1)
            toDrop[i] = rand() % *wqSize;
        used[toDrop[i]] = 1;
    }
    
    mergeSort(&toDrop, half);
    Node *curr = waitingQueue->head, *tmp; // will the queue be initialized before? if not then it's segmentation fault
    int wqIndex = 1;
    int toDropIndex = 0;

//delete half of the wq
    while (curr->next)
    {
        if(toDrop[toDropIndex] == wqIndex){
            tmp = curr;
            curr->prev->next = curr->next;
            curr->next->prev = curr->prev;
            free(tmp);
            toDropIndex++;
        }
        wqIndex++;
        curr = curr->next;

    }

    //if first node was chosen
    if(toDrop[0]==0){
        tmp = waitingQueue->head;
        waitingQueue->head = waitingQueue->head->next;
        free(tmp);
    }

    //if last node was chosen
    if(toDrop[half-1]=wqSize){
        tmp = waitingQueue->last;
        waitingQueue->last = waitingQueue->last->prev;
        free(tmp);
    }

    *wqSize = half; //update wq size
    pthread_cond_signal(&waitingCond);
    pthread_mutex_unlock(&waitingQueueLock);
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    //Preparing Arguments.
    getargs(&port, argc, argv);
    workingQueue = initQueue();
    waitingQueue = initQueue();
    int threadsNum = atoi(argv[2]);
    int Buffers = argv[3];
    int MaxBuffers = (argv[4])? argv[4] : -1;
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


    


 
