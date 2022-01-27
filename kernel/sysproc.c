#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc);

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  int n;
  uint64 va,toaddr;uint tmp = 0;
  struct proc *p = myproc();
  pte_t *pte;
  if(argaddr(0,&va) <0 || argint(1,&n) <0 || argaddr(2,&toaddr) < 0)
    return -1;
  if(n > 32) return -1;
  for(int i=0;i<n;i++){
    pte = walk(p->pagetable, va, 0);
    if(pte == 0 ) return -1;
    if((*pte & PTE_V) == 0 || (*pte & PTE_U) == 0) return -1;
    if(*pte & PTE_A){
      tmp |= (1<<i);
      *pte &= (~PTE_A);
    }
    va += PGSIZE;
  }
  if(copyout(p->pagetable,toaddr,(char*)&tmp,sizeof(tmp)) <0 ) return -1;

  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
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
