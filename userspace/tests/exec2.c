#include "stdio.h"
#include  "../libc/include/unistd.h"
#include "../libc/include/assert.h"

int main(int argc, char *argv[])
{
    printf("\ntest execargssimple\n");

    char *args[] = {"test1", "test2", NULL};
    execv("/usr/execprintargs.sweb", args);

    assert(1); // should never return from exec

    return 0;
}
