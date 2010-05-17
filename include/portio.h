#ifndef PORTIO_H
#define PORTIO_H

static inline void outchar(unsigned short _port, unsigned char _data)
{
   __asm__ ("out %%al, %%dx" : : "a" (_data), "d" (_port));
}

static inline unsigned char inchar(unsigned short _port)
{
   unsigned char result;
   __asm__ ("in %%dx, %%al" : "=a" (result) : "d" (_port));
   return result;
}

static inline void outshort(unsigned short _port, unsigned short _data)
{
   __asm__ ("out %%ax, %%dx" : : "a" (_data), "d" (_port));
}

static inline unsigned short inshort(unsigned short _port)
{
   unsigned short result;
   __asm__ ("in %%dx, %%ax" : "=a" (result) : "d" (_port));
   return result;
}

static inline void outint(unsigned short _port, unsigned int _data)
{
   __asm__ ("out %%eax, %%dx" : : "a" (_data), "d" (_port));
}

static inline unsigned int inint(unsigned short _port)
{
   unsigned int result;
   __asm__ ("in %%dx, %%eax" : "=a" (result) : "d" (_port));
   return result;
}

#endif
