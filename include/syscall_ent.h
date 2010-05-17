	{ 0,	0,	"restart_syscall" },/* 0 */
	{ 1,	TP,	"exit" },		/* 1 */
	{ 0,	TP,	"fork" },		/* 2 */
	{ 3,	TD,	"read" },		/* 3 */
	{ 3,	TD,	"write" },		/* 4 */
	{ 3,	TD|TF,	"open" },		/* 5 */
	{ 1,	TD,	"close" },		/* 6 */
	{ 3,	TP,	"waitpid" },	/* 7 */
	{ 2,	TD|TF,	"creat" },		/* 8 */
	{ 2,	TF,	"link" },		/* 9 */
	{ 1,	TF,	"unlink" },		/* 10 */
	{ 3,	TF|TP,    "execve" },		/* 11 */
	{ 1,	TF,	"chdir" },		/* 12 */
    { 1,    0,	"time"},		/* 13 */
	{ 3,	TF,	"mknod" },		/* 14 */
	{ 2,	TF,	"chmod" },		/* 15 */
	{ 3,	TF, "lchown" },		/* 16 */
	{ 0,	0,	"break" },		/* 17 : not implemented in linux 2.6.22 */
	{ 2,	TF,	"oldstat" },	/* 18 */
	{ 3,	TD,	"lseek" },		/* 19 */
	{ 0,	0,	"getpid" },		/* 20 */
	{ 5,	TF,	"mount" },		/* 21 */
	{ 1,	TF,	"umount" },		/* 22 */
	{ 1,	0,	"setuid" },		/* 23 */
	{ 0,	0,	"getuid" },		/* 24 */
	{ 1,	0,	"stime" },		/* 25 */
	{ 4,	0,	"ptrace" },		/* 26 */
	{ 1,	0,	"alarm" },		/* 27 */
	{ 2,	TD,	"oldfstat" },	/* 28 */
	{ 0,	TS,	"pause" },		/* 29 */
	{ 2,	TF,	"utime" },		/* 30 */
	{ 2,	0,	"stty" },		/* 31 : not implemented in linux 2.6.22 */
	{ 2,	0,	"gtty" },		/* 32 : not implemented in linux 2.6.22 */
	{ 2,	TF,	"access" },		/* 33 */
	{ 1,	0,	"nice" },		/* 34 */
	{ 1,	0,	"ftime" },		/* 35 : argument 1 or 0? man page says 1 but strace file says 0
								man ftime says "This function is obsolete.  Don’t use it." */
	{ 0,	0,	"sync" },		/* 36 */
	{ 2,	TS,	"kill" },		/* 37 */
	{ 2,	TF,	"rename" },		/* 38 */
	{ 2,	TF,	"mkdir" },		/* 39 */
	{ 1,	TF,	"rmdir" },		/* 40 */
	{ 1,	TD,	"dup" },		/* 41 */
	{ 1,	TD,	"pipe" },		/* 42 */
	{ 1,	0,	"times" },		/* 43 */
	{ 0,	0,	"prof" },		/* 44 : not implemented in linux 2.6.22 */
	{ 1,	0,	"brk" },		/* 45 */
	{ 1,	0,	"setgid" },		/* 46 */
	{ 0,	0,	"getgid" },		/* 47 */
	{ 2,	TS,	"signal" },		/* 48 : argument 2 or 3? man page says 2 but strace file says 3
								"Avoid  its  use: use sigaction(2) instead." */
	{ 0,	0,	"geteuid" },	/* 49 */
	{ 0,	0,	"getegid" },	/* 50 */
	{ 1,	TF,	"acct" },		/* 51 */
	{ 2,	TF,	"umount2" },	/* 52 */
	{ 0,	0,	"lock" },		/* 53 : not implemented in linux 2.6.22 */
	{ 3,	TD,	"ioctl" },		/* 54 : variable size arguments (default seems like 3) */
	{ 3,	TD,	"fcntl" },		/* 55 */
	{ 0,	0,	"mpx" },		/* 56 : not implemented in linux 2.6.22 */
	{ 2,	0,	"setpgid" },	/* 57 */
	{ 2,	0,	"ulimit" },		/* 58 */
	{ 1,	0,	"oldolduname" },/* 59 */
	{ 1,	0,	"umask" },		/* 60 */
	{ 1,	TF,	"chroot"	},	/* 61 */
	{ 2,	0,	"ustat"		},	/* 62 */
	{ 2,	TD,	"dup2"		},	/* 63 */
	{ 0,	0,	"getppid"	}, /* 64 */
	{ 0,	0,	"getpgrp"	}, /* 65 */
	{ 0,	0,	"setsid"	}, /* 66 */
	{ 3,	TS,	"sigaction"	}, /* 67 */
	{ 0,	TS,	"siggetmask"	}, /* 68 */
	{ 1,	TS,	"sigsetmask"	}, /* 69 */
	{ 2,	0,	"setreuid"	}, /* 70 */
	{ 2,	0,	"setregid"	}, /* 71 */
	{ 1,	TS,	"sigsuspend"	}, /* 72 : argument 1 or 3? man page says 1 but strace file says 3 */
	{ 1,	TS,	"sigpending"	}, /* 73 */
	{ 2,	0,	"sethostname"	}, /* 74 */
	{ 2,	0,	"setrlimit"	}, /* 75 */
	{ 2,	0,	"getrlimit"	}, /* 76 */
	{ 2,	0,	"getrusage"	}, /* 77 */
	{ 2,	0,	"gettimeofday"	}, /* 78 */
	{ 2,	0,	"settimeofday"	}, /* 79 */
	{ 2,	0,	"getgroups"	}, /* 80 */
	{ 2,	0,	"setgroups"	}, /* 81 */
	{ 5,	TD,	"select"	}, /* 82 : argument 5 or 1? man page says 5 but strace file says 1 */
	{ 2,	TF,	"symlink"	}, /* 83 */
	{ 2,	TF,	"oldlstat"	}, /* 84 */
	{ 3,	TF,	"readlink"	}, /* 85 */
	{ 1,	TF,	"uselib"	}, /* 86 */
	{ 2,	TF,	"swapon"	}, /* 87 : argument 1 or 2? man page says 2 but strace file says 1  */
	{ 3,	0,	"reboot"	}, /* 88 */
	{ 3,	TD,	"readdir"	}, /* 89 */
	{ 6,	TD,	"mmap"	},		/* 90 */
	{ 2,	0,	"munmap"	},	/* 91 */
	{ 2,	TF,	"truncate"	},	/* 92 */
	{ 2,	TD,	"ftruncate"	},	/* 93 */
	{ 2,	TD,	"fchmod"	},	/* 94 */
	{ 3,	TD,	"fchown"	},	/* 95 */
	{ 2,	0,	"getpriority"	}, /* 96 */
	{ 3,	0,	"setpriority"	}, /* 97 */
	{ 4,	0,	"profil"	},	/* 98 */
	{ 2,	TF,	"statfs"	},	/* 99 */
	{ 2,	TD,	"fstatfs"	},	/* 100 */
	{ 3,	0,	"ioperm"	},	/* 101 */
	{ 2,	TD,	"socketcall"},	/* 102 */
	{ 3,	0,	"syslog"	},	/* 103 */
	{ 3,	0,	"setitimer"	},	/* 104 */
	{ 2,	0,	"getitimer"	},	/* 105 */
	{ 2,	TF,	"stat"		},	/* 106 */
	{ 2,	TF,	"lstat"		},	/* 107 */
	{ 2,	TD,	"fstat"		},	/* 108 */
	{ 1,	0,	"olduname"	},	/* 109 */
	{ 1,	0,	"iopl"		},	/* 110 */
	{ 0,	0,	"vhangup"	},	/* 111 */
	{ 0,	0,	"idle"		},	/* 112 */
	{ 1,	0,	"vm86old"	},	/* 113 */
	{ 4,	TP,	"wait4"},		/* 114 */
	{ 1,	0,	"swapoff"	},	/* 115 */
	{ 1,	0,	"sysinfo"	},	/* 116 */
	{ 6,	0,	"ipc"},			/* 117 */
	{ 1,	TD,	"fsync"		},	/* 118 */
	{ 1,	TS,	"sigreturn"	},	/* 119 */
	{ 5,	TP,	"clone"},		/* 120 : variable size arguments
								 man page says: "Since Linux 2.5.49 the system call has five parameters."*/
	{ 2,	0,	"setdomainname"}, /* 121 */
	{ 1,	0,	"uname"		},	/* 122 */
	{ 3,	0,	"modify_ldt"},	/* 123 */
	{ 1,	0,	"adjtimex"	},	/* 124 */
	{ 3,	0,	"mprotect"	},	/* 125 */
	{ 3,	TS,	"sigprocmask"}, /* 126 */

