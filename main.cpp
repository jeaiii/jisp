//
// Jisp - main.cpp
//
// Copyright (C) 1998-2017, James Anhalt III, All Rights Reserved
//

#include "main.h"
#include "node.h"

//----------------------------------------------------------------
NODE *n_names = NIL;
NODE *n_calls = NIL;
NODE *n_binds = NIL;

NODE *n_quote = NIL;
NODE *n_true = NIL;
NODE *n_unbound = NIL;

NODE  *n_stack_head[1024];
NODE **n_stack_tail = n_stack_head;

NODE  *n_exit = NIL;

//----------------------------------------------------------------
NODE *evaluate(NODE *n);
NODE *print(NODE *n);
NODE *read(void);

NODE *f_getn(int argc, NODE* argv[]);
//----------------------------------------------------------------

NODE *make_node(int type)
{
    NODE *node = nodeGet(type);

    if (!node) {
        vOutText("#<fatal: out of memory>");
        vAppDone(1);
    }
    return (node);
}

NODE *make_list(NODE *car, NODE *cdr)
{
    NODE *node = make_node(NT_LIST);

    node->list.car = car;
    node->list.cdr = cdr;
    return (node);
}

NODE *make_math(int data)
{
    NODE *node = make_node(NT_MATH);

    node->math.data = data;
    return (node);
}

NODE *make_text(const VTEXT text)
{
    NODE *node = make_node(NT_TEXT);

    vTextCopy(node->text.text, text);
    return (node);
}

unsigned int Hash(unsigned char data[]) {
    unsigned int hash = 2166136261;
    while (*data) {
        hash ^= *data++;
        hash *= 16777619;
    }
    return hash;
}

