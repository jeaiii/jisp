#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef char VCHAR;
typedef char VNAME[256];
typedef char VTEXT[];
typedef void* VFILE;

void vAppDone (int) {
	ExitProcess(0);
}

void vOutText (const char* head, const char* tail) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD size = tail - head;
	if (size == 0)
		return;
	WriteFile(hOut, head, size, &size, NULL);
	FlushFileBuffers(hOut);
}

void vOutText (const char* text) 
{
	vOutText(text, text + strlen(text));
}

void vOutLine () 
{
	vOutText("\r\n");
}

void vTextCopy(char* dst, const char* src) 
{
	while ((*dst++ = *src++) != 0);
}

int vTextCompare(const char* dst, const char* src)
{
	int i;
	do {
		i = *src++ - *dst++;
		if (i != 0)
			break;
    } while (*src);
	return i;
}

VCHAR *vTextFromNumberX(VTEXT text, unsigned int number)
{
    int i = 7;

    text[8] = '\0';
    do {
        text[i] = "0123456789ABCDEF"[number % 16];
        number /= 16;
    } while (i--);
    return (text);
}

unsigned int vGetText(VTEXT text)
{
    DWORD size = 0;
    
    HANDLE hGet = GetStdHandle(STD_INPUT_HANDLE);
    
    if (ReadFile(hGet, text, 255, &size, NULL)) {
        text[size] = 0;
        return (size);
    }
    return (0);
}

VCHAR *vTextFromNumberD(VTEXT text, int number) 
{
    VCHAR dst[16];
    VCHAR *c = &dst[16];
    int negative = 0;

    if (number < 0) { 
        negative = 1; 
        number = -number; 
    }

    *--c = 0;

    if (number == 0) *--c ='0';
    else while (number != 0) {
        *--c = '0' + (VCHAR)(number % 10);
        number /= 10;
    }
    if (negative) *--c = '-';

    vTextCopy(text, c);
    return (text);
}