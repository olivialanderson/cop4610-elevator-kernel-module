#include <unistd.h>

/* Makes exactly five system calls beyond what an empty program makes.
 * Only unistd.h is used so no extra library setup calls are added. */
int main(void)
{
    getpid();                   /* 1: getpid  - this process's ID */
    getppid();                  /* 2: getppid - parent process's ID */
    getuid();                   /* 3: getuid  - user ID */
    getgid();                   /* 4: getgid  - group ID */
    write(1, "hello\n", 6);     /* 5: write   - 6 bytes to stdout (fd 1) */
    return 0;
}
