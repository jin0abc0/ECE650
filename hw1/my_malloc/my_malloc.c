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
void cleanTail(meta *cur);
void printFreeList() {
  meta *cur = head;
  while (cur != NULL) {
    printf("cur: %p, cur->size: %lu, cur->address: %p\n", cur, cur->size,
           cur->address);
    cur = cur->next;
  }
}

void removeOne(meta *cur) {
  if (cur == head) {
    if (cur->next != NULL) {
      cur->next->prev = NULL;
    }
    head = cur->next;
  } else {
    meta *tempPrev = cur->prev;
    meta *tempNext = cur->next;
    if (tempPrev != NULL) {
      tempPrev->next = cur->next;
    }
    if (tempNext != NULL) {
      tempNext->prev = cur->prev;
    }
  }
}
void *split(meta *cur, size_t size) {
  assert(cur->size >= size);
  if (cur->size < size + sizeof(meta)) { // cannot split
    // printf("cannot split, cur size: %lu, cur->size: %lu\n", (unsigned
    // long)size,
    // (unsigned long)cur->size);

    removeOne(cur);
    cur->prev = NULL;
    cur->next = NULL;
    allocated += cur->size;
    return cur->address;
  } else { // cur->size >= size + sizeof(meta)
    /*
    meta *newBlock = (meta *)(cur->address + (cur->size - sizeof(meta) - size));
    printf("ok, cur size: %lu, cur->size: %lu\n", (unsigned long)size,
           (unsigned long)cur->size);

    newBlock->prev = NULL;
    newBlock->next = NULL;
    newBlock->size = size;
    newBlock->address =
        cur->address + (cur->size - sizeof(meta) - size) + sizeof(meta);
    cur->size = cur->size - sizeof(meta) - size;
    printf("split size: %lu, split return address: %p, cur->size: %lu\n",
           (unsigned long)newBlock->size, newBlock->address,
           (unsigned long)cur->size);
    return newBlock->address;
    */
    meta *newBlock = (meta *)(cur->address + size);
    newBlock->next = cur->next;
    newBlock->prev = cur->prev;
    if (cur == head) {
      head = newBlock;
    } else {
      cur->prev->next = newBlock;
    }
    if (cur->next != NULL) {
      cur->next->prev = newBlock;
    }
    newBlock->address = (char *)(newBlock + 1);
    newBlock->size = cur->size - size - sizeof(meta);
    cur->size = size;
    cur->next = NULL;
    cur->prev = NULL;
    return cur->address;
    /*
    newBlock->prev = cur->prev;
    newBlock->next = cur->next;
    if (cur->prev != NULL) {
      cur->prev->next = newBlock;
    }
    if (cur->next != NULL) {
      cur->next->prev = newBlock;
    }
    newBlock->size = cur->size - size - sizeof(meta);
    newBlock->address = cur->address + size + sizeof(meta);
    cur->prev = NULL;
    cur->next = NULL;
    cur->size = size;
    allocated += cur->size;
    return cur->address;
    */
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
    // printf("new sbrk: %p\n", ptr);
    total += size + sizeof(meta);
    meta *block = (meta *)ptr;
    // initialize the block meta
    block->prev = NULL;
    block->next = NULL;
    block->size = size;
    block->address = ptr + sizeof(meta);
    allocated += size;
    /*
    printf("cur: %p, cur address: %p, cur end: %p\n", block, block->address,
           block->address + block->size);
    */
    return block->address;
  } else { // cur->size >= size
    /*
    void *res = split(cur, size);
    meta *temp = (meta *)((char *)res - sizeof(meta));
    printf("After malloc: \n");
    printFreeList();
    printf("res: %p, temp: %p, return address: %p, return size: %lu\n", res,
           temp, temp->address, temp->size);
    return res;
    */
    void *res = split(cur, size);
    // printf("After malloc: \n");
    // printFreeList();
    return res;
  }
}
void add(meta *block) {
  // printf("Before add: cur->address: %p, cur->size: %lu\n", block->address,
  //     (unsigned long)block->size);
  if (head == NULL || (unsigned long)head > (unsigned long)block) {
    // printf("llllllllllllllllllll\n");
    if (head != NULL) {
      head->prev = block;
    }
    block->next = head;
    block->prev = NULL;
    head = block;
  } else {
    meta *cur = head;
    while (cur != NULL && cur->next != NULL &&
           (unsigned long)cur->next < (unsigned long)block) {
      // printf("sssssssssssss\n");
      cur = cur->next;
    }
    // printf("oooooooooooooo\n");
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
      /*
      block->next = cur->next;
      cur->next->prev = block;
      cur->next = block;
      block->prev = cur;
      */
    }
  }
  // printf("After add: cur->address: %p, cur->size: %lu\n", block->address,
  //     (unsigned long)block->size);
}
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
      if (cur->next != NULL) {
        cur->next->prev = prevBlock;
      }
      cur = prevBlock;
    }
  }
  if (cur->next != NULL && cur->address + cur->size == (char *)cur->next) {
    cur->size += cur->next->size + sizeof(meta);
    cur->next = cur->next->next;
    if (cur->next != NULL) {
      cur->next->prev = cur;
    }
  }
}

void cleanTail(meta *cur) {
  if (cur == NULL) {
    return;
  }
  void *programBreak = sbrk(0);
  /*
  printf("programBreak: %p, cur + size + sizeof(meta): %p, cur size: %lu, cur
  " "pointer: %p, cur address + cur size pointer: %p\n", programBreak, (cur +
  cur->size + sizeof(meta)), (unsigned long)cur->size, cur, cur->address +
  cur->size);
  */
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

void ff_free(void *ptr) {
  // printf("Before free: \n");
  // printFreeList();
  // printf("is going to free: %p\n", ptr);
  meta *block = (meta *)((char *)ptr - sizeof(meta));
  // printf("free middle: block: %p, cur->address: %p, cur->size: %lu\n", block,
  //     block->address, (unsigned long)block->size);
  allocated -= block->size;
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
    if (cur->size >= size) {
      if (cur->size < min) {
        min = cur->size;
        minPtr = cur;
      }
    }
    cur = cur->next;
  }
  if (minPtr == NULL) { // no appropriate space, need sbrk
    void *ptr = sbrk(size + sizeof(meta));
    total += size + sizeof(meta);
    meta *block = (meta *)ptr;
    // initialize the block meta
    block->prev = NULL;
    block->next = NULL;
    block->size = size;
    block->address = ptr + sizeof(meta);
    allocated += size;
    return block->address;
  } else {
    void *res = split(minPtr, size);
    return res;
  }
}

void bf_free(void *ptr) { ff_free(ptr); }

unsigned long get_data_segment_size() { return total; }
unsigned long get_data_segment_free_space_size() { return total - allocated; }
