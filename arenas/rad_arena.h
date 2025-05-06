// arena_rad_style.h
#ifndef RAD_ARENA_H
#define RAD_ARENA_H

#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

// Macros
#define KB(x) ((x) * 1024ULL)
#define MB(x) ((x) * 1024ULL * 1024ULL)
#define AlignPow2(x, b) (((x) + (b) - 1) & ~((b) - 1))

#define arena_alloc(...)                                                       \
  arena_alloc_(&(ArenaParams){.reserve_size = arena_default_reserve_size,      \
                              .commit_size = arena_default_commit_size,        \
                              .flags = arena_default_flags,                    \
                              __VA_ARGS__})
// Constants
#define ARENA_HEADER_SIZE 128

// Types
typedef uint64_t ArenaFlags;
enum {
  ArenaFlag_NoChain = (1 << 0),
  ArenaFlag_LargePages = (1 << 1),
};

typedef struct ArenaParams ArenaParams;
struct ArenaParams {
  ArenaFlags flags;
  uint64_t reserve_size;
  uint64_t commit_size;
  void *optional_backing_buffer;
};

typedef struct Arena Arena;
struct Arena {
  Arena *prev;
  Arena *current;
  ArenaFlags flags;
  uint64_t cmt_size;
  uint64_t res_size;
  uint64_t base_pos;
  uint64_t pos;
  uint64_t cmt;
  uint64_t res;
};

typedef struct Temp Temp;
struct Temp {
  Arena *arena;
  uint64_t pos;
};

// Global variables
extern uint64_t arena_default_reserve_size;
extern uint64_t arena_default_commit_size;
extern ArenaFlags arena_default_flags;

// Function declarations
Arena *arena_alloc_(ArenaParams *params);
void arena_release(Arena *arena);
void *arena_push(Arena *arena, uint64_t size, uint64_t align);
uint64_t arena_pos(Arena *arena);
void arena_pop_to(Arena *arena, uint64_t pos);
void arena_clear(Arena *arena);
void arena_pop(Arena *arena, uint64_t amount);
Temp temp_begin(Arena *arena);
void temp_end(Temp temp);

#endif // RAD_ARENA_H
