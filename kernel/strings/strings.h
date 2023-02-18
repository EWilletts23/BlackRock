#include <stdint.h>
#include <stddef.h>

int strlen(char s[]);
void* strcpy (char *dest, const char *src);
char* strcat(char *dest, const char *src);
int itoa(int num, unsigned char* str, int len, int base);