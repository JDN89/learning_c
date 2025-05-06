#include "rad_arena.h"
#include <inttypes.h>
#include <stdalign.h>
#include <stdio.h>
#include <string.h>

int main() {
  // Allocate arena
  Arena *arena = arena_alloc();
  if (!arena) {
    fprintf(stderr, "Failed to allocate arena\n");
    return 1;
  }

  // Push some memory into arena
  char *message = (char *)arena_push(arena, 32, 8);
  strcpy(message, "Hello, arena allocator!");
  printf("Message: %s\n", message);

  // Start a temporary memory block
  Temp temp = temp_begin(arena);

  // Push temporary memory
  int *temp_array = (int *)arena_push(arena, sizeof(int) * 5, alignof(int));
  for (int i = 0; i < 5; ++i) {
    temp_array[i] = i * 10;
    printf("temp_array[%d] = %d\n", i, temp_array[i]);
  }

  // End temporary memory block (roll back)
  temp_end(temp);

  // temp_array is now invalid; we roll back to before it was allocated
  printf("After temp_end, arena pos: %" PRIu64 "\n", arena_pos(arena));

  // Clear all arena memory
  arena_clear(arena);
  printf("After arena_clear, arena pos: %" PRIu64 "\n", arena_pos(arena));

  // Release memory
  arena_release(arena);

  return 0;

  return 0;
}
