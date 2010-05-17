/********************************************************************************
 * Created and copyright by MAVMM project group:
 * 	Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
 *  Distribution is prohibited without the authors' explicit permission
 ********************************************************************************/

#include <stdarg.h>
#include "types.h"
#include "failure.h"
#include "string.h"
#include "serial.h"
#include "printf.h"

#define VIDEO_MEM ( (char *) 0xb8000 )

enum {
	COLS  		= 80,
	LINES		= 25,
	VIDEO_PORT 	= 0x3d4
};

struct screen_info {
	int orig_x;
	int orig_y;
};
static struct screen_info scr_info;

void clear_screen( void )
{
	scr_info.orig_x = 0;
	scr_info.orig_y = 0;
	memset(VIDEO_MEM,0,COLS*LINES);
	return;
}

static void scroll( void )
{
	int i;

	memmove ( VIDEO_MEM, VIDEO_MEM + COLS * 2, ( LINES - 1 ) * COLS * 2 );
	for ( i = ( LINES - 1 ) * COLS * 2; i < LINES * COLS * 2; i += 2 ) {
		VIDEO_MEM [ i ] = ' ';
	}
}

void putstr ( const char *s )
{
	int x, y;
	char c;

	x = scr_info.orig_x;
	y = scr_info.orig_y;

	while ( ( c = *s++ ) != '\0' )
	{
		//Anh: Send it out to the serial port
		//putchar_serial(c);

		if ( c == '\n' ) {
			x = 0;
			if ( ++y >= LINES ) {
				scroll ( );
				y--;
			}
		} else {
			VIDEO_MEM [ ( x + COLS * y ) * 2 ] = c;
			if ( ++x >= COLS ) {
				x = 0;
				if ( ++y >= LINES ) {
					scroll ( );
					y--;
				}
			}
		}
	}

	scr_info.orig_x = x;
	scr_info.orig_y = y;

#if 0
	/* Update cursor position */
	int pos;
	pos = ( x + COLS * y ) * 2;
	outb_p ( 14, vidport );
	outb_p ( 0xff & ( pos >> 9 ), vidport + 1 );
	outb_p ( 15, vidport );
	outb_p ( 0xff & ( pos >> 1 ), vidport + 1 );
#endif
}

static int number ( char *buf, size_t size, int j, unsigned long num )
{
	const char DIGITS[] = "0123456789abcdef";
	char tmp [ 64 ];
	int len = 0;

	{
		unsigned long n = num;

		do {
			tmp [ len ] = DIGITS [ n & 0xf ];
			n >>= 4;
			len++;
		} while  ( n );
	}

	buf [ j ] = '0';
	buf [ j + 1 ] = 'x';

	{
		int i;
		for ( i = 0; i < len; i++ ) {
			if ( j + i < size ) {
				buf [ j + 2 + i ] = tmp [ len - i - 1 ];
			}
		}
	}

	return len + 2;
}

#define PUT_CHAR(buf, index, size, c)  \
{ \
	if ( index < size ) { \
		buf [ index ] = ( c ); \
	} \
	index++; \
}

int vsnprintf ( char *buf, size_t size, const char *fmt, va_list args )
{
	int i, j;

	if ( size <= 0 ) {
		return 0;
	}

	for (i = 0, j = 0; fmt [ i ] != '\0'; i++ ) {

		if ( fmt [ i ] != '%' ) {
			PUT_CHAR ( buf, j, size, fmt [ i ] );
			continue;
		}

		i++;

		switch ( fmt [ i ] ) {
		case 'c': {
			unsigned char c = ( unsigned char ) va_arg ( args, int );
			PUT_CHAR ( buf, j, size, c );
			break;
		}
		case 's': {
			char *s = va_arg ( args, char * );

			int k;
			for ( k = 0; s [ k ] != '\0'; k++ ) {
				PUT_CHAR ( buf, j, size, s [ k ] );
			}
			break;
		}
		case 'x': {
			unsigned long n = (unsigned long) va_arg ( args, void * );
			j += number ( buf, size, j, n );
			break;
		}
		case '%':
			PUT_CHAR ( buf, j, size, '%' );
			break;
		default:
			fatal_failure ( "vsnprintf" );
		}
	}

	if ( j  < size ) { buf [ j ] = '\0'; }

	return j;
}

//NOTE : BOTH printf and outf do not work right if there are more than 1 argument after fmt
void printf ( const char *fmt, ... )
{
	return; //TODO: added this to preserve guest outputs. All VMM outputs should use outf

	static char buf [BUF_SIZE];
	va_list args;

	va_start ( args, fmt );
	vsnprintf ( buf, sizeof ( buf ), fmt, args );
	va_end ( args );

	putstr ( buf );
}

void print_binary ( char *p, size_t len )
{
	int i;
	const char DIGITS[] = "0123456789abcdef";

	for ( i = 0; i < len; i++ ) {
		char c = * ( p + i );
		int j;

		for ( j = 0; j < 8; j++ ) {
			char s[2];
			s[0] = DIGITS [ ( c >> j ) & 1 ];
			s[1] = '\0';
			printf ( s );
		}

		printf ( " " );

		if ( ( i & 3 ) == 3 ) {
			printf ( "\n" );
		}
	}
}
