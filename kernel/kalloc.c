// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "memlayout.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem;

void init_ref_count()
{
  for (int i = 0; i < (PHYSTOP - KERNBASE) / PGSIZE; i++)
  {
    ref_count[i] = 0;
  }
}

void kinit()
{
  initlock(&kmem.lock, "kmem");
  init_ref_count();
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
  {
    acquire(&kmem.lock);
    ref_count[REF_COUNT_INDEX((uint64)p)] = 1;
    release(&kmem.lock);
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&kmem.lock);
  int rc = --ref_count[REF_COUNT_INDEX((uint64)pa)];
  if (rc > 0)
  {
    release(&kmem.lock);
    return;
  }
  else if (rc < 0)
  {
    printf("ref couont of address %p is %d\n", (uint64)pa, rc);
    panic("kfree: ref count is less than 0");
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  r->next = kmem.freelist;
  kmem.freelist = r;
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
  if (r)
  {
    kmem.freelist = r->next;
    int *rc = &ref_count[REF_COUNT_INDEX((uint64)r)];
    if (*rc != 0)
      panic("kalloc: bad rc");
    (*rc)++;
  }
  release(&kmem.lock);

  if (r)
  {
    memset((char *)r, 5, PGSIZE); // fill with junk
  }
  return (void *)r;
}