/* Some system calls, like ipc(2), create_module(2), init_module(2), and delete_module(2)
 * only exist when the Linux kernel was built with support for them.
 * http://linux.die.net/man/ <- this page seems to talk about linux 2.4 */
	{ 2,	0,	"create_module"}, /* 127 : can't find man page in linux,
						found the man page on the web http://linux.die.net/man/ has 2 arguments*/
	{ 3,	0,	"init_module"},	/* 128 : can't find man page in linux,
						found the man page on the web http://linux.die.net/man/ has 3 arguments*/
	{ 2,	0,	"delete_module"}, /* 129 : can't find man page in linux,
						found the man page on the web http://linux.die.net/man/ has 2 arguments*/
	{ 1,	0,	"get_kernel_syms"}, /* 130 : can't find man page in linux,
						found the man page on the web http://linux.die.net/man/ has 2 arguments*/

	{ 4,	0,	"quotactl"	}, /* 131  */
	{ 1,	0,	"getpgid"	}, /* 132 */
	{ 1,	TD,	"fchdir"	}, /* 133 */
	{ 2,	0,	"bdflush"	}, /* 134 : argument 0 or 2? man page says 2 but strace file says 0 */
	{ 3,	0,	"sysfs"}, /* 135 : variable size arguments 1~3 */
	{ 1,	0,	"personality"	}, /* 136 */
	{ 5,	0,	"afs_syscall"	}, /* 137 : not implemented in linux 2.6.22 */
	{ 1,	0,	"setfsuid"	}, /* 138 */
	{ 1,	0,	"setfsgid"	}, /* 139 */
	{ 5,	TD,	"_llseek"	}, /* 140 */
	{ 3,	TD,	"getdents"	}, /* 141 */
	{ 5,	TD,	"_newselect"	}, /* 142 */
	{ 2,	TD,	"flock"		}, /* 143 */
	{ 3,	0,	"msync"		}, /* 144 */
	{ 3,	TD,	"readv"}, /* 145 */
	{ 3,	TD,	"writev"}, /* 146 */
	{ 1,	0,	"getsid"	}, /* 147 */
	{ 1,	TD,	"fdatasync"	}, /* 148 */
	{ 1,	0,	"_sysctl"	}, /* 149 */
	{ 2,	0,	"mlock"		}, /* 150 */
	{ 2,	0,	"munlock"	}, /* 151 */
	{ 1,	0,	"mlockall"	}, /* 152 : argument 1 or 2? man page says 1 but strace file says 2 */
	{ 0,	0,	"munlockall"	}, /* 153 */
	{ 2,	0,	"sched_setparam"}, /* 154 : argument 0 or 2? man page says 2 but strace file says 0 */
	{ 2,	0,	"sched_getparam"}, /* 155 */
	{ 3,	0,	"sched_setscheduler"}, /* 156 */
	{ 1,	0,	"sched_getscheduler"}, /* 157 */
	{ 0,	0,	"sched_yield"}, /* 158 */
	{ 1,	0,	"sched_get_priority_max"}, /* 159 */
	{ 1,	0,	"sched_get_priority_min"}, /* 160 */
	{ 2,	0,	"sched_rr_get_interval"}, /* 161 */
	{ 2,	0,	"nanosleep"	}, /* 162 */
	{ 4,	0,	"mremap"	}, /* 163 */
	{ 3,	0,	"setresuid"	}, /* 164 */
	{ 3,	0,	"getresuid"	}, /* 165 */
	{ 2,	0,	"vm86"		}, /* 166 : argument 2 or 5? man page says 2 but strace file says 5 */
	{ 5,	0,	"query_module"	}, /* 167 : can't find man page "it was removed in Linux 2.6." */
	{ 3,	TD,	"poll"		}, /* 168 */
	{ 3,	0,	"nfsservctl"	}, /* 169 */
	{ 3,	0,	"setresgid"	}, /* 170 */
	{ 3,	0,	"getresgid"	}, /* 171 */
	{ 5,	0,	"prctl"		}, /* 172 */
	{ 1,	TS,	"rt_sigreturn"	}, /* 173 */

