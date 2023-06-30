#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "malloc_2.h"

void insertToMetaList(MallocMetadata* newMetaData){
    MallocMetadata* curr = malloc_meta_head;
    /*list is empty*/
    if(!curr){
        malloc_meta_head = newMetaData;
        return;
    }

    /*newMetaData should be head*/
    if(newMetaData < curr){
        malloc_meta_head = newMetaData;
        newMetaData->next = curr;
        curr->prev = newMetaData;
        return;
    }

    /*else*/
    curr = curr->next;
    while (curr->next)
    {
        /*address fits in here regarding addresses order?*/
        if(newMetaData >= curr && curr->next && newMetaData < curr->next){
            MallocMetadata* temp = curr->next;
            curr->next = newMetaData;
            newMetaData->prev = curr;
            newMetaData->next = temp;
            temp->prev = newMetaData;
            return;
        } else {
            curr = curr->next;
        }
    }

    /*newMetaData should be the tail*/
    curr->next = newMetaData;
    newMetaData->prev = curr;
}
