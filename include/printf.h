#ifndef __PRINTF_H__
#define __PRINTF_H__


#include <stdarg.h>
#include "types.h"

extern void putstr ( const char *s );
extern void printf ( const char *fmt, ... );
extern void print_binary ( char *p, size_t len );
extern void clear_screen( void );

extern int vsnprintf ( char *buf, size_t size, const char *fmt, va_list args );

#define BUF_SIZE 1024

#endif /* __PRINTF_H__ */
