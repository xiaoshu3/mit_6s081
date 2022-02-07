#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

uint64 sys_sigalarm(){
    int n;
    struct proc *p = myproc();
    void (*handler)();
    if(argint(0,&n) < 0 || argaddr(1,(uint64*)&handler) < 0 || n < 0)
        return -1;
    p->ticks = n;
    p->handler = handler;
    //printf("%p\n",handler);
    p->stat = 1;
    return 0;
}

uint64 sys_sigreturn(){
    struct proc *p = myproc();
    if(p->stat == 0){
        p->trapframe->epc  =p->trapframe->tmp_pc;
        p->trapframe->s1 = p->trapframe->tmp_s1;
        p->trapframe->s0 = p->trapframe->tmp_s0;
        p->trapframe->sp = p->trapframe->tmp_sp;
        p->trapframe->ra = p->trapframe->tmp_ra;

        p->trapframe->s2 = p->trapframe->tmp_s2;
        p->trapframe->s3 = p->trapframe->tmp_s3;
        p->trapframe->s4 = p->trapframe->tmp_s4;
        p->trapframe->s5 = p->trapframe->tmp_s5;
        p->trapframe->s6 = p->trapframe->tmp_s6;
        p->trapframe->s7 = p->trapframe->tmp_s7;
        p->trapframe->s8 = p->trapframe->tmp_s8;
        p->trapframe->s9 = p->trapframe->tmp_s9;
        p->trapframe->s10 = p->trapframe->tmp_s10;
        p->trapframe->s11= p->trapframe->tmp_s11;

        p->trapframe->a0= p->trapframe->tmp_a0;
        p->trapframe->a1= p->trapframe->tmp_a1;
        p->trapframe->a2= p->trapframe->tmp_a2;
        p->trapframe->a5= p->trapframe->tmp_a5;

        p->stat = 1;
    }
    return 0;
}
