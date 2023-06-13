#include "segel.h"

/************************** 
 * Error-handling functions
 **************************/
/* $begin errorfuns */
/* $begin unixerror */


requestInfo errorValue(){
    requestInfo r;
    r.fd = -1;
    r.thread = NULL;
    return r;
}

Queue *initQueue() {
    Queue *newQueue = malloc(sizeof(Queue));
    if (newQueue == NULL) {
        return NULL;
    }
    newQueue->size = 0;
    newQueue->head = newQueue->last = NULL;
    return newQueue;
}

QueueStatus _push(Queue* queue,T val){
    if(queue == NULL){
        return NULL_ARG;
    }
    Node* newNode = (Node*)malloc(sizeof(Node));
    if(newNode == NULL){
        return MALLOC_FAIL;
    }
    if(queue->head ==NULL){
        queue->head =queue->last = newNode;

    }else{
        queue->last->next = newNode;
        newNode->prev = queue->last;
        queue->last = newNode;
    }
    newNode->next = NULL;
    newNode->data = val;
    queue->size++;
    return SUCCESS;
}


T _pop(Queue* queue){
    if(queue == NULL){
        return errorValue();
    }
    if(queue->size == 0){ // a thread needs to sleep until the condition var is bigger than 0
        return errorValue();
    }
    T val = queue->head->data;
    if(queue->head == queue->last){ // N1->NULL => NULL
        free(queue->head);
        queue->head= queue->last = NULL;
    }else{
        Node* newHead = queue->head->next;
        newHead->prev = NULL;
        free(queue->head);
        queue->head= newHead;
    }
    queue->size--;
    return val;
}

T removeNode(Queue* queue, Node* node){
    if(queue == NULL || queue->size ==0 || node == NULL){
        return errorValue();
    }
    T result = node->data;
    if(node->prev ==NULL){//first
        if(node->next ==NULL){//and last
            queue->head = queue->last = NULL;
        }else{
            node->next->prev = NULL;
            queue->head = node->next;
        }
    }else{//last
        if(node->next ==NULL){
            if(node->prev ==NULL){//and last
                queue->head = queue->last = NULL;
            }else{
                node->prev->next = NULL;
                queue->last = node->prev;
            }
        }else{
            node->next->prev =node->prev;
            node->prev->next = node->next;
        }
    }
    queue->size--;
    free(node);
    return result;
}

T _popBack(Queue* queue){
    if(queue == NULL){
        return errorValue();
    }
    if(queue->size == 0){ // a thread needs to sleep until the condition var is bigger than 0
        return errorValue();
    }
    T val = queue->last->data;
    if(queue->head == queue->last){ // N1->NULL => NULL
        free(queue->head);
        queue->head= queue->last = NULL;
    }else{
        Node* newPrev = queue->last->prev;
        newPrev->next = NULL;
        free(queue->last);
        queue->last=newPrev ;
    }
    queue->size--;
    return val;
}

QueueStatus destroyQueue(Queue* queue){
    if(queue == NULL){
        return NULL_ARG;
    }
    Node* curr = queue->head;
    while(curr != NULL){
        Node* temp = curr->next;
        free(curr);
        curr = temp;
    }
    return SUCCESS;
}


QueueStatus _deleteNode(Queue* queue,T data){
    if(queue == NULL){
        return NULL_ARG;
    }
    if(queue->head->data.fd == data.fd){
        _pop(queue);
        return SUCCESS;
    }
    Node* curr = queue->head;
    while(curr !=NULL){
        if(curr->data.fd == data.fd){
            break;
        }
        curr=curr->next;
    }
    if(curr->next ==NULL){//the last
        Node* newLast = queue->last->prev;
        newLast->next = NULL;
        free(queue->last);
        queue->last= newLast;
        return SUCCESS;
    }
    curr->prev->next = curr->next;
    curr->next->prev = curr->prev;
    free(curr);
    return SUCCESS;
}



QueueStatus push(Queue* queue,T val,pthread_cond_t* c, pthread_mutex_t* m, int* condVar){
    if(queue == NULL || c == NULL || m == NULL || condVar == NULL){
        printf("\nNULL\n");
        return NULL_ARG;
    }
    pthread_mutex_lock(m);
    QueueStatus status = _push(queue, val);
    (*condVar)++;
    pthread_cond_signal(c);
    pthread_mutex_unlock(m);
    return status;
}


T pop(Queue* queue,pthread_cond_t* c, pthread_mutex_t* m, int* condVar,pthread_cond_t* empty, pthread_cond_t* full){
    if(queue == NULL || c == NULL || m == NULL || condVar == NULL){
        return errorValue();
    }
    pthread_mutex_lock(m);
    while(*condVar == 0) pthread_cond_wait(c,m);
    T data = _pop(queue);
    (*condVar)--;
    pthread_cond_signal(full);
    if(*condVar == 0) pthread_cond_signal(empty);
    pthread_mutex_unlock(m);
    return data;
}

