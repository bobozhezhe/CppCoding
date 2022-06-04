#include "stdio.h"
#include "string.h"

#ifndef __UNIX
#pragma warning(disable : 4996).
#endif // !__UNIX

int
main() {
	printf("hellow world!");
	char* a = "abc";
	char b[30];

	strcpy(b, a);
}
