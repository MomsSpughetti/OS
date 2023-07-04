
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

MallocMetadata* malloc_meta_head = NULL;
memoryInfo mm_info{0,0,0,0};

typedef struct MallocMetadata
{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
} MallocMetadata;

typedef struct memoryInfo
{
    int freeBlocksCount; //done
    size_t totalFreeBytes;//
    int totalBlocksCount;
    size_t totaBytes;
} memoryInfo;


void _removeMetaListTail(){
    MallocMetadata* curr = malloc_meta_head;

    if(!curr){return;}
    if(!curr->next){malloc_meta_head = nullptr;}
    while (curr->next->next)
    {
        curr = curr->next;
    }
    //curr is now pointing to one befoe last
    curr->next = nullptr;
}

void* _getEffectiveSpaceAddr(MallocMetadata* metaDataObj){
    return reinterpret_cast<void*>(reinterpret_cast<char*>(metaDataObj) + sizeof(MallocMetadata));
}

void* _getMetaDataAddr(void* effData){
    return reinterpret_cast<void*>(reinterpret_cast<char*>(effData) - sizeof(MallocMetadata));
}

void _metaDataMemcpy(MallocMetadata metadata, void* dest){
    MallocMetadata* newMeta = static_cast<MallocMetadata*>(dest);
    newMeta->is_free = metadata.is_free;
    newMeta->size = metadata.size;
    newMeta->next = metadata.next;
    newMeta->prev = metadata.prev;
}

/*append to the list*/
void _insertToMetaList(MallocMetadata* newMetaData){
    if(!malloc_meta_head){
        malloc_meta_head=newMetaData;
        return;
    }
    MallocMetadata* curr = malloc_meta_head;
    /*get tail*/
    while (curr->next)
    {
        curr = curr->next;
    }
    
    curr->next = newMetaData;
    newMetaData->prev = curr;
}

/*
#increases heap space with metadata and size
#returns pointer (void*) to the start of the effective space
*/
void* _allocNew(size_t size){

    //try to allocoate -size- bytes
    void* status_meta = sbrk(sizeof(malloc_meta_head));

    if(status_meta == reinterpret_cast<void*>(-1)){ //why not (void*)(-1)
        //sbrk() fails
        return nullptr;
    }

    MallocMetadata newMeta{size, false, nullptr, nullptr};
    _metaDataMemcpy(newMeta, status_meta); //you can use std::memmove instead
    _insertToMetaList((MallocMetadata*)status_meta);

    void* status = sbrk(size);
    if(status == reinterpret_cast<void*>(-1)){
        //deallocate metadata
        sbrk(-1 * sizeof(MallocMetadata));
        _removeMetaListTail();
        return nullptr;
    }
    mm_info.totalBlocksCount++;
    mm_info.totaBytes += size + sizeof(MallocMetadata);
    return status;
    //success
    //return pointer
}

void* smalloc(size_t size){

    //Failure
    //size == 0 || size > 10^8
    if (size == 0 || size > 10^8)
    {
        return nullptr;
    }
    
    /*search for a suitable freed place in the list*/
    MallocMetadata* target = malloc_meta_head;
    while (target)
    {
        //if free
        //and size is enough
        //get a pointer for the effective space
        if (target->is_free && target->size >= size)
        {
            //return (void*)(target + sizeof(MallocMetadata));
            target->is_free = true;
            target->size = size;
            return _getEffectiveSpaceAddr(target);
            /*
            why casting to char*:
            adding a value to a pointer increments the pointer by the number of elements of the pointed-to type.
            */
        }
        target = target->next;
    }
    
    /*does not fit => increase heap space*/
    return _allocNew(size);
}

void* scalloc(size_t num, size_t size){
    //Failure
    //size == 0 || size > 10^8
    if (size == 0 || size > 10^8)
    {
        return nullptr;
    }

    MallocMetadata* target = malloc_meta_head;
    while (target)
    {
        //if free
        //and size is enough
        //get a pointer for the effective space
        if (target->is_free && target->size >= size)
        {
            //return (void*)(target + sizeof(MallocMetadata));
            target->is_free = true;
            target->size = size;
            std::memset(_getEffectiveSpaceAddr(target), 0, target->size); //should I do this? or check if they are zeros
            return _getEffectiveSpaceAddr(target);
            /*
            why casting to char*:
            adding a value to a pointer increments the pointer by the number of elements of the pointed-to type.
            */
        }
        target = target->next;
    }
    /*block not found*/
    void* newAlloc = _allocNew(num*size);
    std::memset(newAlloc, 0, num*size);
    return newAlloc;
}

/*Merge function*/
void _merge(MallocMetadata* metaData){

}

void sfree(void* p){
    if (!p){return;}
    
    MallocMetadata* curr = malloc_meta_head;
    while (curr)
    {
        if(_getEffectiveSpaceAddr(curr) == p){
            curr->is_free = true;
            mm_info.freeBlocksCount++;
            mm_info.totalFreeBytes += curr->size;
            return;
        }
        curr = curr->next;
    }
    
}

void* srealloc(void* oldp, size_t size){
    //Failure
    //size == 0 || size > 10^8
    if (size == 0 || size > 10^8)
    {
        return nullptr;
    }
    MallocMetadata* metaDataPtr = (MallocMetadata*)_getMetaDataAddr(oldp);
    if (metaDataPtr->is_free && metaDataPtr->size >= size)
    {
        std::memmove(_getEffectiveSpaceAddr(metaDataPtr), oldp, size);
        return _getEffectiveSpaceAddr(metaDataPtr);
    }

    //needs a new allocation
    //free the previous
    metaDataPtr->is_free = true;
    mm_info.freeBlocksCount++;
    mm_info.totalFreeBytes += metaDataPtr->size;
    //use smalloc
    void* newAlloc = smalloc(size);
}

size_t _num_free_blocks(){
    return mm_info.freeBlocksCount;
}
size_t _num_free_bytes(){
    return mm_info.totalFreeBytes;
}
size_t _num_allocated_blocks(){
    return mm_info.totalBlocksCount;
}

size_t _getTotalMetaBytes(){
    return sizeof(MallocMetadata)*mm_info.totalBlocksCount;
}
size_t _num_allocated_bytes(){
    return mm_info.totaBytes - _getTotalMetaBytes();
}
size_t _num_meta_data_bytes(){
    return _getTotalMetaBytes();
}
size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}