T popBack(Queue* queue,pthread_cond_t* c, pthread_mutex_t* m, int* condVar,pthread_cond_t* empty, pthread_cond_t* full){
    if(queue == NULL || c == NULL || m == NULL || condVar == NULL){
        return errorValue();
    }
    pthread_mutex_lock(m);
    while(*condVar == 0) pthread_cond_wait(c,m);
    T data = _popBack(queue);
    (*condVar)--;
    pthread_cond_signal(full);
    if(*condVar == 0) pthread_cond_signal(empty);
    pthread_mutex_unlock(m);
    return data;
}


QueueStatus deleteNode(Queue* queue,T fd,pthread_cond_t* c, pthread_mutex_t* m, int* condVar,pthread_cond_t* empty, pthread_cond_t* full) {
    if(queue == NULL || c == NULL || m == NULL || condVar == NULL){
        return NULL_ARG;
    }
    pthread_mutex_lock(m);
    while(*condVar == 0) pthread_cond_wait(c,m);
    _deleteNode(queue,fd);
    (*condVar)--;
 //   printf("section 1\n");
    pthread_cond_signal(full);
  //  printf("section 2\n");
    if(*condVar == 0){
   //     printf("section 3\n");
        pthread_cond_signal(empty);
    //    printf("section 4\n");
    }
  //  printf("Trying to Unlock\n");
    pthread_mutex_unlock(m);
  //  printf("Unlocked\n");
    return SUCCESS;
}


void unix_error(char *msg) /* unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */

void posix_error(int code, char *msg) /* posix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
}

void dns_error(char *msg) /* dns-style error */
{
    fprintf(stderr, "%s: DNS error %d\n", msg, h_errno);
    exit(0);
}

void app_error(char *msg) /* application error */
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}
/* $end errorfuns */


int Gethostname(char *name, size_t len)
{
    int rc;

    if ((rc = gethostname(name, len)) < 0)
        unix_error("Setenv error");
    return rc;
}

int Setenv(const char *name, const char *value, int overwrite)
{
    int rc;

    if ((rc = setenv(name, value, overwrite)) < 0)
        unix_error("Setenv error");
    return rc;
}

/*********************************************
 * Wrappers for Unix process control functions
 ********************************************/

/* $begin forkwrapper */
pid_t Fork(void)
{
    pid_t pid;

    if ((pid = fork()) < 0)
        unix_error("Fork error");
    return pid;
}
/* $end forkwrapper */

void Execve(const char *filename, char *const argv[], char *const envp[])
{
    if (execve(filename, argv, envp) < 0)
        unix_error("Execve error");
}

/* $begin wait */
pid_t Wait(int *status)
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
        unix_error("Wait error");
    return pid;
}

pid_t WaitPid(pid_t pid, int *status, int options)
{
    if ((pid = waitpid(pid, status, options)) < 0) unix_error("Wait error");
    return pid;
}


/* $end wait */

/********************************
 * Wrappers for Unix I/O routines
 ********************************/

int Open(const char *pathname, int flags, mode_t mode)
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
        unix_error("Open error");
    return rc;
}

ssize_t Read(int fd, void *buf, size_t count)
{
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0)
        unix_error("Read error");
    return rc;
}

ssize_t Write(int fd, const void *buf, size_t count)
{
    ssize_t rc;

    if ((rc = write(fd, buf, count)) < 0)
        unix_error("Write error");
    return rc;
}

off_t Lseek(int fildes, off_t offset, int whence)
{
    off_t rc;

    if ((rc = lseek(fildes, offset, whence)) < 0)
        unix_error("Lseek error");
    return rc;
}

void Close(int fd)
{
    int rc;

    if ((rc = close(fd)) < 0)
        unix_error("Close error");
}

int Select(int  n, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout)
{
    int rc;

    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
        unix_error("Select error");
    return rc;
}

int Dup2(int fd1, int fd2)
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
        unix_error("Dup2 error");
    return rc;
}

void Stat(const char *filename, struct stat *buf)
{
    if (stat(filename, buf) < 0)
        unix_error("Stat error");
}

void Fstat(int fd, struct stat *buf)
{
    if (fstat(fd, buf) < 0)
        unix_error("Fstat error");
}

/***************************************
 * Wrappers for memory mapping functions
 ***************************************/
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    void *ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1))
        unix_error("mmap error");
    return(ptr);
}

void Munmap(void *start, size_t length)
{
    if (munmap(start, length) < 0)
        unix_error("munmap error");
}

/**************************** 
 * Sockets interface wrappers
 ****************************/

int Socket(int domain, int type, int protocol)
{
    int rc;

    if ((rc = socket(domain, type, protocol)) < 0)
        unix_error("Socket error");
    return rc;
}

