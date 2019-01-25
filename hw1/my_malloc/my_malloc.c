#include "my_malloc.h"
#include <assert.h>
#include <limits.h>
#include <unistd.h>
typedef struct meta_t {
  struct meta_t *prev;
  struct meta_t *next;
  size_t size;
  char *address;
} meta;

static meta *head = NULL; // freed address linked list
static unsigned long total = 0;
static unsigned long allocated = 0;
static meta *tail = NULL;
void cleanTail(meta *cur);
void printFreeList() {
  meta *cur = head;
  while (cur != NULL) {
    printf("cur: %p, cur->size: %lu, cur->address: %p\n", cur, cur->size,
           cur->address);
    cur = cur->next;
  }
}
// remove oen meta from the free list
void removeOne(meta *cur) {
  if (cur == head && cur == tail) {
    head = NULL;
    tail = NULL;
  } else if (cur == tail) {
    /*
    if (cur->prev != NULL) {
      cur->prev->next = NULL;
    }
    */
    cur->prev->next = NULL;
    tail = cur->prev;
  } else if (cur == head) {
    /*
    if (cur->next != NULL) {
      cur->next->prev = NULL;
    }
    */
    cur->next->prev = NULL;
    head = cur->next;
  } else {
    meta *tempPrev = cur->prev;
    meta *tempNext = cur->next;
    /*
    if (tempPrev != NULL) {
      tempPrev->next = cur->next;
    }
    if (tempNext != NULL) {
      tempNext->prev = cur->prev;
    }
    */
    tempPrev->next = cur->next;
    tempNext->prev = cur->prev;
  }
}
void *split(meta *cur, size_t size) {
  assert(cur->size >= size);
  if (cur->size < size + sizeof(meta)) { // cannot split
    removeOne(cur);
    cur->prev = NULL;
    cur->next = NULL;
    allocated += cur->size + sizeof(meta);
    return cur->address;
  } else { // cur->size >= size + sizeof(meta)
    meta *newBlock = (meta *)(cur->address + size);
    newBlock->next = cur->next;
    newBlock->prev = cur->prev;
    if (cur == head) {
      head = newBlock;
    } else {
      cur->prev->next = newBlock;
    }
    if (cur == tail) {
      tail = newBlock;
    } else {
      cur->next->prev = newBlock;
    }
    newBlock->address = (char *)(newBlock + 1);
    newBlock->size = cur->size - size - sizeof(meta);
    cur->size = size;
    cur->next = NULL;
    cur->prev = NULL;
    allocated += cur->size + sizeof(meta);
    return cur->address;
  }
}
void *ff_malloc(size_t size) {
  meta *cur = head;
  while (cur != NULL) {
    if (cur->size >= size) {
      break;
    }
    cur = cur->next;
  }
  if (cur == NULL) { // no appropriate space, need sbrk
    void *ptr = sbrk(size + sizeof(meta));
    total += size + sizeof(meta);
    meta *block = (meta *)ptr;
    // initialize the block meta
    block->prev = NULL;
    block->next = NULL;
    block->size = size;
    block->address = ptr + sizeof(meta);
    allocated += size + sizeof(meta);
    return block->address;
  } else { // cur->size >= size
    void *res = split(cur, size);
    // printf("After malloc: \n");
    // printFreeList();
    return res;
  }
}

void addAndTraverseFromHead(meta *block) {
  meta *cur = head;
  while (cur != NULL && cur->next != NULL &&
         (unsigned long)cur->next < (unsigned long)block) {
    cur = cur->next;
  }
  assert(cur->next != NULL);
  meta *tempNext = cur->next;
  tempNext->prev = block;
  cur->next = block;
  block->next = tempNext;
  block->prev = cur;
}

