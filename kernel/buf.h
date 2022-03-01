struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev; // LRU cache list
  struct buf *next;

  struct buf *prev_hash;
  struct buf *next_hash;
  uint num;
  uchar data[BSIZE];
};

