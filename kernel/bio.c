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

#define NBUFMAP_BUCKET 13
#define HASH(dev,blockno) ((((dev)<<27)|(blockno))%NBUFMAP_BUCKET)

struct {
  struct spinlock eviction_lock;
  struct buf buf[NBUF];
  
  struct buf bufmap[NBUFMAP_BUCKET];//哈希表
  struct spinlock bufmap_locks[NBUFMAP_BUCKET];
} bcache;

void
binit(void)
{
  //初始化哈希表bufmap
  for(int i=0;i<NBUFMAP_BUCKET;i++){
    initlock(&bcache.bufmap_locks[i],"bcache_bufmap");
    bcache.bufmap[i].next=0;//桶的链表头指针初始化为0，表示桶中没有缓冲区
  }
  //初始化buffers
  for(int i=0;i<NBUF;i++){
    struct buf *b=&bcache.buf[i];//必须加&，取地址
    initsleeplock(&b->lock,"buffer");
    b->refcnt=0;
    b->lastuse=0;
    b->next=bcache.bufmap[0].next;//放在第一个桶的链表头
    bcache.bufmap[0].next=b;
  }
  //初始化驱逐锁
    initlock(&bcache.eviction_lock,"bcache_eviction");
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint key=HASH(dev,blockno);

  acquire(&bcache.bufmap_locks[key]);
  //检查所需的blockno是否在key哈希桶中
  for(b=bcache.bufmap[key].next;b;b=b->next){
    if(b->dev==dev&&b->blockno==blockno){
      b->refcnt++;
      release(&bcache.bufmap_locks[key]);
      acquiresleep(&b->lock);//在返回使用缓存块b前，要获取该块的睡眠锁
      return b;
    }
  }

  release(&bcache.bufmap_locks[key]);
  acquire(&bcache.eviction_lock);//驱逐锁

  //获取淘汰锁后再次进行遍历查询，确保不会为同一区块创建多个缓存
  //该线程持有淘汰锁，不会再发生别的淘汰操作，所有哈希桶的链表结构不会改变；
  //因此无需持有对应桶的锁就可以进行 遍历 ，因为有了更强淘汰锁
  for(b=bcache.bufmap[key].next;b;b=b->next){
    if(b->dev==dev&&b->blockno==blockno){
      acquire(&bcache.bufmap_locks[key]);//此处需要获取桶锁，是为了确保refcnt++这个非原子操作
      b->refcnt++;
      release(&bcache.bufmap_locks[key]);
      release(&bcache.eviction_lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  struct buf *before_least=0;//要驱逐缓冲区的前一个
  uint holding_bucket=-1;//持有哪个哈希桶的锁
  for(int i=0;i<NBUFMAP_BUCKET;i++){
    acquire(&bcache.bufmap_locks[i]);
    int newfound=0;//标志位，该桶中是否有可淘汰的缓冲区
    for(b=&bcache.bufmap[i];b->next;b=b->next){
      if(b->next->refcnt==0&&(!before_least||b->next->lastuse<before_least->next->lastuse)){
        before_least=b;
        newfound=1;
      }
    }
    if(!newfound){
      release(&bcache.bufmap_locks[i]);
    }else{
      if(holding_bucket!=-1)release(&bcache.bufmap_locks[holding_bucket]);//保证只持有一个桶的锁
      holding_bucket=i;
    }
  }

  if(!before_least){
    panic("bget:no buffers");
  }
  b=before_least->next;

  //缓冲区的重新分配
  if(holding_bucket!=key){
    before_least->next=b->next;
    release(&bcache.bufmap_locks[holding_bucket]);
    acquire(&bcache.bufmap_locks[key]);
    b->next=bcache.bufmap[key].next;
    bcache.bufmap[key].next=b;
  }

  b->dev=dev;
  b->blockno=blockno;
  b->refcnt=1;
  b->valid=0;
  release(&bcache.bufmap_locks[key]);
  release(&bcache.eviction_lock);
  acquiresleep(&b->lock);
  return b;
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

void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint key=HASH(b->dev,b->blockno);
  acquire(&bcache.bufmap_locks[key]);
  b->refcnt--;
  if(b->refcnt==0){
    b->lastuse=ticks;
  }
  release(&bcache.bufmap_locks[key]);
}

void
bpin(struct buf *b) {
  uint key=HASH(b->dev,b->blockno);
  acquire(&bcache.bufmap_locks[key]);
  b->refcnt++;
  release(&bcache.bufmap_locks[key]);
}

void
bunpin(struct buf *b) {
  uint key=HASH(b->dev,b->blockno);
  acquire(&bcache.bufmap_locks[key]);
  b->refcnt--;
  release(&bcache.bufmap_locks[key]);
}


