/********************************************************************************
* This software is licensed under the GNU General Public License:
* http://www.gnu.org/licenses/gpl.html
*
* MAVMM Project Group:
* Anh M. Nguyen, Nabil Schear, Apeksha Godiyal, HeeDong Jung, et al
*
*********************************************************************************/

#include "serial.h"

void inline breakpoint(const char * szMsg)
{
	outf(szMsg);

	__asm__ __volatile__ ( "push %%dx; mov 0x00ffffff, %%dl; pop %%dx" : : );
}

static void
die ( void )
{
	__asm__ __volatile__ ( "ud2a" : : );
}

void
fatal_failure ( const char *msg )
{
	outf ( "FATAL FAILURE: " );
	outf ( msg );

//	__asm__ __volatile__ ( "leaq 0(%%rip), %0" : "=r" (rip) : );

	die ( );
}
