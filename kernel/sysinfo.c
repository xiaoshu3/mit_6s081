#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "fcntl.h"
#include "sysinfo.h"


uint64 count_freemem();
uint64 count_unused();
struct proc;

int sysinfo(uint64 addr){
    struct proc *p = myproc();
    struct sysinfo tmp;

    tmp.nproc = count_unused();
    tmp.freemem  = count_freemem();

    if(copyout(p->pagetable, addr, (char *)&tmp, sizeof(tmp)) < 0)
      return -1;
    return 0;
}

uint64 
sys_sysinfo(void){
    uint64 sinfo; // user pointer to struct sysinfo
    if(argaddr(0, &sinfo) < 0)
        return -1;
    return sysinfo(sinfo);
}
