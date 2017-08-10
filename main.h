//
// Jisp - main.cpp
//
// Copyright (C) 1998-2017, James Anhalt III, All Rights Reserved
//

typedef char VCHAR;
typedef char VNAME[256];
typedef char VTEXT[];
typedef void* VFILE;

void vAppDone (int i);
void vOutText (const char* head, const char* tail);
void vOutText (const char* text);
void vOutLine ();

void vTextCopy (char* dst, const char* src); 
int vTextCompare (const char* dst, const char* src);
VCHAR *vTextFromNumberD(VTEXT text, int number);
VCHAR *vTextFromNumberX(VTEXT text, unsigned int number);
unsigned int vGetText(VTEXT text);

#define NULL 0