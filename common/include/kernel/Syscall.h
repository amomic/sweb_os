#pragma once

#include <types.h>

class Syscall
{
  public:
    static size_t syscallException(size_t syscall_number, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5);

    static void exit(size_t exit_code);
    static void outline(size_t port, pointer text);

    static size_t write(size_t fd, pointer buffer, size_t size);
    static size_t read(size_t fd, pointer buffer, size_t count);
    static size_t close(size_t fd);
    static size_t open(size_t path, size_t flags);
    static void pseudols(const char *pathname, char *buffer, size_t size);

    static size_t createprocess(size_t path, size_t sleep);
    static void trace();
    static size_t pthread_create(pointer thread, pointer attr, void *(*start_routine)(void *), pointer arg,pointer wrapper);
    static size_t pthread_join(size_t thread, size_t return_val);
    static size_t pthread_detach(size_t thread);

    static void pthread_exit(void *value);

    static size_t pthread_cancel(size_t thread_id);
    static size_t pthread_setcancelstate(size_t state, size_t *oldstate);
    static size_t pthread_setcanceltype(size_t type, size_t *oldtype);

    static size_t fork();

    static size_t execv(char* path, char* const* argv);
    static pid_t waitpid(pid_t pid, int *status, int options);

    static size_t clock(void);
    static size_t thread_sleep(size_t seconds);

    static size_t brk(size_t end_data_segment);
    static size_t sbrk(size_t increment);

    static size_t get_dirty();

    static size_t get_clean();

    static void swapStack();
};

