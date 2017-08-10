//
// Jisp - main.cpp
//
// Copyright (C) 1998-2017, James Anhalt III, All Rights Reserved
//

#define NIL ((NODE *)0)

#define PAGE_SIZE (64 * 1024)

union PAGE {
    char data[PAGE_SIZE];
    struct {
        int     type;
        PAGE    *next;
    } head;
};

union NODE {

    int type() const { return ((const PAGE*)0 + ((const PAGE*)this - (const PAGE*)0))->head.type; }
    bool type(int t) { return this != NIL && type() == t; }

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

NODE *nodeGet(int type);
