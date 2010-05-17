#ifndef __STRING_H__
#define __STRING_H__


#include "types.h"

extern void * memcpy ( void *to, const void *from, size_t len );
extern void * memmove ( void * dest, const void *src, size_t count );
extern void * memset ( void *s, int c, size_t count );

extern char * strcpy(char * dest,const char *src);
extern int strcmp ( const char * cs,const char * ct );
extern int strncmp ( const char *cs, const char *ct, size_t count );
extern int strlen(const char *);
extern char *strstr (register char *buf,register char *sub);

// Quangtin3: add for hostfs module
extern char *	strchr(const char *s, char c);
extern char *	strncpy(char *dst, const char *src, size_t size);
extern char *   strrchr(const char *p, int ch);
extern int  strncasecmp ( const char* s1, const char* s2, size_t len );

#endif /* __STRING_H__ */
