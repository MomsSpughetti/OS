
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "malloc_2.h"

void removeMetaListTail(){
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

void* getEffectiveSpaceAddr(MallocMetadata* metaDataObj){
    return reinterpret_cast<void*>(reinterpret_cast<char*>(metaDataObj) + sizeof(MallocMetadata));
}

void* getMetaDataAddr(void* effData){
    return reinterpret_cast<void*>(reinterpret_cast<char*>(effData) - sizeof(MallocMetadata));
}

void metaDataMemcpy(MallocMetadata metadata, void* dest){
    MallocMetadata* newMeta = static_cast<MallocMetadata*>(dest);
    newMeta->is_free = metadata.is_free;
    newMeta->size = metadata.size;
    newMeta->next = metadata.next;
    newMeta->prev = metadata.prev;
}

/*append to the list*/
void insertToMetaList(MallocMetadata* newMetaData){
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

/*increases heap space with metadata and size*/
void* allocNew(size_t size){

    //try to allocoate -size- bytes
    void* status_meta = sbrk(sizeof(malloc_meta_head));

    if(status_meta == reinterpret_cast<void*>(-1)){ //why not (void*)(-1)
        //sbrk() fails
        return nullptr;
    }

    MallocMetadata newMeta{size, false, nullptr, nullptr};
    metaDataMemcpy(newMeta, status_meta);
    insertToMetaList((MallocMetadata*)status_meta);

    void* status = sbrk(size);
    if(status == reinterpret_cast<void*>(-1)){
        //deallocate metadata
        sbrk(-1 * sizeof(MallocMetadata));
        removeMetaListTail();
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
            return getEffectiveSpaceAddr(target);
            /*
            why casting to char*:
            adding a value to a pointer increments the pointer by the number of elements of the pointed-to type.
            */
        }
        target = target->next;
    }
    
    /*does not fit => increase heap space*/
    if (!target)
    {
        return allocNew(size);
    }
}

void sfree(void* p){
    if (!p){return;}
    
    MallocMetadata* curr = malloc_meta_head;
    while (curr)
    {
        if(getEffectiveSpaceAddr(curr) == p){
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
    MallocMetadata* metaDataPtr = (MallocMetadata*)getMetaDataAddr(oldp);
    if (metaDataPtr->size >= size)
    {
        return getEffectiveSpaceAddr(metaDataPtr);
    }

    //needs a new allocation
    //free the previous
    metaDataPtr->is_free = true;
    mm_info.freeBlocksCount++;
    mm_info.totalFreeBytes += metaDataPtr->size;
    //use smalloc
    return smalloc(size);
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

size_t getTotalMetaBytes(){
    return sizeof(MallocMetadata)*mm_info.totalBlocksCount;
}
size_t _num_allocated_bytes(){
    return mm_info.totaBytes - getTotalMetaBytes();
}
size_t _num_meta_data_bytes(){
    return getTotalMetaBytes();
}
size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}