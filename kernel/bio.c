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

#define NBUCKET 13
#define hash(dev, blockno) (dev * blockno % NBUCKET)
struct {
  struct buf buf[NBUF];

  struct spinlock lock[NBUCKET];
  struct buf bukets[NBUCKET];
} bcache;

void
binit(void)
{
  for (int buck_id = 0; buck_id < NBUCKET; buck_id++) {
    initlock(&bcache.lock[buck_id], "bucket");
  }

  acquire(&bcache.lock[0]);
  bcache.bukets[0].next = &bcache.buf[0];
  for (int i = 1; i < NBUF; i++) {
    bcache.buf[i-1].next = &bcache.buf[i];
    initsleeplock(&bcache.buf[i].lock, "buf");
  }
  release(&bcache.lock[0]);
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int buck_id = hash(dev, blockno);
  // Is the block already cached?
  acquire(&bcache.lock[buck_id]);
  b = bcache.bukets[buck_id].next;
  while(b) {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[buck_id]);
      acquiresleep(&b->lock);
      return b;
    }
    b = b->next;
  }
  release(&bcache.lock[buck_id]);

  uint max_timestamp = 0;
  int is_better = 0;
  struct buf *b_lru_pre = (void*) 0;
  int lru_buck = -1;

  struct buf *b_pre;
  for (int i = 0; i < NBUCKET; i++) {
    acquire(&bcache.lock[i]);
    b_pre = &bcache.bukets[i];
    while (b_pre->next) {
      if (b_pre->next->refcnt == 0 && b_pre->next->timestemp >= max_timestamp) {
        max_timestamp = b_pre->next->timestemp;
        is_better = 1;
        b_lru_pre = b_pre;
      }
      b_pre = b_pre->next;
    }
    if (is_better) {
      if (i != 0) {
        release(&bcache.lock[lru_buck]);
      }
      lru_buck = i;
    } else {
      release(&bcache.lock[i]);
    }
    is_better = 0;
  }
  struct buf *b_lru;
  if (b_lru_pre) {
    b_lru = b_lru_pre->next;
    b_lru_pre->next = b_lru->next;
    release(&bcache.lock[lru_buck]);
  } else {
    panic("bget: no buffers");
  }
  
  acquire(&bcache.lock[buck_id]);
  b_lru->next = bcache.bukets[buck_id].next;
  bcache.bukets[buck_id].next =b_lru;

  b = bcache.bukets[buck_id].next;
  while (b) {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[buck_id]);
      acquiresleep(&b->lock);
      return b;
    }
    b = b->next;
  }

  b_lru->dev = dev;
  b_lru->blockno = blockno;
  b_lru->valid = 0;
  b_lru->refcnt = 1;
  acquiresleep(&b_lru->lock);
  release(&bcache.lock[buck_id]);
  return b_lru;
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

  releasesleep(&b->lock);

  int buck_id = hash(b->dev, b->blockno);

  acquire(&bcache.lock[buck_id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->timestemp = ticks;
  }
  
  release(&bcache.lock[buck_id]);
}

void
bpin(struct buf *b) {
  int buck_id = hash(b->dev, b->blockno);
  acquire(&bcache.lock[buck_id]);
  b->refcnt++;
  release(&bcache.lock[buck_id]);
}

void
bunpin(struct buf *b) {
  int buck_id = hash(b->dev, b->blockno);
  acquire(&bcache.lock[buck_id]);
  b->refcnt--;
  release(&bcache.lock[buck_id]);
}


