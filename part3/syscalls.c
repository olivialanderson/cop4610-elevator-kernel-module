#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>

/* Function pointers ("stubs"). NULL until the elevator module loads and sets them. */
long (*STUB_start_elevator)(void) = NULL;
long (*STUB_issue_request)(int, int, int) = NULL;
long (*STUB_stop_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_start_elevator);
EXPORT_SYMBOL(STUB_issue_request);
EXPORT_SYMBOL(STUB_stop_elevator);

/* Syscall 548 */
SYSCALL_DEFINE0(start_elevator)
{
    if (STUB_start_elevator != NULL)
        return STUB_start_elevator();
    return -ENOSYS;
}

/* Syscall 549 */
SYSCALL_DEFINE3(issue_request, int, start_floor, int, destination_floor, int, type)
{
    if (STUB_issue_request != NULL)
        return STUB_issue_request(start_floor, destination_floor, type);
    return -ENOSYS;
}

/* Syscall 550 */
SYSCALL_DEFINE0(stop_elevator)
{
    if (STUB_stop_elevator != NULL)
        return STUB_stop_elevator();
    return -ENOSYS;
}
