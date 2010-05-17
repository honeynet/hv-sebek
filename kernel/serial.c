/********************************************************************************
 * Created and copyright by MAVMM project group:
 * 	Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
 *  Distribution is prohibited without the authors' explicit permission
 ********************************************************************************/

#include "portio.h"
#include "printf.h"
#include "serial.h"

// remove if want outf to print to serial port
//#define OUTF_2_SCREEN 1

// here are some various links related to this
// this code is mostly from http://www.beyondlogic.org/serial/serial.htm
// http://www.beyondlogic.org/

// also here are a few more resources
// http://linuxgazette.net/issue94/ramankutty.html
// http://www.osdever.net/tutorials/brunmar/
// http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html#s4
// http://www.programmersheaven.com/mb/x86_asm/356326/356326/ReadMessage.aspx

/* Name		 : Sample Comm's Program - Polled Version - termpoll.c	  */
/* Written By : Craig Peacock <cpeacock@senet.com.au>						 */
/* Date		 : Saturday 22nd February 1997									  */

/*		  Copyright 1997 CRAIG PEACOCK <cpeacock@senet.com.au>			 */

/*			See http://www.senet.com.au/~cpeacock/serial1.htm				*/
/*							  For More Information									*/

/* Defines Serial Ports Base Address */
/* COM1 0x3F8								*/
/* COM2 0x2F8					 */
/* COM3 0x3E8					 */
/* COM4 0x2E8					 */

// global pointer to the serial port we're going to use
unsigned int port = 0;

//Anh - output string to IO port
void outf ( const char *fmt, ... )
{
	static char buf [BUF_SIZE];
	va_list args;

	#ifdef OUTF_2_SCREEN
		va_start ( args, fmt );
		vsnprintf ( buf, sizeof ( buf ), fmt, args );
		va_end ( args );

		putstr ( buf );
		return;
	#else

		char * s = buf;

		va_start ( args, fmt );
		vsnprintf ( buf, sizeof ( buf ), fmt, args );
		va_end ( args );

		char c;
		int i = 0;	//over flown guarding
		while ( ( c = *s++ ) != '\0' )
		{
			i ++;
			putchar_serial(c);
			if (i == BUF_SIZE) break;
		}

	#endif
}

static void serial_test()
{
	outf("\nSerial Port Initialized Successfully\n==================\n\n");
}

void putchar_serial(char c) {
	outchar(port,c);
}

/**
 * do some setup on the serial port to discover which one to use
 */
void setup_serial() {
	#ifdef OUTF_2_SCREEN
		return;
	#else

		unsigned int *ptraddr;  		/* Pointer to location of Port Addresses */
		unsigned int address;	 	/* Address of Port */
		int a;

		ptraddr=(unsigned int *)0x00000400;	//1KB
		//Anh: seem like the memory mapping address of the ports are located at addr 0x400
		//size of each entry = 32 bits?

		for (a = 0; a <  4; a++)
		{
			address = *ptraddr;
			if (address == 0)
							printf("No port found for COM%x \n", a + 1);
			else
							printf("Address assigned to COM%x is %x h\n", a + 1 , address);
			ptraddr++;
		}

		ptraddr=(unsigned int *)0x00000400;

		//Anh - Use COM1 (USECOM = 1)
		port = *(ptraddr + USECOM - 1);

		printf("\nUsing port %x at %x\n", USECOM, port);

		outchar(port + 1 , 0);	/* Turn off interrupts - Port1 */

		/*			PORT 1 - Communication Settings			*/
		outchar(port + 3 , 0x80);  /* SET DLAB ON */
		outchar(port + 0 , 0x0C);  /* Set Baud rate - Divisor Latch Low Byte */
		/* Default 0x03 =  38,400 BPS */
		/*			0x01 = 115,200 BPS */
		/*			0x02 =  57,600 BPS */
		/*			0x06 =  19,200 BPS */
		/*			0x0C =	9,600 BPS */
		/*			0x18 =	4,800 BPS */
		/*			0x30 =	2,400 BPS */
		outchar(port + 1 , 0x00);  /* Set Baud rate - Divisor Latch High Byte */
		outchar(port + 3 , 0x03);  /* 8 Bits, No Parity, 1 Stop Bit */
		outchar(port + 2 , 0xC7);  /* FIFO Control Register */
		outchar(port + 4 , 0x0B);  /* Turn on DTR, RTS, and OUT2 */

		serial_test();

		return;
	#endif
}
