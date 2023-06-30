
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void* smalloc(size_t size){

    //Failure
    //size == 0 || size > 10^8
    if (size == 0 || size > 10^8)
    {
        return NULL;
    }
    
    //try to allocoate -size- bytes
    void* previousBreak = sbrk(0);
    void* status = sbrk(size);


    if(status == reinterpret_cast<void*>(-1)){
        //sbrk() fails
        return NULL;
    }

    return status;
    //success
    //return pointer

}