#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>
#include <stddef.h>

#define EOF -1

#ifdef __cplusplus
extern "C" {
#endif

int  __printf(int (*)(void *, int), void *, const char *, va_list);
int  printf(const char *, ...);
int  snprintf(char *s, size_t n, const char *, ...);
int  vprintf(const char *, va_list);
int  vsnprintf(char *s, size_t n, const char *, va_list);

void perror(const char *);

#ifdef __cplusplus
};
#endif

#endif  // !__STDIO_H__
