#include "string.h"
#include "stdio.h"
#include "../include/user.h"

void vmmcall(int rebx, int recx, int redx) {
	printf("vmmcall: ebx=%x, ecx=%x, edx=%x\n", rebx, recx, redx);

	__asm__("vmmcall" :: "b" (rebx), "c" (recx), "d" (redx));
}

struct cmdflag {
   const char * szcmd;
   int	cmdflag;
};

struct cmdflag intercp_flags[] = {
	{"swint", USER_ITC_SWINT},
	{"task", USER_ITC_TASKSWITCH},
	{"syscall", USER_ITC_SYSCALL},
	{"step", USER_SINGLE_STEPPING},
	{"unpack", USER_UNPACK},
	{"_end", 0}
};

int get_intercp_flag(char * arg)
{
	int i = 0;
	int flag = 0;

	while (1) {
		if (strcmp(intercp_flags[i].szcmd, "_end") == 0) break;

		if (strcmp(intercp_flags[i].szcmd, arg) == 0) flag = intercp_flags[i].cmdflag;

		i ++;
	}

	return flag;
}

struct cmdflag test_flags[] = {
	{"smode", USER_TEST_SWITCHMODE},
	{"_end", 0}
};

int get_test_flag(char * arg)
{
	int i = 0;
	int flag = 0;

	while (1) {
		if (strcmp(test_flags[i].szcmd, "_end") == 0) break;

		if (strcmp(test_flags[i].szcmd, arg) == 0) flag = test_flags[i].cmdflag;

		i ++;
	}

	return flag;
}

int main(int argc, char * argv[]) {
	int ebx = 0, ecx = 0, edx = 0;

	if (argc < 3) {
		printf("usage: %s cmd arg\n", argv[0]);
		printf("cmd: en / dis\n");
		printf("arg: task pname / syscall / step / unpack\n");
		return -1;
	}

	if (strcmp(argv[1], "en") == 0)
	{
		ebx = USER_CMD_ENABLE;
		ecx = get_intercp_flag(argv[2]);
	}
	else if (strcmp(argv[1], "dis") == 0)
	{
		ebx = USER_CMD_DISABLE;
		ecx = get_intercp_flag(argv[2]);
	}
	else if (strcmp(argv[1], "test") == 0)
	{
		ebx = USER_CMD_TEST;
		ecx = get_test_flag(argv[2]);
	}

	if (ecx == USER_ITC_TASKSWITCH & argc < 4) {
		printf("Error: for 'task' cmd, need to specify process name: task pname\n");
		return -1;
	}

	if (argc > 3) edx = (int) argv[3];

	vmmcall(ebx, ecx, edx);

	return 0;
}
