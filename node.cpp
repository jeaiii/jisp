//
// Jisp - main.cpp
//
// Copyright (C) 1998-2017, James Anhalt III, All Rights Reserved
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "main.h"
#include "node.h"

#define PAGE_SIZE (64 * 1024)

struct PAGE {
    union {
        char data[PAGE_SIZE];
        struct {
            unsigned int type;
            PAGE        *next;
        } head;        
    };
};

struct TYPE {
   ULONG  size;
   ULONG  node;

   ULONG  used;
   NODE  *free;
   PAGE  *page;
};

#define NODESIZE(t) (sizeof(((NODE *)0)->t))

TYPE types[NT_LAST] = {
    { NODESIZE(free) },
    { NODESIZE(list) },
    { NODESIZE(text) },
    { NODESIZE(math) },
    { NODESIZE(name) },
    { NODESIZE(call) },
    { NODESIZE(call) },
    { NODESIZE(user) },
    { NODESIZE(user) },
    { NODESIZE(bind) },
    { NODESIZE(file) },
    { NODESIZE(fail) },
    { NODESIZE(hash) },
};

NODE *nodeGet(unsigned int type)
{
    if (type >= NT_LAST)
        return 0;

    types[type].used += 1;

    if (types[type].free != 0) {
        NODE *node = types[type].free;
        types[type].free = node->free.next;
        node->free.next = 0;
        return (node);
    }

    if (types[type].node == 0) {
        PAGE *page = (PAGE *)VirtualAllocEx(GetCurrentProcess(), NULL, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
        if (page == 0)
            return 0;

        page->head.type  = type;
        page->head.next  = types[type].page;
        types[type].page = page;
        types[type].node = sizeof(page->head);
    };

    NODE *node = (NODE *)&types[type].page->data[types[type].node];

    types[type].node += types[type].size;
    if ((types[type].node + types[type].size) > PAGE_SIZE) types[type].node = 0;
    
    return (node);
}