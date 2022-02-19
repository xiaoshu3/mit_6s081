// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"


#define  LOW_MEM  0x80000000
#define PAGING_MEMORY (PHYSTOP - LOW_MEM)
#define PAGING_PAGES (PAGING_MEMORY>>12)
#define MAP_NR(addr) (((addr)-LOW_MEM)>>12)

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  unsigned int mem_map[PAGING_PAGES];
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  memset(kmem.mem_map,0,sizeof(kmem.mem_map));
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    kmem.mem_map[MAP_NR((uint64)p)] = 1;
    kfree(p);
  }
    
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{ 
  
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  acquire(&kmem.lock);
  int n = kmem.mem_map[MAP_NR((uint64)pa)];
  if(n == 1){
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    r->next = kmem.freelist;
    kmem.freelist = r;
    kmem.mem_map[MAP_NR((uint64)pa)] = 0;
  }
  else if(n > 1){
    --kmem.mem_map[MAP_NR((uint64)pa)];
  }
  else{
    printf("trying to free free page %p\n",pa);
    // release(&kmem.lock);
    // return ;
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    if(kmem.mem_map[MAP_NR((uint64)r)]){
      printf("kalloc error\n");
    }
    kmem.mem_map[MAP_NR((uint64)r)] = 1;
  }
    
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

int num_map(uint64 pa){
  acquire(&kmem.lock);
  int n  =  kmem.mem_map[MAP_NR(pa)];
  release(&kmem.lock);
  return n;
}

void add_map(uint64 pa){
  acquire(&kmem.lock);
  ++kmem.mem_map[MAP_NR(pa)];
  release(&kmem.lock);
}
