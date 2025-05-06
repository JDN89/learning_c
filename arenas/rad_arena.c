#include "rad_arena.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#define AlignPow2(x, b) (((x) + (b) - 1) & ~((b) - 1))

uint64_t arena_default_reserve_size = MB(64); // Or whatever default makes sense
uint64_t arena_default_commit_size = MB(4);   // Adjust based on your needs
ArenaFlags arena_default_flags = 0;           // e.g., no flags by default

////////////////////////////////
//~ Constants
#define ARENA_HEADER_SIZE 128

////////////////////////////////
//~ Core Arena Functions

Arena *arena_alloc_(ArenaParams *params) {
  uint64_t reserve_size = params->reserve_size;
  uint64_t commit_size = params->commit_size;
  uint64_t page_size = sysconf(_SC_PAGESIZE);

  reserve_size = AlignPow2(reserve_size, page_size);
  commit_size = AlignPow2(commit_size, page_size);

  void *base = params->optional_backing_buffer;
  if (base == NULL) {
    base =
        mmap(NULL, reserve_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED)
      return NULL;
    mprotect(base, commit_size, PROT_READ | PROT_WRITE);
  }

  Arena *arena = (Arena *)base;
  arena->current = arena;
  arena->flags = params->flags;
  arena->cmt_size = commit_size;
  arena->res_size = reserve_size;
  arena->base_pos = 0;
  arena->pos = ARENA_HEADER_SIZE;
  arena->cmt = commit_size;
  arena->res = reserve_size;
  return arena;
}

#define arena_alloc(...)                                                       \
  arena_alloc_(&(ArenaParams){.reserve_size = arena_default_reserve_size,      \
                              .commit_size = arena_default_commit_size,        \
                              .flags = arena_default_flags,                    \
                              __VA_ARGS__})

void arena_release(Arena *arena) {
  for (Arena *n = arena->current, *prev = NULL; n != NULL; n = prev) {
    prev = n->prev;
    munmap(n, n->res);
  }
}

void *arena_push(Arena *arena, uint64_t size, uint64_t align) {
  Arena *current = arena->current;
  uint64_t pos_pre = AlignPow2(current->pos, align);
  uint64_t pos_pst = pos_pre + size;

  if (pos_pst > current->cmt) {
    uint64_t commit_end = AlignPow2(pos_pst, current->cmt_size);
    uint64_t commit_diff = commit_end - current->cmt;
    mprotect((uint8_t *)current + current->cmt, commit_diff,
             PROT_READ | PROT_WRITE);
    current->cmt = commit_end;
  }

  void *result = (uint8_t *)current + pos_pre;
  current->pos = pos_pst;
  return result;
}

uint64_t arena_pos(Arena *arena) {
  Arena *current = arena->current;
  return current->base_pos + current->pos;
}

void arena_pop_to(Arena *arena, uint64_t pos) {
  Arena *current = arena->current;
  uint64_t rel_pos = pos - current->base_pos;
  if (rel_pos < current->pos) {
    current->pos = rel_pos;
  }
}

void arena_clear(Arena *arena) { arena_pop_to(arena, 0); }

void arena_pop(Arena *arena, uint64_t amount) {
  uint64_t new_pos = arena_pos(arena);
  new_pos = (amount < new_pos) ? (new_pos - amount) : 0;
  arena_pop_to(arena, new_pos);
}

Temp temp_begin(Arena *arena) {
  Temp temp = {arena, arena_pos(arena)};
  return temp;
}

void temp_end(Temp temp) { arena_pop_to(temp.arena, temp.pos); }