/* below syscalls starting with rt show man pages for ones without rt (for example sigaction)
 * when you search it on the web, there is rt_sigcation man page. http://linux.die.net/man/2/rt_sigaction */
	{ 4,	TS,	"rt_sigaction"  }, /* 174 */
	{ 4,	TS,	"rt_sigprocmask"}, /* 175 */
	{ 2,	TS,	"rt_sigpending"	}, /* 176 */
	{ 4,	TS,	"rt_sigtimedwait"}, /* 177 */
	{ 3,	TS,	"rt_sigqueueinfo"}, /* 178 */
	{ 2,	TS,	"rt_sigsuspend"	}, /* 179 */

	{ 4,	TD,	"pread64"}, /* 180 : argument 4 or 5? man page says 4 but strace file says 5 */
	{ 4,	TD,	"pwrite64"}, /* 181 : argument 4 or 5? man page says 4 but strace file says 5 */
	{ 3,	TF,	"chown"		}, /* 182 */
	{ 2,	TF,	"getcwd"	}, /* 183 */
	{ 2,	0,	"capget"	}, /* 184 */
	{ 2,	0,	"capset"	}, /* 185 */
	{ 2,	TS,	"sigaltstack"	}, /* 186 */
	{ 4,	TD,	"sendfile"	}, /* 187 */
	{ 5,	0,	"getpmsg"	}, /* 188 : not implemented in linux 2.6.22 */
	{ 5,	0,	"putpmsg"	}, /* 189 : not implemented in linux 2.6.22 */
	{ 0,	TP,	"vfork"}, /* 190 */
	{ 2,	0,	"getrlimit"	}, /* 191 */
	{ 6,	0,	"mmap2"		}, /* 192 */
	{ 2,	TF,	"truncate64"	}, /* 193 : argument 2 or 3? man page says 2 but strace file says 3 */
	{ 2,	TD,	"ftruncate64"	}, /* 194 : argument 2 or 3? man page says 2 but strace file says 3 */
	{ 2,	TF,	"stat64"	}, /* 195 */
	{ 2,	TF,	"lstat64"	}, /* 196 */
	{ 2,	TD,	"fstat64"	}, /* 197 */
	{ 3,	TF,	"lchown32"	}, /* 198 */
	{ 0,	0,	"getuid32"	}, /* 199 */
	{ 0,	0,	"getgid32"	}, /* 200 */
	{ 0,	0,	"geteuid32"	}, /* 201 */
	{ 0,	0,	"getegid32"	}, /* 202 */
	{ 2,	0,	"setreuid32"	}, /* 203 */
	{ 2,	0,	"setregid32"	}, /* 204 */
	{ 2,	0,	"getgroups32"	}, /* 205 */
	{ 2,	0,	"setgroups32"	}, /* 206 */
	{ 3,	TD,	"fchown32"	}, /* 207 */
	{ 3,	0,	"setresuid32"	}, /* 208 */
	{ 3,	0,	"getresuid32"	}, /* 209 */
	{ 3,	0,	"setresgid32"	}, /* 210 */
	{ 3,	0,	"getresgid32"	}, /* 211 */
	{ 3,	TF,	"chown32"	}, /* 212 */
	{ 1,	0,	"setuid32"	}, /* 213 */
	{ 1,	0,	"setgid32"	}, /* 214 */
	{ 1,	0,	"setfsuid32"	}, /* 215 */
	{ 1,	0,	"setfsgid32"	}, /* 216 */
	{ 2,	TF,	"pivot_root"	}, /* 217 */
	{ 3,	0,	"mincore"	}, /* 218 */
	{ 3,	0,	"madvise"	}, /* 219 */
	{ 3,	TD,	"getdents64"	}, /* 220 */
	{ 3,	TD,	"fcntl64"	}, /* 221 */

