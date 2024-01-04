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
// #define NBUF 2

struct bucket
{
  struct buf *buf;
  struct spinlock lock;
};

struct
{
  struct bucket bucket[NBUCKET];
  struct buf buf[NBUF];
  // struct spinlock lock;
} bcache;

uint hash(uint blockno)
{
  return blockno % NBUCKET;
}

void binit(void)
{
  // initlock(&bcache.lock, "bcache");

  int ibuf = 0;
  for (int i = 0; i < NBUCKET; i++)
  {
    struct bucket *bucket = &bcache.bucket[i];
    initlock(&bucket->lock, "bcache.bucket");

    // assign 2~3 bufs to each bucket at initialization.
    int count;
    if (0 <= i && i < 4)
      count = 3;
    else
      count = 2;
    for (int j = 0; j < count; j++)
    {
      bcache.buf[ibuf].next = bucket->buf;
      bucket->buf = &bcache.buf[ibuf];
      ibuf++;
    }
  }

  if (ibuf != NBUF)
    panic("invalid number of bufs assigned at init");

  for (int i = 0; i < NBUF; i++)
  {
    initsleeplock(&bcache.buf[i].lock, "bcache.buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;

  // Is the block already cached?
  uint i_target_bucket = hash(blockno);
  struct bucket *target_bucket = &bcache.bucket[i_target_bucket];
  acquire(&target_bucket->lock);
  for (b = target_bucket->buf; b != (void *)0; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&target_bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.

  // Search starts from current bucket.
  for (b = target_bucket->buf; b != NULL; b = b->next)
  {
    if (b->refcnt != 0)
      continue;

    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    release(&target_bucket->lock);
    acquiresleep(&b->lock);
    return b;
  }

  // Continue searching from next bucket.
  // Since we always acquire locks after current bucket in loop manner,
  // the only case in which deadlock occur is there is no free buffer.
  // It should panic in this case, so deacklock does not matter.
  for (int i = hash(blockno + 1); i != i_target_bucket; i = (i + 1) % NBUCKET)
  {
    struct bucket *bucket = &bcache.bucket[i];
    acquire(&bucket->lock);
    for (b = bucket->buf; b != NULL; b = b->next)
    {
      if (b->refcnt != 0)
        continue;

      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;

      struct bucket *old_bucket = bucket;

      if (old_bucket->buf == b)
      {
        old_bucket->buf = b->next;
        b->next = (void *)0;
      }
      else
      {
        // Search for the buf just previous to b.
        struct buf *prev_b = old_bucket->buf;
        for (prev_b = old_bucket->buf; prev_b->next != b; prev_b = prev_b->next)
          ;
        // Remove b from its bucket.
        prev_b->next = b->next;
        b->next = (void *)0;
      }

      b->next = target_bucket->buf;
      target_bucket->buf = b;

      release(&bucket->lock);
      release(&target_bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
    release(&bucket->lock);
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if (!b->valid)
  {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Time-stamp the buffer.
void brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  struct bucket *bucket = &bcache.bucket[hash(b->blockno)];
  acquire(&bucket->lock);
  b->refcnt--;
  release(&bucket->lock);
}

void bpin(struct buf *b)
{
  struct bucket *bucket = &bcache.bucket[hash(b->blockno)];
  acquire(&bucket->lock);
  b->refcnt++;
  release(&bucket->lock);
}

void bunpin(struct buf *b)
{
  struct bucket *bucket = &bcache.bucket[hash(b->blockno)];
  acquire(&bucket->lock);
  b->refcnt--;
  release(&bucket->lock);
}
