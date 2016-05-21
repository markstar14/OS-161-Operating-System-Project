#ifndef PROCCALLS_H_
#define PROCCALLS_H_

#include <../../../user/include/sys/null.h>
#include <limits.h>
#include <types.h>
#include <../../arch/mips/include/trapframe.h> 


struct process{
        struct thread* thd;
        int exit_code;
        pid_t par_pid;
        bool exit;
	struct  semaphore* sem;
};

struct spinlock *proclock;

struct process *proc_tbl[PID_LIMIT];

pid_t find_pid(void);

void proc_create(struct thread *td, pid_t id, pid_t parent_pid);

void enter(void* tf, unsigned long addrs);

void sys_exit(int exitcode);

int sys_fork(struct trapframe *tf, int *retval);

pid_t sys_getpid(void);

int sys_waitpid(pid_t pid, int *status, int options, int *retval);

int sys_execv(const char *program, char **args);

#endif

