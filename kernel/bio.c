// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"


#define NR_HASH 17
struct {
  struct spinlock lock;
  struct buf head;
}hash_table[NR_HASH];
#define _hashfn(dev,block) (((uint64)(dev^block))%NR_HASH)



struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;


static struct buf* find_buf(uint dev, uint blockno){
  uint64 n  = _hashfn(dev,blockno);
  struct buf*b;
  acquire(&hash_table[n].lock);
  for(b = hash_table[n].head.next_hash;b != &(hash_table[n].head);b = b->next_hash){
    if(b->dev == dev && b->blockno == blockno){
      if(b->refcnt == 0) b->valid = 0;
      b->refcnt++;
      b->num = n;
      release(&hash_table[n].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&hash_table[n].lock);
  return 0;
}

static void insert_hash(struct buf*b,uint dev,uint blockno){
  uint64 n  = _hashfn(dev,blockno);
  acquire(&hash_table[n].lock);

  b->num =  n;
  b->next_hash = hash_table[n].head.next_hash;
  b->prev_hash = &hash_table[n].head;

  hash_table[n].head.next_hash->prev_hash = b;
  hash_table[n].head.next_hash = b;

  release(&hash_table[n].lock);
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }

  for(int i=0;i<NR_HASH;i++){
    initlock(&(hash_table[i].lock),"bcache");
    hash_table[i].head.prev_hash = &hash_table[i].head;
    hash_table[i].head.next_hash = &hash_table[i].head;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // acquire(&bcache.lock);

  // // Is the block already cached?
  // for(b = bcache.head.next; b != &bcache.head; b = b->next){
  //   if(b->dev == dev && b->blockno == blockno){
  //     b->refcnt++;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  b = find_buf(dev,blockno);
  if(b) return b;
  
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  acquire(&bcache.lock);
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;

      b->next->prev = b->prev;
      b->prev->next = b->next;
      insert_hash(b,dev,blockno);

      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock);
  //panic("bget: no buffers");

  // for(int i=0;i<NR_HASH;i++){
  //   acquire(&hash_table[i].lock);
  //   for(b = hash_table[i].head.next_hash;b!= &hash_table[i].head;b = b->next_hash){
  //     if(b->refcnt == 0){
  //       b->dev = dev;
  //       b->blockno = blockno;
  //       b->valid = 0;
  //       b->refcnt = 1;

  //       b->next_hash->prev_hash = b->prev_hash;
  //       b->prev_hash->next_hash = b->next_hash;

  //       insert_hash(b,dev,blockno);

  //       release(&hash_table[i].lock);
  //       acquiresleep(&b->lock);
  //       return b;
  //     }
  //   }
  //   release(&hash_table[i].lock);
  // }
  
  for(int i=0;i<NR_HASH;i++){
    acquire(&hash_table[i].lock);
    for(b = hash_table[i].head.prev_hash;b!=&hash_table[i].head;b= b->prev_hash){
      if(b->refcnt ==0){
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        b->next_hash->prev_hash = b->prev_hash;
        b->prev_hash->next_hash = b->next_hash;
        release(&hash_table[i].lock);

        insert_hash(b,dev,blockno);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&hash_table[i].lock);
  }
  panic("bget: no buffers");

} 

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  uint n = b->num;
  releasesleep(&b->lock);

  acquire(&hash_table[n].lock);
  if(!(b->refcnt--)) panic("Trying to free free buffer");
  release(&hash_table[n].lock);
  
  
  // if (b->refcnt == 0) {
  //   acquire(&bcache.lock);
  //   // no one is waiting for it.
  //   // b->next->prev = b->prev;
  //   // b->prev->next = b->next;
  //   acquire(&hash_table[n].lock);
  //   b->next_hash->prev_hash  = b->prev_hash;
  //   b->prev_hash->next_hash = b->next_hash;
  //   release(&hash_table[n].lock);

  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;

  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  //   release(&bcache.lock);
  // }
}

void
bpin(struct buf *b) {
  // acquire(&bcache.lock);
  // b->refcnt++;
  // release(&bcache.lock);
  uint n = b->num;
  acquire(&hash_table[n].lock);
  b->refcnt++;
  release(&hash_table[n].lock);
}

void
bunpin(struct buf *b) {
  // acquire(&bcache.lock);
  // b->refcnt--;
  // release(&bcache.lock);
  uint n = b->num;
  acquire(&hash_table[n].lock);
  b->refcnt--;
  release(&hash_table[n].lock);
}


