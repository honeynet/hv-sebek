#ifndef SERIAL_H_
#define SERIAL_H_

#define USECOM 1

/**
 * do some setup on the serial port to discover which one to use
 */
extern void outf ( const char *fmt, ... );

extern void setup_serial();
// put a single character out over the serial port
extern void putchar_serial(char c);

#endif /*SERIAL_H_*/