unsigned int HashFast (const unsigned char * data, unsigned int len) {
unsigned int hash = len, tmp;
unsigned int rem;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += *(unsigned short *)(data);
        tmp    = ((*(unsigned short *)(data+2)) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (unsigned short);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += *(unsigned short *)(data);
                hash ^= hash << 16;
                hash ^= data[sizeof(unsigned short)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += *(unsigned short *)(data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

unsigned int HashPerl(unsigned char k[], unsigned int length, unsigned int initval = 0)
{
  #define mix(a,b,c) \
  { \
    a -= b; a -= c; a ^= (c>>13); \
    b -= c; b -= a; b ^= (a<<8); \
    c -= a; c -= b; c ^= (b>>13); \
    a -= b; a -= c; a ^= (c>>12);  \
    b -= c; b -= a; b ^= (a<<16); \
    c -= a; c -= b; c ^= (b>>5); \
    a -= b; a -= c; a ^= (c>>3);  \
    b -= c; b -= a; b ^= (a<<10); \
    c -= a; c -= b; c ^= (b>>15); \
  }

  typedef unsigned int  ub4;
  typedef unsigned char ub1;

  register ub4 a,b,c,len;

  /* Set up the internal state */
  len = length;
  a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
  c = initval;         /* the previous hash value */

  /*---------------------------------------- handle most of the key */
  while (len >= 12)
  {
    a += (k[0] +((ub4)k[1]<<8) +((ub4)k[2]<<16) +((ub4)k[3]<<24));
    b += (k[4] +((ub4)k[5]<<8) +((ub4)k[6]<<16) +((ub4)k[7]<<24));
    c += (k[8] +((ub4)k[9]<<8) +((ub4)k[10]<<16)+((ub4)k[11]<<24));
    mix(a,b,c);
    k += 12; len -= 12;
  }

  /*------------------------------------- handle the last 11 bytes */
  c += length;
  switch(len)              /* all the case statements fall through */
  {
  case 11: c+=((ub4)k[10]<<24);
  case 10: c+=((ub4)k[9]<<16);
  case 9 : c+=((ub4)k[8]<<8);
  /* the first byte of c is reserved for the length */
  case 8 : b+=((ub4)k[7]<<24);
  case 7 : b+=((ub4)k[6]<<16);
  case 6 : b+=((ub4)k[5]<<8);
  case 5 : b+=k[4];
  case 4 : a+=((ub4)k[3]<<24);
  case 3 : a+=((ub4)k[2]<<16);
  case 2 : a+=((ub4)k[1]<<8);
  case 1 : a+=k[0];
  /* case 0: nothing left to add */
  }
  mix(a,b,c);
  /*-------------------------------------------- report the result */
  return c;
}

NODE *make_name(const VTEXT name)
{ 
//    unsigned int hash = HashFast((unsigned char *)name, vTextLength(name));
    unsigned int hash = Hash((unsigned char *)name);

    NODE **make = &n_names;

    for (;;) {
      NODE *node = *make;

      if (node == NIL) break;

      if (node->type(NT_HASH)) {
        make = &node->hash.nodes[hash & 63];//>> (32-8)];
        //make = &node->hash.nodes[hash >> (32-6)];
        hash = (hash >> (32-8)) | (hash << 8);
      }
      else {
        if (vTextCompare(node->name.text->text.text, name) == 0) return (node);
        make = &node->name.next;
      }
    }

    NODE *node = make_node(NT_NAME);
    
    node->name.text = make_text(name);
    node->name.data = n_unbound;

    node->name.next = NIL;

    *make = node;

    return (node);
}

NODE *make_call(char *text, NODE *(*code)(int argc, NODE **argv), int type)
{
    NODE *node = make_node(type);
    NODE *name = make_name(text);

    name->name.data = node;

    node->call.text = name->name.text;
    node->call.code = code;

    node->call.next = n_calls;
    n_calls = node;

    return (node);   
}

NODE *make_fail(NODE* info, NODE* next) {
    NODE *node = make_node(NT_FAIL);
    node->fail.info = info;
    if (next != 0 && next->type() != NT_FAIL)
        next = make_fail(next, 0);
    node->fail.next = next;
    return node;
}

NODE *make_fail(VTEXT text, NODE *next)
{
    return make_fail(make_text(text), next);
}

NODE *make_fail(VTEXT name, VTEXT text, NODE *next)
{
    if (text != 0)
        next = make_fail(text, next);
    if (name != 0)
        next = make_fail(name, next);
    return next;
}

//----------------------------------------------------------------

static VNAME  readBuffer;
static VCHAR *readPointer = NULL;

int readChar(void)
{
    if (!readPointer || !*readPointer) {
        readPointer = NULL;;
        if (!vGetText(readBuffer)) return (-1);
        readPointer = readBuffer;
    }
    return (*readPointer++);
}

void readCharPut(VCHAR c)
{
    if (readPointer && readPointer != readBuffer) {
        *--readPointer = c;
    }
}

#define EOF (-1)


int kill_white_space(void)
{
    int c = 0;
    while (c != EOF && c <= ' ') c = readChar(); // kill white space
    return (c);
}    


bool ToFrom (int& n, const VTEXT text) {
    n = 0;
    if (!text)
        return false;

    int i = 0;
    int sign = 0;

    if (text[i] == '+')
        i++;
    else if (text[i] == '-') {
        i++;
        sign = -1;
    }

    if (text[i] < '0' || text[i] > '9')
        return false;

    do {
        n = n * 10 + (text[i] - '0');
        i++;
    } while (text[i] >= '0' && text[i] <= '9');

    n = (n ^ sign) - sign;
    return true;

}

NODE *read(void)
{    
    static char buffer[256];
    char *bp = buffer;
    int c;
    NODE *n, *tail, *item;

    c = kill_white_space();
    if (c == EOF) return (NIL);

    if (c == '(' || c == '[') {
        char p = (c == '(') ? ')' : ']';
        c = kill_white_space();
        if (c == EOF) return (NIL);
        if (c == p) return (NIL);

        n = tail = make_node(NT_LIST);
        
        do {
            readCharPut((VCHAR)c);
            item = read();
            tail->list.car = item;
            c = kill_white_space();
            if (c == EOF) return (NIL);
            if (c == '.') {
                tail->list.cdr = read();
                c = kill_white_space();
            }
            else if (c == p) {
                tail->list.cdr = NIL;
            }
            else {
                tail->list.cdr = make_node(NT_LIST);
                tail = tail->list.cdr;
            }
        } while (c != p);
        return (n);
    }
    else if (c == '\'') {
        return (make_list(n_quote, make_list(read(), NIL)));
    }
    else {
        *bp++ = c;
        c = readChar();
        while (c != EOF && c > ' ' && c != '(' && c != ')' && c != '[' && c != ']') {
            *bp++ = c;
            c = readChar();
        }
        *bp = 0;
        readCharPut((VCHAR)c);

        bp = buffer;
        if (*bp == '+' || *bp == '-')
            bp++;

        if(*bp < '0' || *bp >'9')
            return (make_name(buffer));

        for (bp++; *bp; bp++)
            if (*bp < '0' || *bp > '9') return (make_name(buffer));
        
        int i = 0;
        ToFrom(i, buffer);
        return (make_math(i));
    }
}

//----------------------------------------------------------------
void print_hex(unsigned long n) {
    char number[16];
    number[0] = '0';
    number[1] = 'x';
    vTextFromNumberX(&number[2], n);
    vOutText(number);
}

void print_int(int n) {
    char number[16];
    vTextFromNumberD(number, n);
    vOutText(number);
}

void print_aux(NODE *n)
{
    if (n == NIL) vOutText("()");
    else switch(n->type()) {
    case NT_LIST:
        vOutText("(");
        do {
          print_aux(n->list.car);
          n = n->list.cdr;
          if (n != NIL) vOutText(" ");
        } while (n->type(NT_LIST));
        if (n != NIL) {
          vOutText(".");
          vOutText(" ");
          print_aux(n);
        }
        vOutText(")");
        break;
    case NT_NAME:
        vOutText(n->name.text->text.text);
        break;
    case NT_TEXT:
        vOutText("\"");
        vOutText(n->text.text);
        vOutText("\"");
        break;
    case NT_MATH:
        print_int(n->math.data);
        break;
    case NT_CALL:
        vOutText("#<");
        vOutText("call");
        vOutText(" ");
        print_aux(n->call.text);
        vOutText(">");
        break;
    case NT_CALL_FORM:
        vOutText("#<");
        vOutText("form");
        vOutText(" ");
        print_aux(n->call.text);
        vOutText(">");
        break;
    case NT_USER:
        vOutText("#<");
        vOutText("lambda");
        vOutText(" ");
        print_aux(n->user.text);
        vOutText(" ");
        print_aux(n->user.names);
        vOutText(" ");
        print_aux(n->user.code);
        vOutText(" ");
        print_aux(n->user.binds);
        vOutText(">");
        break;
    case NT_USER_FORM:
        vOutText("#<");
        vOutText("macro");
        vOutText(" ");
        print_aux(n->user.text);
        vOutText(" ");
        print_aux(n->user.names);
        vOutText(" ");
        print_aux(n->user.code);
        vOutText(" ");
        print_aux(n->user.binds);
        vOutText(">");
        break;
    case NT_BIND:
        vOutText("#<");
        vOutText("bind");
        do {
            vOutText(" ");
            vOutText("{");
            print_aux(n->bind.name);
            vOutText(" ");
            print_aux(n->bind.data);
            vOutText("}");
            n = n->bind.next;
        } while (n != 0);
        vOutText(">");
        break;
    case NT_FAIL:
        vOutText("#<");
        vOutText("fail");
        do {
          vOutText(" ");
          print_aux(n->fail.info);
          n = n->fail.next;
        } while (n);
        vOutText(">");
        break;
    case NT_HASH:
        vOutText("#<");
        vOutText("hash");
        for (int i = 0; i < 64; i++) {
          vOutText(" ");
          NODE *node = n->hash.nodes[i];
          if (!node->type(NT_NAME)) {
            print_aux(node);
          }
          else {
            vOutText("[");
            print_aux(node);
            for (;;) {
              node = node->name.next;
              if (node == NIL) break;
              if (node->type(NT_NAME)) {
                vOutText(" ");
                print_aux(node);
              }
              else {
                vOutText(":");
                print_aux(node);
                break;
              }
            }
            vOutText("]");
          }
        }
        vOutText(">");
      break;
      default:
        vOutText("#<");
        vOutText("????");
        vOutText(" ");
        print_hex((unsigned long)n->type());
        vOutText(" ");
        print_hex((unsigned long)(long long)n);
        vOutText(">");
        break;
    }
}               

NODE *print(NODE *n)
{
    print_aux(n);
    vOutLine();
    return (n);
}

//----------------------------------------------------------------

void make_bind(NODE *name, NODE *data, NODE *binds)
{
    NODE *node = make_node(NT_BIND);

    node->bind.name = name;
    node->bind.data = data;    
    node->bind.next = binds->list.car;
    
    binds->list.car = node;
}

NODE *bind_parms(NODE *names, NODE *parms, NODE *binds)
{
    binds = make_list(NIL, binds);
    
    while (names != NIL) {
        if (!parms->type(NT_LIST)) return (make_fail("too few args", "bind_parms()", parms));

        make_bind(names->list.car, evaluate(parms->list.car), binds);
        names = names->list.cdr;
        parms = parms->list.cdr;
    }    
    if (parms != NIL) return (make_fail("too many args", "bind_parms()", parms));
    return (binds);
}

int argv_parms(NODE *parms, int type)
{
    int i = 0;

    for (i = 0; parms->type(NT_LIST); i++, parms = parms->list.cdr) {
        *n_stack_tail++ = (type == NT_CALL) ? evaluate(parms->list.car) : parms->list.car;
    }
    return (i);
}    

NODE *set_name_data(NODE *name, NODE *data)
{
    for (NODE *list = n_binds; list != NIL; list = list->list.cdr) {
        for (NODE* bind = list->list.car; bind != NIL; bind = bind->bind.next) {
            if (bind->bind.name == name) {
                NODE* last = bind->bind.data;
                bind->bind.data = data;
                return (last);
            }
        }
    }

    NODE* last = name->name.data;
    name->name.data = data;
    return (last);
}
            
NODE *get_name_data(NODE *name)
{
    for (NODE *list = n_binds; list != NIL; list = list->list.cdr) {
        for (NODE *bind = list->list.car; bind != NIL; bind = bind->bind.next) {
            if (bind->bind.name == name) {
                return (bind->bind.data);
            }
        }
    }
    return (name->name.data);
}

//----------------------------------------------------------------
NODE *evaluate(NODE *n)
{
    if (n == NIL) return (n);
    else if (n->type() == NT_NAME) return (get_name_data(n));
    else if (n->type() == NT_LIST) {
        NODE *foo = evaluate(n->list.car);
        int i;

        if (foo == NIL) return (make_fail("nil function", "evaluate()", foo));
        switch (foo->type()) {
          case NT_CALL:
          case NT_CALL_FORM:
            i = argv_parms(n->list.cdr, foo->type());
            n = foo->call.code(i, n_stack_tail - i);
            n_stack_tail -= i;
            return (n);

          case NT_USER:
            n = bind_parms(foo->user.names, n->list.cdr, foo->user.binds);
            if (n->type(NT_FAIL)) return (n);

            *n_stack_tail++ = n_binds;

            n_binds = n;

            foo = foo->user.code;
            for (n = NIL; foo != NIL; foo = foo->list.cdr) {
                n = evaluate(foo->list.car);
                //if (n->type(NT_FAIL)) break;
            }

            n_binds = *--n_stack_tail;
            return (n);

          case NT_USER_FORM:
            return (n);

          case NT_HASH:
            *n_stack_tail++ = foo;
            i = 1 + argv_parms(n->list.cdr, NT_CALL);
            n = f_getn(i, n_stack_tail - i);
            n_stack_tail -= i;
            return (n);

          default:
            return (make_fail("bad function", "evaluate()", foo));
        }                
    }
    return (n);
}

//----------------------------------------------------------------

NODE *f_exit(int argc, NODE *argv[])
{
    argc; argv;
    n_exit = n_true;
    return (n_exit);
}

NODE *f_setq(int argc, NODE *argv[])
{
    if (argc == 2 && argv[0]->type(NT_NAME)) {
        argv[1] = evaluate(argv[1]);

        // change this, only if we set the global value to the function
        //if (argv[1]->type(NT_USER) || argv[1]->type(NT_USER_FORM)) {
        //    if (argv[1]->user.text == NIL)
        //        argv[1]->user.text = argv[0]->name.text;
        //}

        return (set_name_data(argv[0], argv[1]));
    }
    return (make_fail("bad args", "set!", NIL));
}

NODE *f_setcarq(int argc, NODE *argv[])
{
    if (argc == 2 && argv[0]->type(NT_LIST)) {
        NODE *old = argv[0]->list.car;
        argv[0]->list.car = argv[1];
        return (old);
    }
    return (make_fail("bad args", "set-car!", NIL));
}

NODE *f_setcdrq(int argc, NODE *argv[])
{
    if (argc == 2 && argv[0]->type(NT_LIST)) {
        NODE *old = argv[0]->list.cdr;
        argv[0]->list.cdr = argv[1];
        return (old);
    }
    return (make_fail("bad args", "set-cdr!", NIL));
}

NODE *f_quote(int argc, NODE *argv[])
{
    argc;
    return (argv[0]);
}

NODE *f_lambda(int argc, NODE *argv[])
{
    NODE *code, *foo;
    if (argc < 1) return (make_fail("bad args", "lambda", NIL));

    code = NIL;
    while (--argc) code = make_list(argv[argc], code);

    foo = make_node(NT_USER);
    foo->user.text  = NIL;
    foo->user.code  = code;
    foo->user.names = argv[0];
    foo->user.binds = n_binds;
    return (foo);
}

NODE *f_define(int argc, NODE *argv[])
{
    if (argc < 2) return (make_fail("bad args", "define", NIL));

    if (argv[0]->type(NT_NAME)) {

        if (argc != 2) return (make_fail("bad args", "define", NIL));
    
        argv[1] = evaluate(argv[1]);
        set_name_data(argv[0], argv[1]);
        return (argv[0]);
    }
    
    if (!argv[0]->type(NT_LIST)) return (make_fail("bad args", "define", NIL));
    
    NODE *name = argv[0]->list.car;

    if (!name->type(NT_NAME)) return (make_fail("bad args", "define", NIL));  

    NODE *code = NIL;
    while (--argc) code = make_list(argv[argc], code);

    NODE *node = make_node(NT_USER);
    node->user.text = name->name.text;
    node->user.code  = code;
    node->user.names = argv[0]->list.cdr;
    node->user.binds = n_binds;

    set_name_data(name, node);

    return (name);
}

NODE *f_if(int argc, NODE *argv[])
{
    if (argc < 2 || argc > 3) return (make_fail("bad args", "if", NIL));
    NODE* test = evaluate(argv[0]);
    if (test == NIL) return (argc < 3) ? test : evaluate(argv[2]);
    return (test->type(NT_FAIL)) ? test : evaluate(argv[1]);
}

//----------------------------------------------------------------

NODE *f_cons(int argc, NODE *argv[])
{
    if (argc < 2) return (make_fail("bad args", "cons", NIL));
    return (make_list(argv[0], argv[1]));
}

NODE *f_car(int argc, NODE *argv[])
{
    if (argc < 1) return (make_fail("bad args", "car", NIL));
    if (!argv[0]->type(NT_LIST)) return (make_fail("bad args", "car", argv[0]));
    return (argv[0]->list.car);
}

NODE *f_cdr(int argc, NODE *argv[])
{
    if (argc < 1) return (make_fail("bad args", "cdr", NIL));
    if (!argv[0]->type(NT_LIST)) return (make_fail("bad args", "cdr", argv[0]));
    return (argv[0]->list.cdr);
}

NODE *f_read(int argc, NODE *argv[])
{
    return (read());
}

NODE *f_eval(int argc, NODE *argv[])
{
    if (argc < 1) return (make_fail("bad args", "eval", NIL));
    return (evaluate(argv[0]));
}

NODE *f_print(int argc, NODE *argv[])
{
    if (argc < 1) return (make_fail("bad args", "show", NIL));
    if (argv[0]->type(NT_USER) || argv[0]->type(NT_USER_FORM)) {
      return (make_list(argv[0]->user.text, make_list(argv[0]->user.names, make_list(argv[0]->user.code, argv[0]->user.binds))));
    }
    return (argv[0]);
}

NODE *check_math_args(int argc, NODE *argv[])
{
    int i;
    if (argc < 1) return (make_fail("bad args", "math", NIL));
    for (i = 0; i < argc; i++) if (!argv[i]->type(NT_MATH)) return (make_fail("bad args", "math", argv[i]));
    return (NIL);
}

NODE *f_add(int argc, NODE *argv[])
{
    NODE *n = check_math_args(argc, argv);
    if (n) return (n);

    int r = argv[0]->math.data;
    for (int i = 1; i < argc; i++) r += argv[i]->math.data;
    return (make_math(r));
}
    
NODE *f_sub(int argc, NODE *argv[])
{
    NODE *n = check_math_args(argc, argv);
    if (n) return (n);

    int r = argv[0]->math.data;
    if (argc == 1) return (make_math(-r));
    for (int i = 1; i < argc; i++) r -= argv[i]->math.data;
    return (make_math(r));
}

NODE *f_mul(int argc, NODE *argv[])
{
    NODE *n = check_math_args(argc, argv);
    if (n) return (n);

    int r = argv[0]->math.data;
    for (int i = 1; i < argc; i++) r *= argv[i]->math.data;
    return (make_math(r));
}

NODE *f_div(int argc, NODE *argv[])
{
    NODE *n = check_math_args(argc, argv);
    if (n) return (n);

    int r = argv[0]->math.data;
    for (int i = 1; i < argc; i++) r /= argv[i]->math.data;
    return (make_math(r));
}

NODE *f_mod(int argc, NODE *argv[])
{
    NODE *n = check_math_args(argc, argv);
    if (n) return (n);

    int r = argv[0]->math.data;
    for (int i = 1; i < argc; i++) r %= argv[i]->math.data;
    return (make_math(r));
}

NODE *f_divmod(int argc, NODE *argv[])
{
    NODE *n = check_math_args(argc, argv);
    if (n) return (n);
    if (argc != 2) return (make_fail("bad args", "math", NIL));
    int q = argv[0]->math.data / argv[1]->math.data;
    int r = argv[0]->math.data % argv[1]->math.data;
    return (make_list(make_math(q), make_math(r)));
}

NODE *f_less(int argc, NODE *argv[])
{
    NODE *n = check_math_args(argc, argv);
    if (n) return (n);

    if (argc == 1) return (NIL);
    int v = argv[0]->math.data;
    for (int i = 1; i < argc; v = argv[i]->math.data, i++) if (!(v < argv[i]->math.data)) return (NIL);
    return (n_true);
}

NODE *f_getn(int argc, NODE* argv[])
{
    if (argc != 2 || !argv[0]->type(NT_HASH) || !argv[1]->type(NT_MATH)) return (make_fail("bad args", "index", NIL));
    return argv[0]->hash.nodes[argv[1]->math.data];
}

typedef unsigned int uint32;
typedef signed int int32;

enum {
    op_move,
    op_jump,
    op_jmpt,
    op_jmpf,
    op_call,
    op_exit
};

union instruction {
    uint32 ifull;
    struct {
        uint32 i:6;
        uint32 a:8;
        uint32 b:9;
        uint32 c:9;
    } iabc;
    struct {
        uint32 i:6;
        uint32 a:8;
        uint32 u:18;
    } iau;
    struct {
        uint32 i:6;
        uint32 a:8;
        int32  s:18;
    } ias;
};

struct jit {
    NODE*  konstants;
    uint32 code[256];
};

struct virtual_machine {
    NODE*    symbols;
    uint32*  pc;
    NODE**   rp;
    NODE**   kp;
    NODE*    stack[124];
    uint32** sp;
    uint32*  ret[124];
};

#define rk_(i) ((i >= 256) ? vm.kp[i] : vm.rp[i])

void vm_execute (virtual_machine& vm) {
    instruction ir = { *vm.pc++ };
    switch (ir.iabc.i) {
    case op_move: vm.rp[ir.iabc.a] = rk_(ir.iabc.b); break;
    case op_jump: vm.pc += ir.ias.s; break;
    case op_jmpt: if (vm.rp[ir.ias.a] != 0) vm.pc += ir.ias.s; break;
    case op_jmpf: if (vm.rp[ir.ias.a] == 0) vm.pc += ir.ias.s; break;
    case op_call:
    case op_exit: 
        vm.pc = *vm.sp++; 
        //vm.rp += 
            break;
        

    default: __debugbreak(); break;
    }
}

//----------------------------------------------------------------

void make_calls(void)
{
    make_call("cons",   f_cons,     NT_CALL);
    make_call("car",    f_car,      NT_CALL);
    make_call("cdr",    f_cdr,      NT_CALL);
    make_call("read",   f_read,     NT_CALL);
    make_call("eval",   f_eval,     NT_CALL);
    make_call("print",  f_print,    NT_CALL);
    make_call("+",      f_add,      NT_CALL);
    make_call("-",      f_sub,      NT_CALL);
    make_call("*",      f_mul,      NT_CALL);
    make_call("/",      f_div,      NT_CALL);
    make_call("%",      f_mod,      NT_CALL);
    make_call("/%",     f_divmod,   NT_CALL);
    make_call("<",      f_less,     NT_CALL);
    make_call("getn",   f_getn,     NT_CALL);

    make_call("set-car!",   f_setcarq,  NT_CALL);
    make_call("set-cdr!",   f_setcdrq,  NT_CALL);

    make_call("set!",       f_setq,     NT_CALL_FORM);

    make_call("define",     f_define,   NT_CALL_FORM);
    make_call("lambda",     f_lambda,   NT_CALL_FORM);
    make_call("if",         f_if,       NT_CALL_FORM);
    make_call("exit",       f_exit,     NT_CALL_FORM);
    make_call("quote",      f_quote,    NT_CALL_FORM);
}

//----------------------------------------------------------------
void main(void)
{
    vOutText("JISP, v0.9, Copyright (C) 1998-2017, James Anhalt III, All Rights Reserved");
    vOutLine();

    n_names = make_node(NT_HASH);

    n_unbound = make_fail("unbound", NIL);
    
    NODE *temp = make_name("names");
    temp->name.data = n_names;

    n_true = make_name("t");
    n_true->name.data = n_true;
    
    make_calls();

    n_quote = make_name("quote");

    while (!n_exit) {
        vOutText("> ");
        print(evaluate(read()));
    }
}
