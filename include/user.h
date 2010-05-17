#ifndef _H_USER_H_
#define _H_USER_H_

#define 	USER_CMD_ENABLE 		0
#define 	USER_CMD_DISABLE 		1
#define 	USER_CMD_TEST 			9

#define 	USER_ITC_SWINT			1 << 0
#define 	USER_ITC_TASKSWITCH		1 << 1
#define 	USER_ITC_SYSCALL		1 << 2
#define 	USER_ITC_IRET			1 << 3
#define 	USER_SINGLE_STEPPING	1 << 4
#define 	USER_UNPACK				1 << 5
#define		USER_ITC_ALL			0xFF

#define 	USER_TEST_SWITCHMODE	1

#endif