void Setsockopt(int s, int level, int optname, const void *optval, int optlen)
{
    int rc;

    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
        unix_error("Setsockopt error");
}

void Bind(int sockfd, struct sockaddr *my_addr, int addrlen)
{
    int rc;

    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
        unix_error("Bind error");
}

void Listen(int s, int backlog)
{
    int rc;

    if ((rc = listen(s,  backlog)) < 0)
        unix_error("Listen error");
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
        unix_error("Accept error");
    return rc;
}

void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen)
{
    int rc;

    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
        unix_error("Connect error");
}

/************************
 * DNS interface wrappers 
 ***********************/

/* $begin gethostbyname */
struct hostent *Gethostbyname(const char *name)
{
    struct hostent *p;

    if ((p = gethostbyname(name)) == NULL)
        dns_error("Gethostbyname error");
    return p;
}
/* $end gethostbyname */

struct hostent *Gethostbyaddr(const char *addr, int len, int type)
{
    struct hostent *p;

    if ((p = gethostbyaddr(addr, len, type)) == NULL)
        dns_error("Gethostbyaddr error");
    return p;
}

/*********************************************************************
 * The Rio package - robust I/O functions
 **********************************************************************/
/*
 * rio_readn - robustly read n bytes (unbuffered)
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nread = read(fd, bufp, nleft)) < 0) {
            if (errno == EINTR) /* interrupted by sig handler return */
                nread = 0;      /* and call read() again */
            else
                return -1;      /* errno set by read() */
        }
        else if (nread == 0)
            break;              /* EOF */
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readn */

/*
 * rio_writen - robustly write n bytes (unbuffered)
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR)  /* interrupted by sig handler return */
                nwritten = 0;    /* and call write() again */
            else
                return -1;       /* errorno set by write() */
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */


/* 
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* refill if buf is empty */
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf,
                           sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0) {
            if (errno != EINTR) /* interrupted by sig handler return */
                return -1;
        }
        else if (rp->rio_cnt == 0)  /* EOF */
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}
/* $end rio_read */

/*
 * rio_readinitb - Associate a descriptor with a read buffer and reset buffer
 */
/* $begin rio_readinitb */
void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}
/* $end rio_readinitb */

/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nread = rio_read(rp, bufp, nleft)) < 0) {
            if (errno == EINTR) /* interrupted by sig handler return */
                nread = 0;      /* call read() again */
            else
                return -1;      /* errno set by read() */
        }
        else if (nread == 0)
            break;              /* EOF */
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readnb */

/* 
 * rio_readlineb - robustly read a text line (buffered)
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0) {
            if (n == 1)
                return 0; /* EOF, no data read */
            else
                break;    /* EOF, some data was read */
        } else
            return -1;    /* error */
    }
    *bufp = 0;
    return n;
}
/* $end rio_readlineb */

/**********************************
 * Wrappers for robust I/O routines
 **********************************/
ssize_t Rio_readn(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
        unix_error("Rio_readn error");
    return n;
}

void Rio_writen(int fd, void *usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n)
        unix_error("Rio_writen error");
}

void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
}

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
        unix_error("Rio_readnb error");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
        unix_error("Rio_readlineb error");
    return rc;
}

/******************************** 
 * Client/server helper functions
 ********************************/
/*
 * open_clientfd - open connection to server at <hostname, port> 
 *   and return a socket descriptor ready for reading and writing.
 *   Returns -1 and sets errno on Unix error. 
 *   Returns -2 and sets h_errno on DNS (gethostbyname) error.
 */
/* $begin open_clientfd */
int open_clientfd(char *hostname, int port)
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1; /* check errno for cause of error */

    /* Fill in the server's IP address and port */
    if ((hp = gethostbyname(hostname)) == NULL)
        return -2; /* check h_errno for cause of error */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);

    /* Establish a connection with the server */
    if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    return clientfd;
}
/* $end open_clientfd */

/*  
 * open_listenfd - open and return a listening socket on port
 *     Returns -1 and sets errno on Unix error.
 */
/* $begin open_listenfd */
int open_listenfd(int port)
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket failed\n");
        return -1;
    }

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0) {
        fprintf(stderr, "setsockopt failed\n");
        return -1;
    }

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0) {
        fprintf(stderr, "bind failed\n");
        return -1;
    }

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0) {
        fprintf(stderr, "listen failed\n");
        return -1;
    }
    return listenfd;
}
/* $end open_listenfd */

/******************************************
 * Wrappers for the client/server helper routines 
 ******************************************/
int Open_clientfd(char *hostname, int port)
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0) {
        if (rc == -1)
            unix_error("Open_clientfd Unix error");
        else
            dns_error("Open_clientfd DNS error");
    }
    return rc;
}

int Open_listenfd(int port)
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
        unix_error("Open_listenfd error");
    return rc;
}