/* there is no 222, 223 in unistd_32.h */
	{ 4,	0,	"SYS_222"	}, /* 222 */
	{ 4,	0,	"SYS_223"	}, /* 223 */

	{ 0,	0,	"gettid"	}, /* 224 */
	{ 3,	TD,	"readahead"	}, /* 225 : argument 3 or 4? man page says 3 but strace file says 4 */

/* can't find man page  http://linux.die.net/man/2/setxattr */
	{ 5,	TF,	"setxattr"	}, /* 226 */
	{ 5,	TF,	"lsetxattr"	}, /* 227 */
	{ 5,	TD,	"fsetxattr"	}, /* 228 */

/* can't find man page  http://linux.die.net/man/2/getxattr */
	{ 4,	TF,	"getxattr"	}, /* 229 */
	{ 4,	TF,	"lgetxattr"	}, /* 230 */
	{ 4,	0,	"fgetxattr"	}, /* 231 */

/* can't find man page  http://linux.die.net/man/2/listxattr */
	{ 3,	TF,	"listxattr"	}, /* 232 */
	{ 3,	TF,	"llistxattr"	}, /* 233 */
	{ 3,	0,	"flistxattr"	}, /* 234 */

/* can't find man page  http://linux.die.net/man/2/removexattr */
	{ 2,	TF,	"removexattr"	}, /* 235 */
	{ 2,	TF,	"lremovexattr"	}, /* 236 */
	{ 2,	TD,	"fremovexattr"	}, /* 237 */

	{ 2,	TS,	"tkill"		}, /* 238 */
	{ 4,	TD,	"sendfile64"	}, /* 239 */
	{ 6,	0,	"futex"		}, /* 240 */
	{ 3,	0,	"sched_setaffinity" },/* 241 */
	{ 3,	0,	"sched_getaffinity" },/* 242 */
	{ 1,	0,	"set_thread_area" }, /* 243 */
	{ 1,	0,	"get_thread_area" }, /* 244 */
	{ 2,	0,	"io_setup"	}, /* 245 */
	{ 1,	0,	"io_destroy"	}, /* 246 */
	{ 5,	0,	"io_getevents"	}, /* 247 */
	{ 3,	0,	"io_submit"	}, /* 248 */
	{ 3,	0,	"io_cancel"	}, /* 249 */
	{ 4,	0,	"fadvise64"	}, /* 250 : argument 4 or 5? man page says 4 but strace file says 5
								"sys_fadvise64() function is obsolete" */

