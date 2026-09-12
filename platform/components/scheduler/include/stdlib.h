#ifndef __STDLIB_H__
#define __STDLIB_H__

#include "stdbool.h"
#include "stddef.h"

#define FALSE 0
#define TRUE  1

#define ULONG_MAX 0xFFFFFFFFFFFFFFFFUL

#ifndef NULL
#define NULL ((void*)0)
#endif

int tolower(int c);

int abs(int __n);
long long llabs(long long int __n);
int atoi(const char* str);
unsigned long long int strtoull(const char* ptr, char** end, int base);
long int strtol(const char* nptr, char** endptr, int base);
int isspace(int c);
unsigned long strtoul(const char* nptr, char** endptr, int base);
int strcmp(const char* s1, const char* s2);
size_t strlen(const char* str);

void* memcpy(void* dest, const void* src, long n);
#endif