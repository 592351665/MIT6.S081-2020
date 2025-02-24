struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  uint lastuse;//时间戳，该缓冲区最后使用时间
  // struct buf *prev; // LRU cache list
  struct buf *next;//单向链表
  uchar data[BSIZE];
};

