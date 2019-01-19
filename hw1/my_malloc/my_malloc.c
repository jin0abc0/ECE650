#include "my_malloc.h"
#include <assert.h>
#include <limits.h>
#include <unistd.h>
typedef struct meta_t {
  struct meta_t *prev;
  struct meta_t *next;
  size_t size;
  void *address;
} meta;

static meta *head = NULL; // freed address linked list
void *split(meta *cur, size_t size) {
  assert(cur->size >= size);
  if (cur->size >= size && cur->size < size + sizeof(meta)) { // cannot split
    cur->prev->next = cur->next;
    cur->next->prev = cur->prev;
    cur->prev = NULL;
    cur->next = NULL;
    return cur->address;
  } else {
    meta *newBlock = (meta *)(cur->address + size);
    newBlock->prev = cur->prev;
    newBlock->next = cur->next;
    cur->prev->next = newBlock;
    cur->next->prev = newBlock;
    newBlock->size = cur->size - size - sizeof(meta);
    newBlock->address = cur->address + size + sizeof(meta);
    cur->prev = NULL;
    cur->next = NULL;
    cur->size = size;
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
    meta *block = (meta *)ptr;
    // initialize the block meta
    block->prev = NULL;
    block->next = NULL;
    block->size = size;
    block->address = ptr + sizeof(meta);
    return block->address;
  } else { // cur->size >= size
    void *res = split(cur, size);
    return res;
  }
}
void add(meta *block) {
  if (head == NULL || head > block) {
    block->next = head == NULL ? NULL : head->next;
    block->prev = NULL;
    head = block;
  } else {
    meta *cur = head;
    while (cur != NULL && cur->next != NULL && cur->next < block) {
      cur = cur->next;
    }
    if (cur->next == NULL) {
      cur->next = block;
      block->next = NULL;
      block->prev = cur;
    } else { // cur->next > blcok && cur < blcok
      block->next = cur->next;
      cur->next = block;
      block->prev = cur;
    }
  }
}
void merge() {
  meta *cur = head;
  while (cur != NULL && cur->next != NULL) {
    if (cur->address + cur->size == cur->next) {
      cur->size += cur->next->size + sizeof(meta);
      cur->next = cur->next->next;
      cur->next->next->prev = cur;
    } else {
      cur = cur->next;
    }
  }
}

void cleanTail() {
  if (head == NULL) {
    return;
  }
  meta *cur = head;
  void *programBreak = sbrk(0);
  while (cur != NULL && cur->next != NULL) {
    cur = cur->next;
  }
  if (cur + cur->size + sizeof(meta) == programBreak) {
    if (cur == head) {
      brk(cur);
      head = NULL;
    } else {
      meta *prevBlock = cur->prev;
      prevBlock->next = NULL;
      brk(prevBlock->size + prevBlock->address);
    }
  }
}

void ff_free(void *ptr) {
  meta *block = (meta *)(ptr - sizeof(meta));
  add(block);
  merge();
  cleanTail();
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
    meta *block = (meta *)ptr;
    // initialize the block meta
    block->prev = NULL;
    block->next = NULL;
    block->size = size;
    block->address = ptr + sizeof(meta);
    return block->address;
  } else {
    void *res = split(minPtr, size);
    return res;
  }
}

void bf_free(void *ptr) { ff_free(ptr); }
