// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

// struct run {
//   struct run *next;
// };

// struct {
//   struct spinlock lock;
//   struct run *freelist;
// } kmem;

void
kinit()
{
  // initlock(&kmem.lock, "kmem");
  init_cpus_memlock();
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
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
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  struct kmem *kmem = mycpu()->kmem;
  acquire(&kmem->lock);
  r->next = kmem->freelist;
  kmem->freelist = r;
  kmem->freenums +=1;
  release(&kmem->lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{

  struct run *r;
  struct kmem *my_kmem = mycpu()->kmem;
  if (my_kmem->freenums <= 0) {
    struct run *run_got;
    struct kmem *most_kmem = cpu_most_mems()->kmem;
    acquire(&most_kmem->lock);
    int block_got = most_kmem->freenums / 2;
    most_kmem->freenums -= block_got;
    run_got = most_kmem->freelist;
    for (int i = 0; i < most_kmem->freenums; i++) {
      run_got = run_got->next;
    }
    release(&most_kmem->lock);
    
    acquire(&my_kmem->lock);
    run_got->next = my_kmem->freelist;
    my_kmem->freelist = run_got;
    my_kmem->freelist += block_got;
    release(&my_kmem->lock);
  }

  acquire(&my_kmem->lock);
  r = my_kmem->freelist;
  my_kmem->freenums -= 1;

  if (my_kmem->freenums > 0) {
    my_kmem->freelist = r->next;
  }
  release(&my_kmem->lock);
  
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
