//
// Jisp - main.cpp
//
// Copyright (C) 1998-2017, James Anhalt III, All Rights Reserved
//

#define NIL ((NODE *)0)

struct NODE {
    unsigned int type() const { return (*(unsigned int *)((long long)this & ~0x0ffff)); }
    //unsigned int type() const { return (*(unsigned int *)((char *)this - ((int)this & 0x0ffff))); }
    bool type(int t) { return this != NIL && type() == t; }

    union {
        struct {
            NODE *next;
        } free;

        struct {
            NODE  *car;
            NODE  *cdr;
        } list;

        struct {
            VNAME  text;
        } text;

        struct {
            int    data;
        } math;

        struct {
            NODE  *text;
            NODE  *data;
            NODE  *next;
        } name;

        struct {
            NODE   *text;
            NODE *(*code)(int argc, NODE *argv[]);
            NODE   *next;
        } call;

        struct {
            NODE  *text;
            NODE  *code;
            NODE  *names;
            NODE  *binds;
        } user;

        struct {
            NODE  *name;
            NODE  *data;
            NODE  *next;
        } bind;

        struct {
            VFILE  file;
        } file;

        struct {
            NODE  *info;
            NODE  *next;
        } fail;

        struct {
            NODE *nodes[64];
        } hash;
    };
};    

enum {
    NT_FREE = 0,
    
    NT_LIST,
    NT_TEXT,
    NT_MATH,

    NT_NAME,
    
    NT_CALL,
    NT_CALL_FORM,

    NT_USER,
    NT_USER_FORM,

    NT_BIND,
    NT_FILE,
    NT_FAIL,
    NT_HASH,

    NT_LAST
};

extern NODE *nodeGet(unsigned int type);
