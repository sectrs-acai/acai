//
// Created by b on 1/18/23.
//
#include <linux/uaccess.h>
#include <linux/mmu_context.h>
#include <linux/workqueue.h>
#include <linux/mm_types.h>
#include <linux/sched/mm.h>
#include <linux/sched/task.h>

#include "fh_driver_util.h"

struct protect_pid_struct {
    struct work_struct work;
    struct mm_struct *mm;
    unsigned long start;
    size_t len;
    unsigned long prot;
    int ret_value;
};

int (*orig_mprotect_pkey)(
        unsigned long start,
        size_t len,
        unsigned long prot,
        int pkey) = NULL;


static void protect_pid_worker(struct work_struct *work)
{
    struct protect_pid_struct *data = container_of(work,
    struct protect_pid_struct,
    work);

    use_mm(data->mm);
    data->ret_value = orig_mprotect_pkey(data->start,
                                         data->len,
                                         data->prot, -1);
    unuse_mm(data->mm);
}


static long protect_pid(
        pid_t pid,
        unsigned long addr,
        size_t len,
        unsigned long prot,
)
{
    long ret;
    struct suprotect_request params;
    struct pid *pid_struct = NULL;
    struct task_struct *task = NULL;
    struct mm_struct *mm = NULL;
    struct suprotect_work work;


    /* Find the task by the pid */
    pid_struct = find_get_pid(params.pid);
    if (!pid_struct)
        return -ESRCH;

    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task) {
        ret = -ESRCH;
        goto out;
    }

    /* Get the mm of the task */
    mm = get_task_mm(task);
    if (!mm) {
        ret = -ESRCH;
        goto out;
    }

    /* Initialize work */
    INIT_WORK(&work.work, suprotect_work);

    work.mm = mm;
    work.start = (unsigned long) params.addr;
    work.len = params.len;
    work.prot = params.prot;

    /* Queue the work */
    (void) schedule_work(&work.work);

    /* Wait for completion of the work */
    flush_work(&work.work);
    ret = work.ret_value;

    out:
    if (mm) mmput(mm);
    if (task) put_task_struct(task);
    if (pid_struct) put_pid(pid_struct);

    return ret;
}
