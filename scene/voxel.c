#include <stdlib.h>
#include "voxel.h"

// Create and initialize a new voxel instance
voxel_t* voxel__instance__new(int x, int y, char content) {
    // Allocate memory for the new voxel
    voxel_t* voxel = (voxel_t*)malloc(sizeof(voxel_t));
    if (!voxel) {
        return NULL; // Return NULL if memory allocation fails
    }

    // Initialize voxel properties
    voxel->x = x;
    voxel->y = y;
    voxel->content = content;

    return voxel; // Return the pointer to the new voxel
}

unsigned long long voxel__instance__hash(voxel_t* self) {
    if (!self) return 0; // Return 0 for null input

    unsigned long long hash = 0;
    hash |= ((unsigned long long)(unsigned int)self->y) << 32; // Ensure `self->y` fits in 32 bits
    hash |= (unsigned long long)(unsigned int)self->x;         // Ensure `self->x` fits in 32 bits

    return hash;
}

// Create and return a deep copy of the voxel
voxel_t* voxel__instance__deep_copy(voxel_t* self) {
    if (!self) {
        return NULL; // Return NULL if the input voxel is NULL
    }

    // Allocate memory for the copy
    voxel_t* copy = (voxel_t*)malloc(sizeof(voxel_t));
    if (!copy) {
        return NULL; // Return NULL if memory allocation fails
    }

    // Copy properties from the source voxel
    copy->x = self->x;
    copy->y = self->y;
    copy->content = self->content;

    return copy; // Return the pointer to the copied voxel
}

// Free the memory of a voxel instance
int voxel__instance__free(voxel_t* self) {
    if (!self) {
        return -1; // Return error code if the input voxel is NULL
    }

    free(self); // Free the allocated memory
    return 0;   // Return success code
}
