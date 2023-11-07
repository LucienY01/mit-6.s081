#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if (argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if (argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  backtrace();

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sigalarm(void)
{
  struct proc *p = myproc();
  if (argint(0, &p->ticks) < 0)
    return -1;

  if (argaddr(1, &p->handler) < 0)
    return -1;

  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();

  p->trapframe->epc = p->alarmtrapframe->epc;

  // restore states
  p->trapframe->ra = p->alarmtrapframe->ra;
  p->trapframe->sp = p->alarmtrapframe->sp;
  p->trapframe->gp = p->alarmtrapframe->gp;
  p->trapframe->tp = p->alarmtrapframe->tp;
  p->trapframe->t0 = p->alarmtrapframe->t0;
  p->trapframe->t1 = p->alarmtrapframe->t1;
  p->trapframe->t2 = p->alarmtrapframe->t2;
  p->trapframe->s0 = p->alarmtrapframe->s0;
  p->trapframe->s1 = p->alarmtrapframe->s1;
  p->trapframe->a0 = p->alarmtrapframe->a0;
  p->trapframe->a1 = p->alarmtrapframe->a1;
  p->trapframe->a2 = p->alarmtrapframe->a2;
  p->trapframe->a3 = p->alarmtrapframe->a3;
  p->trapframe->a4 = p->alarmtrapframe->a4;
  p->trapframe->a5 = p->alarmtrapframe->a5;
  p->trapframe->a6 = p->alarmtrapframe->a6;
  p->trapframe->a7 = p->alarmtrapframe->a7;
  p->trapframe->s2 = p->alarmtrapframe->s2;
  p->trapframe->s3 = p->alarmtrapframe->s3;
  p->trapframe->s4 = p->alarmtrapframe->s4;
  p->trapframe->s5 = p->alarmtrapframe->s5;
  p->trapframe->s6 = p->alarmtrapframe->s6;
  p->trapframe->s7 = p->alarmtrapframe->s7;
  p->trapframe->s8 = p->alarmtrapframe->s8;
  p->trapframe->s9 = p->alarmtrapframe->s9;
  p->trapframe->s1 = p->alarmtrapframe->s1;
  p->trapframe->s1 = p->alarmtrapframe->s1;
  p->trapframe->t3 = p->alarmtrapframe->t3;
  p->trapframe->t4 = p->alarmtrapframe->t4;
  p->trapframe->t5 = p->alarmtrapframe->t5;
  p->trapframe->t6 = p->alarmtrapframe->t6;

  p->is_handling = 0;
  return 0;
}
