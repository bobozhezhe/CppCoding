#include "stdio.h"
#include "string.h"
#include "malloc.h"

#ifndef __UNIX
#pragma warning(disable : 4996).
#endif // !__UNIX

char* my_strcat(char* dst, const char* src) {
	char* p = dst;
	dst += strlen(dst);
	strcpy(dst, src);
	return p;
}

int
test_strcat() {
	char s1[] = "hello ";
	char s2[] = "students!";
	char* p = (char*)malloc(strlen(s1) + strlen(s2) + 1);

	p = strcpy(p, s1);

	//p = strcat(p, s1);
	p = my_strcat(p, s2);

	printf("%s", p);
}