/* 251 is available for reuse (was briefly sys_set_zone_reclaim) */
	{ 5,	0,	"SYS_251"	}, /* 251 */

	{ 1,	TP,	"exit_group"}, /* 252 */
	{ 3,	0,	"lookup_dcookie"}, /* 253 : argument 3 or 4? man page says 3 but strace file says 4 */
	{ 1,	0,	"epoll_create"	}, /* 254 */
	{ 4,	TD,	"epoll_ctl"	}, /* 255 */
	{ 4,	TD,	"epoll_wait"	}, /* 256 */
	{ 5,	0,	"remap_file_pages"}, /* 257 */
	{ 1,	0,	"set_tid_address"}, /* 258 */

/* http://linux.die.net/man/2/timer_create */
	{ 3,	0,	"timer_create"	}, /* 259 : can't find man page in linux */
	{ 4,	0,	"timer_settime"	}, /* 260 : can't find man page in linux */
	{ 2,	0,	"timer_gettime"	}, /* 261 : can't find man page in linux */
	{ 1,	0,	"timer_getoverrun"}, /* 262 : can't find man page in linux */
	{ 1,	0,	"timer_delete"	}, /* 263 */

	{ 2,	0,	"clock_settime"	}, /* 264 */
	{ 2,	0,	"clock_gettime"	}, /* 265 */
	{ 2,	0,	"clock_getres"	}, /* 266 */

/* http://linux.die.net/man/2/clock_nanosleep */
	{ 4,	0,	"clock_nanosleep"}, /* 267 : can't find man page in linux */

/* http://linux.die.net/man/2/statfs64 says 3 arguments but not in man page */
	{ 3,	TF,	"statfs64"	}, /* 268 */
	{ 3,	TD,	"fstatfs64"	}, /* 269 */

	{ 3,	TS,	"tgkill"	}, /* 270 */
	{ 2,	TF,	"utimes"	}, /* 271 */
	{ 4,	0,	"fadvise64_64"	}, /* 272 : argument 4 or 6? man page says 4 but strace file says 6 */
	{ 5,	0,	"vserver"	}, /* 273 : not implemented in linux 2.6.22 */
	{ 6,	0,	"mbind"		}, /* 274 */
	{ 5,	0,	"get_mempolicy"	}, /* 275 */
	{ 3,	0,	"set_mempolicy"	}, /* 276 : can't find man page in linux, http://linux.die.net/man/2/set_mempolicy */
	{ 4,	0,	"mq_open"	}, /* 277 : variable arguments 2 or 4 */
	{ 1,	0,	"mq_unlink"	}, /* 278 */
	{ 5,	0,	"mq_timedsend"	}, /* 279 */
	{ 5,	0,	"mq_timedreceive" }, /* 280 */
	{ 2,	0,	"mq_notify"	}, /* 281 */
	{ 3,	0,	"mq_getsetattr"	}, /* 282 */
	{ 5,	0,	"kexec_load" }, /* 283 : can't find man page in linux */
	{ 4,	TP,	"waitid"	}, /* 284 : argument 4 or 5? man page says 4 but strace file says 5 */

