#pragma once

#define fd_stdin 0
#define fd_stdout 1
#define fd_stderr 2

#define sc_exit 1
#define sc_fork 2
#define sc_read 3
#define sc_write 4
#define sc_open 5
#define sc_close 6
#define sc_execv 7
#define sc_lseek 19
#define sc_pseudols 43
#define sc_dirty 98
#define sc_clean 99
#define sc_outline 105
#define sc_sched_yield 158
#define sc_createprocess 191
#define sc_trace 252
#define sc_pthread_create 300
#define sc_pthread_exit 301
#define sc_pthread_join 302
#define sc_pthread_cancel 303
#define sc_pthread_setcancelstate 304
#define sc_pthread_setcanceltype 305
#define sc_pthread_detach 306
#define sc_waitpid 601
#define sc_clock 1000
#define sc_sleep 1001

#define sc_sbrk 4001
#define sc_brk 4002


