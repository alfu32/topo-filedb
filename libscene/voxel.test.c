#include <stdio.h>
#include "voxel.h"

int main() {
    // Create a new voxel
    voxel_t* voxel = voxel__instance__new(5, 10, 'A');
    if (!voxel) {
        printf("Failed to create voxel\n");
        return 1;
    }
    printf("Created voxel: x = %d, y = %d, content = %c\n", voxel->x, voxel->y, voxel->content);

    printf("Created voxel hash %llu\n", voxel__instance__hash(voxel));

    // Create a deep copy of the voxel
    voxel_t* copy = voxel__instance__deep_copy(voxel);
    if (!copy) {
        printf("Failed to copy voxel\n");
        voxel__instance__free(voxel);
        return 1;
    }
    printf("Copied voxel: x = %d, y = %d, content = %c\n", copy->x, copy->y, copy->content);
    printf("Created voxel hash %llu\n", voxel__instance__hash(copy));

    // Free the voxel instances
    voxel__instance__free(voxel);
    voxel__instance__free(copy);

    return 0;
}