/* #define __NR_sys_setaltroot	285 - commented out in unistd_32.h */
	{ 5,	0,	"SYS_285"	}, /* 285 */

	{ 5,	0,	"add_key"	}, /* 286 : can't find man page in linux */
	{ 4,	0,	"request_key"	}, /* 287 : can't find man page in linux */
	{ 5,	0,	"keyctl"	}, /* 288 : can't find man page in linux, variable arguments */
	{ 3,	0,	"ioprio_set"	}, /* 289 */
	{ 2,	0,	"ioprio_get"	}, /* 290 */
	{ 0,	TD,	"inotify_init"	}, /* 291 */
	{ 3,	TD,	"inotify_add_watch" }, /* 292 */
	{ 2,	TD,	"inotify_rm_watch" }, /* 293 */
	{ 4,	0,	"migrate_pages"	}, /* 294 : can't find man page in linux */
	{ 4,	TD|TF,	"openat"	}, /* 295 : variable arguments 3 or 4 */
	{ 3,	TD|TF,	"mkdirat"	}, /* 296 */
	{ 4,	TD|TF,	"mknodat"	}, /* 297 */
	{ 5,	TD|TF,	"fchownat"	}, /* 298 */
	{ 3,	TD|TF,	"futimesat"	}, /* 299 */
	{ 4,	TD|TD,	"fstatat64"	}, /* 300 */
	{ 3,	TD|TF,	"unlinkat"	}, /* 301 */
	{ 4,	TD|TF,	"renameat"	}, /* 302 */
	{ 5,	TD|TF,	"linkat"	}, /* 303 */
	{ 3,	TD|TF,	"symlinkat"	}, /* 304 */
	{ 4,	TD|TF,	"readlinkat"	}, /* 305 */
	{ 4,	TD|TF,	"fchmodat"	}, /* 306 : argument 3 or 4? man page says 4 but strace file says 3 */
	{ 4,	TD|TF,	"faccessat"	}, /* 307 : argument 3 or 4? man page says 4 but strace file says 3 */
	{ 6,	TD,	"pselect6"	}, /* 308 */
	{ 4,	TD,	"ppoll"		}, /* 309 : argument 4 or 5? man page says 4 but strace file says 5 */
	{ 1,	TP,	"unshare"	}, /* 310 */
	{ 2,	0,	"set_robust_list" }, /* 311 : can't find man page in linux, http://linux.die.net/man/2/set_robust_list*/
	{ 3,	0,	"get_robust_list" }, /* 312 : can't find man page in linux */
	{ 6,	TD,	"splice"	}, /* 313 */
	{ 4,	TD,	"sync_file_range" }, /* 314 */
	{ 4,	TD,	"tee"		}, /* 315 */
	{ 4,	TD,	"vmsplice"	}, /* 316 */

/* This syscall is implemented only on the i386 and IA-64 architectures since kernel 2.6. */
	{ 6,	0,	"move_pages"	}, /* 317 : can't find man page in linux */
	{ 3,	0,	"getcpu"	}, /* 318 : can't find man page in linux */
	{ 5,	TD,	"epoll_pwait"	}, /* 319 */
	{ 4,	TD|TF,	"utimensat"	}, /* 320 : can't find man page in linux */
	{ 3,	TD|TS,	"signalfd"	}, /* 321 */
	{ 2,	TD,	"timerfd_create"}, /* 322 */
	{ 2,	TD,	"eventfd"	}, /* 323 : argument 1 or 2? man page says 2 but strace file says 1 */
	{ 4,	TF,	"fallocate"	}, /* 324 : argument 4 or 6? man page says 4 but strace file says 6 */
	{ 4,	TD,	"timerfd_settime"}, /* 325 */
	{ 2,	TD,	"timerfd_gettime"}, /* 326 */

	{ 0,	0,	"signalfd4"}, /* 327 : can't find man page in linux */
	{ 0,	0,	"eventfd2"}, /* 328 : can't find man page in linux */
	{ 0,	0,	"epoll_create1"}, /* 329 : can't find man page in linux */
	{ 0,	0,	"dup3"}, /* 330 : can't find man page in linux */
	{ 0,	0,	"pipe2"}, /* 331 : can't find man page in linux */
	{ 0,	0,	"inotify_init1"}, /* 332 : can't find man page in linux */


