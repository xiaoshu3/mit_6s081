#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h"

#define NULL  (void*)0
#define VM_SIZE 16

int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

struct{
    struct spinlock lock;
    struct vm_area_struct vm[VM_SIZE];
}vm_array;

void vm_struct_init(struct vm_area_struct*t){
    t->vm_start = t->vm_end = 0;
    t->flags = t->length = t->offset = t->prot = 0;
    t->next =  (void*)0;
    t->valid = 0;
    t->shared = 0;
}
void vminit(){
    initlock(&vm_array.lock,"mmap");
    for(int i=0;i<VM_SIZE;i++){
        // vm_array.vm[i].valid = 0;
        // vm_array.vm[i].flags = vm_array.vm[i].prot = 0;
        // vm_array.vm[i].offset = 0;
        vm_struct_init(&vm_array.vm[i]);
    }
}

uint64
sys_mmap(void ){
    uint64 p;
    int length,off;
    int prot,flags;
    struct file *f;
    struct proc *proc = myproc();

    if(argaddr(0,&p) <0 || argint(1,&length) <0  || argint(2,&prot) <0 || 
        argint(3,&flags) <0 || argfd(4,0,&f)<0 || argint(5,&off) <0)
        return -1;
    if(!(f->readable)) return -1;
    if(flags & MAP_SHARED && !(f->writable) && (prot & PROT_WRITE)) return -1;
    int i;
    acquire(&vm_array.lock);
    for(i=0;i<VM_SIZE;i++){
        if(vm_array.vm[i].valid == 0){
            vm_array.vm[i].valid =1 ;
            vm_array.vm[i].length = length;
            vm_array.vm[i].f = filedup(f);
            vm_array.vm[i].prot = prot;
            vm_array.vm[i].offset = off;
            vm_array.vm[i].flags = flags;
            break;
        }
    }
    release(&vm_array.lock);
    if(i == VM_SIZE) return -1;

    // vm_array.vm[i].ref =1;

    //printf("flags %p\n",flags);
    uint64 last_start,new_start;
    struct vm_area_struct* tmp = proc->mmap;
    if(tmp == (void*)0) last_start = TRAPFRAME ;
    
    while(tmp!= NULL && tmp->next != NULL) tmp = tmp->next;
    if(tmp!=NULL){
        //printf("tmp->vm_start %p\n",tmp->vm_start);
        last_start = tmp->vm_start;
    }
    //printf("last_start %p\n",last_start);

    length = (length + PGSIZE - 1) & ~(PGSIZE-1);
    new_start = last_start - length;

    
    if(new_start <= proc->sz){
        vm_array.vm[i].valid = 0;
        return -1;
    }

    vm_array.vm[i].vm_start = new_start;
    vm_array.vm[i].vm_end = last_start;
    vm_array.vm[i].next = tmp!=NULL? tmp->next : (void*)0;
    if(tmp != NULL) tmp->next = &vm_array.vm[i];
    else proc->mmap = &vm_array.vm[i];

    return vm_array.vm[i].vm_start;

}

uint64
sys_munmap(void ){
    uint64 p;
    int length;
    struct proc *proc = myproc();
    struct vm_area_struct* tmp = proc->mmap;
    int flags;

    if(argaddr(0,&p) <0 || argint(1,&length) <0) return -1;
    while(tmp){
        if(tmp->vm_start == p) {
            //printf("find 1\n");
            break;
        }
        tmp = tmp->next;
    }
    if(!tmp || tmp->vm_start != p) return -1;
    if((tmp->vm_end - tmp->vm_start) < length) return -1;

    pte_t *pte;
    if((pte = walk(proc->pagetable, PGROUNDDOWN(tmp->vm_start), 0)) == 0)
        panic("sys_munmap walk");
    
    flags = tmp->flags;
    if(flags & MAP_SHARED){
        if(*pte & PTE_D) filewrite(tmp->f,tmp->vm_start,length);
        // if(filewrite(tmp->f,tmp->vm_start,length) < 0 )
        //     // return -1;
        //     ;
    }
    
    if(*pte & PTE_V)
        uvmunmap(proc->pagetable,PGROUNDDOWN(tmp->vm_start),length/PGSIZE,1);

    tmp->vm_start += length;
   
    if(tmp->vm_start == tmp->vm_end){
        struct vm_area_struct* tmp_prev = proc->mmap;
        // while(tmp_prev && tmp_prev ->next != tmp) tmp_prev = tmp_prev->next;
        if(tmp == tmp_prev){
            proc->mmap = tmp->next;
        }
        else{
             while(tmp_prev != NULL){
                if(tmp_prev->next == tmp) {
                    //printf("find\n");
                    break;
                }
                tmp_prev = tmp_prev->next;
            }
            if(tmp_prev == NULL) panic("sys_munmap  exit");
            tmp_prev->next  = tmp->next;
        }
       
        fileclose(tmp->f);
        vm_struct_init(tmp);

        //if(proc->mmap == NULL) printf("yes except\n");
    } 
    return 0;
}

struct vm_area_struct* allocate_vm(){
    int i;
    acquire(&vm_array.lock);
    for(i=0;i<VM_SIZE;i++){
        if(vm_array.vm[i].valid == 0){
            break;
        }
    }
    release(&vm_array.lock);
    if(i == VM_SIZE) return 0;

    vm_array.vm[i].valid =1;
    return &vm_array.vm[i];
}

void copy_vm(struct vm_area_struct* t,struct vm_area_struct* p){
    t->f = p->f;
    t->flags = p->flags;
    t->length = p->length;
    t->next = p->next;
    t->offset = p->offset;
    t->vm_start = p->vm_start;
    t->vm_end = p->vm_end;
    t->prot = p->prot;
}

void handle_share(struct vm_area_struct* t){
    if(!t || !t->shared) return;
    struct proc *proc = myproc();
    if(t->shared){
        t->shared--;
        proc->mmap = allocate_vm();
        copy_vm(proc->mmap,t);
        t = t->next;
    }
    struct vm_area_struct* des = proc->mmap;
    while(t){
        struct vm_area_struct* tmp  = allocate_vm();
        copy_vm(tmp,t);
        des->next = tmp;

        des = des->next;
        t = t->next;
    }

}