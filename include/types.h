#ifndef __TYPES_H__
#define __TYPES_H__


#ifndef __ASSEMBLY__

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

typedef unsigned long size_t;

#define BYTES_PER_LONG  8

#if 0 
#define __init		__attribute__ ((__section__ (".init.text")))
#else
#define __init
#endif

#define NULL	0 

#endif /* ! __ASSEMBLY__ */

#endif /* __TYPES_H__ */
