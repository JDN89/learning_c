#include "rad_arena.h"
#include <inttypes.h>
#include <stdalign.h>
#include <stdio.h>
#include <string.h>

// Define the structure of a node
struct Node {
  int data;
  struct Node *left;
  struct Node *right;
};

// Function to create a new node using the arena
struct Node *createNode(Arena *arena, int value) {
  struct Node *newNode = (struct Node *)arena_push(arena, sizeof(struct Node),
                                                   alignof(struct Node));
  newNode->data = value;
  newNode->left = NULL;
  newNode->right = NULL;
  return newNode;
}

// Insert a node in a binary search tree using arena
struct Node *insert(Arena *arena, struct Node *root, int value) {
  if (root == NULL) {
    return createNode(arena, value);
  }
  if (value < root->data) {
    root->left = insert(arena, root->left, value);
  } else {
    root->right = insert(arena, root->right, value);
  }
  return root;
}

// Inorder traversal (left, root, right)
void inorder(struct Node *root) {
  if (root != NULL) {
    inorder(root->left);
    printf("%d ", root->data);
    inorder(root->right);
  }
}

int main() {
  // Allocate the arena
  Arena *arena = arena_alloc();

  // Use arena to build the tree
  struct Node *root = NULL;
  root = insert(arena, root, 50);
  insert(arena, root, 30);
  insert(arena, root, 70);
  insert(arena, root, 20);
  insert(arena, root, 40);
  insert(arena, root, 60);
  insert(arena, root, 80);

  // Display the tree
  printf("Inorder traversal: ");
  inorder(root);
  printf("\n");

  // Release arena memory
  arena_release(arena);

  return 0;
}

// int main() {
//   // Allocate arena
//   Arena *arena = arena_alloc();
//   if (!arena) {
//     fprintf(stderr, "Failed to allocate arena\n");
//     return 1;
//   }
//
//   // Push some memory into arena
//   char *message = (char *)arena_push(arena, 32, 8);
//   strcpy(message, "Hello, arena allocator!");
//   printf("Message: %s\n", message);
//
//   // Start a temporary memory block
//   Temp temp = temp_begin(arena);
//
//   // Push temporary memory
//   int *temp_array = (int *)arena_push(arena, sizeof(int) * 5, alignof(int));
//   for (int i = 0; i < 5; ++i) {
//     temp_array[i] = i * 10;
//     printf("temp_array[%d] = %d\n", i, temp_array[i]);
//   }
//
//   // End temporary memory block (roll back)
//   temp_end(temp);
//
//   // temp_array is now invalid; we roll back to before it was allocated
//   printf("After temp_end, arena pos: %" PRIu64 "\n", arena_pos(arena));
//
//   // Clear all arena memory
//   arena_clear(arena);
//   printf("After arena_clear, arena pos: %" PRIu64 "\n", arena_pos(arena));
//
//   // Release memory
//   arena_release(arena);
//
//   return 0;
//
//   return 0;
// }
