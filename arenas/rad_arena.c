#include "rad_arena.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#define AlignPow2(x, b)                                                        \
  (((x) + (b) - 1) &                                                           \
   ~((b) - 1)) // Align x up to the next multiple of b (power-of-two)

////////////////////////////////
//~ Default Settings

// Default sizes for memory reservation and commitment
uint64_t arena_default_reserve_size = MB(64); // Reserve 64 MB by default
uint64_t arena_default_commit_size = MB(4);   // Commit 4 MB initially
ArenaFlags arena_default_flags = 0;           // No special flags

////////////////////////////////
//~ Constants

#define ARENA_HEADER_SIZE                                                      \
  128 // Offset to start allocations after space for arena metadata

////////////////////////////////
//~ Core Arena Functions

// Internal allocator function that sets up an arena with given parameters
Arena *arena_alloc_(ArenaParams *params) {
  uint64_t reserve_size = params->reserve_size;
  uint64_t commit_size = params->commit_size;
  uint64_t page_size = sysconf(_SC_PAGESIZE); // Get system memory page size

  // Align sizes to page boundaries
  reserve_size = AlignPow2(reserve_size, page_size);
  commit_size = AlignPow2(commit_size, page_size);

  // Allocate backing memory if none was provided
  void *base = params->optional_backing_buffer;
  if (base == NULL) {
    // Reserve memory with no access permissions initially
    // PROT_NONE doesn't give read or access rights. Reserve large block of
    // virtual address space without allocating phisical memory (RAM)
    // with mprotect only the first commit_size (4mb), or more after we exceed
    // the 4mb, is made accessible
    // avoids committing all reserved size upfront, more efficient and less
    // change of page faults
    base =
        mmap(NULL, reserve_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED)
      return NULL;
    // Commit the initial region with read/write permissions
    mprotect(base, commit_size, PROT_READ | PROT_WRITE);
  }

  // Initialize arena metadata at the start of the memory block
  Arena *arena = (Arena *)base;
  arena->current = arena;         // Self-linked (used if arena chains exist)
  arena->flags = params->flags;   // Store flags
  arena->cmt_size = commit_size;  // Store commit size
  arena->res_size = reserve_size; // Store reserve size
  arena->base_pos = 0;            // Starting position offset
  arena->pos = ARENA_HEADER_SIZE; // Skip header space for allocations
  arena->cmt = commit_size;       // Current committed end
  arena->res = reserve_size;      // Total reserved size
  return arena;
}

// Macro wrapper for default arena allocation with optional overrides
#define arena_alloc(...)                                                       \
  arena_alloc_(&(ArenaParams){.reserve_size = arena_default_reserve_size,      \
                              .commit_size = arena_default_commit_size,        \
                              .flags = arena_default_flags,                    \
                              __VA_ARGS__})

// Releases the entire arena memory (and any chained arenas if present)
void arena_release(Arena *arena) {
  for (Arena *n = arena->current, *prev = NULL; n != NULL; n = prev) {
    prev = n->prev;
    munmap(n, n->res); // Free the memory region
  }
}

// Pushes a new block of memory onto the arena stack with the requested size and
// alignment
void *arena_push(Arena *arena, uint64_t size, uint64_t align) {
  Arena *current = arena->current;
  uint64_t pos_pre = AlignPow2(current->pos, align);
  uint64_t pos_pst = pos_pre + size;

  // If allocation exceeds reserved memory, chain a new arena
  if (pos_pst > current->res) {
    Arena *new_arena =
        arena_alloc_(&(ArenaParams){.reserve_size = current->res_size,
                                    .commit_size = current->cmt_size,
                                    .flags = current->flags});

    if (!new_arena)
      return NULL;

    new_arena->prev = current;
    arena->current = new_arena;
    current = new_arena;
    pos_pre = AlignPow2(current->pos, align);
    pos_pst = pos_pre + size;
  }

  // If allocation exceeds committed size, commit more memory
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

// Returns the absolute position in the arena (useful for saving state)
uint64_t arena_pos(Arena *arena) {
  Arena *current = arena->current;
  return current->base_pos + current->pos;
}

// Pops the arena back to a previously saved position
void arena_pop_to(Arena *arena, uint64_t pos) {
  Arena *current = arena->current;
  uint64_t rel_pos = pos - current->base_pos;
  if (rel_pos < current->pos) {
    current->pos = rel_pos;
  }
}

// Clears the entire arena (resets position to zero)
void arena_clear(Arena *arena) { arena_pop_to(arena, 0); }

// Pops a given number of bytes from the arena
void arena_pop(Arena *arena, uint64_t amount) {
  uint64_t new_pos = arena_pos(arena);
  new_pos = (amount < new_pos) ? (new_pos - amount) : 0;
  arena_pop_to(arena, new_pos);
}

// Begins a temporary memory scope
Temp temp_begin(Arena *arena) {
  Temp temp = {arena, arena_pos(arena)};
  return temp;
}

// Ends a temporary memory scope, rolling back all allocations made during the
// scope
void temp_end(Temp temp) { arena_pop_to(temp.arena, temp.pos); }