void addAndTraverseFromTail(meta *block) {
  meta *cur = tail;
  while (cur != NULL && cur->prev != NULL &&
         (unsigned long)cur->prev > (unsigned long)block) {
    cur = cur->prev;
  }
  assert(cur->prev != NULL);
  meta *tempPrev = cur->prev;
  tempPrev->next = block;
  cur->prev = block;
  block->next = cur;
  block->prev = tempPrev;
}
void add(meta *block) {
  if (head == NULL && tail == NULL) {
    head = block;
    tail = block;
  } else if ((unsigned long)head > (unsigned long)block) {
    head->prev = block;
    block->next = head;
    block->prev = NULL;
    head = block;
  } else if ((unsigned long)tail < (unsigned long)block) {
    tail->next = block;
    block->prev = tail;
    block->next = NULL;
    tail = block;
  } else {
    unsigned long distToHead = (unsigned long)block - (unsigned long)head;
    unsigned long distToTail = (unsigned long)tail - (unsigned long)block;
    if (distToHead < distToTail) {
      addAndTraverseFromHead(block);
    } else {
      addAndTraverseFromTail(block);
    }
    /*
    meta *cur = head;
    while (cur != NULL && cur->next != NULL &&
           (unsigned long)cur->next < (unsigned long)block) {
      cur = cur->next;
    }
    if (cur->next == NULL) {
      cur->next = block;
      block->next = NULL;
      block->prev = cur;
    } else { // cur->next > blcok && cur < blcok
      meta *tempNext = cur->next;
      tempNext->prev = block;
      cur->next = block;
      block->next = tempNext;
      block->prev = cur;
    }
    */
  }
}

// another merge method, time complexity O(n) and clean the tail, not used
void merge1() {
  meta *cur = head;
  while (cur != NULL && cur->next != NULL) {
    // printf("tttttttttttttt\n");
    if (cur->address + cur->size == (char *)cur->next) {
      // printf("is going to merge, cur->size: %lu, cur->next->size: %lu\n",
      // (unsigned long)cur->size, cur->next->size);
      cur->size += cur->next->size + sizeof(meta);
      cur->next = cur->next->next;
      if (cur->next != NULL) {
        cur->next->prev = cur;
        cur = cur->next;
        if (cur != NULL && cur->address + cur->size == (char *)cur->next) {
          cur->size += cur->next->size + sizeof(meta);
          cur->next = cur->next->next;
          if (cur->next != NULL) {
            cur->next->prev = cur;
          }
        }
      }
      break;
    } else {
      cur = cur->next;
    }
  }
  // cleanTail(cur);
}
void merge(meta *cur) {
  if (cur->prev != NULL) {
    meta *prevBlock = cur->prev;
    if (prevBlock->address + prevBlock->size == (char *)cur) {
      prevBlock->size += cur->size + sizeof(meta);
      prevBlock->next = cur->next;
      if (cur == tail) {
        tail = prevBlock;
      } else { // cur->next != NULL
        cur->next->prev = prevBlock;
      }
      cur = prevBlock;
    }
  }
  if (cur->next != NULL && cur->address + cur->size == (char *)cur->next) {
    cur->size += cur->next->size + sizeof(meta);
    if (cur->next == tail) {
      cur->next = NULL;
      tail = cur;
    } else {
      cur->next = cur->next->next;
      cur->next->prev = cur;
    }
  }
}
/*
// this method is going to clean the freed block at the end, not used
void cleanTail(meta *cur) {
  if (cur == NULL) {
    return;
  }
  void *programBreak = sbrk(0);
  if ((cur->address + cur->size) == (char *)programBreak) {
    total -= (cur->size + sizeof(meta));
    if (cur == head) {
      brk(cur);
      head = NULL;
    } else {
      meta *prevBlock = cur->prev;
      prevBlock->next = NULL;
      brk(cur);
    }
  }
}
*/
void ff_free(void *ptr) {
  meta *block = (meta *)((char *)ptr - sizeof(meta));
  allocated -= block->size + sizeof(meta);
  add(block);
  merge(block);
  // merge1();
  // printf("Free: \n");
  // printFreeList();
}

void *bf_malloc(size_t size) {
  meta *cur = head;
  size_t min = INT_MAX;
  meta *minPtr = NULL;
  while (cur != NULL) {
    if (cur->size >= size && cur->size < min) {
      min = cur->size;
      minPtr = cur;
      if (cur->size == size) {
        break;
      }
    }
    cur = cur->next;
  }
  cur = minPtr;
  if (cur == NULL) { // no appropriate space, need sbrk
    void *ptr = sbrk(size + sizeof(meta));
    // printf("new sbrk: %p\n", ptr);
    total += size + sizeof(meta);
    meta *block = (meta *)ptr;
    // initialize the block meta
    block->prev = NULL;
    block->next = NULL;
    block->size = size;
    block->address = ptr + sizeof(meta);
    allocated += size + sizeof(meta);
    return block->address;
  } else { // cur->size >= size
    void *res = split(cur, size);
    // printf("After malloc: \n");
    // printFreeList();
    return res;
  }
}

void bf_free(void *ptr) { ff_free(ptr); }

unsigned long get_data_segment_size() { return total; }
unsigned long get_data_segment_free_space_size() { return total - allocated; }
