#include <stdio.h>
#include "scene.h"

void print_voxel(scene_t* scene, voxel_t* voxel, int i) {
    printf("Voxel %d: x = %d, y = %d, content = %c\n", i, voxel->x, voxel->y, voxel->content);
}

voxel_t* duplicate_voxel(scene_t* scene, voxel_t* voxel, int i) {
    return voxel__instance__deep_copy(voxel);
}

int filter_voxels_with_content_a(scene_t* scene, voxel_t* voxel, int i) {
    return voxel->content == 'A';
}

int main_test_iterators() {
    scene_t* scene = scene__static__alloc();
    scene__instance__add_voxel_at(scene, 1, 1, 'A');
    scene__instance__add_voxel_at(scene, 2, 2, 'B');
    scene__instance__add_voxel_at(scene, 3, 3, 'A');

    // For Each
    printf("For Each:\n");
    scene__instance__for_each(scene, &print_voxel);

    // Map
    printf("\nMap:\n");
    scene_t* mapped_scene = scene__instance__map(scene, &duplicate_voxel);
    scene__instance__for_each(mapped_scene, &print_voxel);

    // Slice
    printf("\nSlice:\n");
    scene_t* sliced_scene = scene__instance__slice(scene, &filter_voxels_with_content_a);
    scene__instance__for_each(sliced_scene, &print_voxel);

    // Cleanup
    scene__instance__free_slice(sliced_scene);
    scene__instance__free(mapped_scene);
    scene__instance__free(scene);

    return 0;
}


int main_test_from_string() {
    // Define the map string
    const char* map = "\
                          OOO    \n\
                            O    \n\
    O                            \n\
                                 \n\
              O         OO       \n\
             OOO       OOOO      \n\
              OOOOOOOOOOOOOO     \n\
                OOOOOOOOOO       \n\
                   OOOO          \n\
     OO              OOO         \n\
    OOOO                         \n\
     OO                          \n\
                     OOOOO       \n\
                                 \n\
";

    // Create an empty scene and anchor voxel
    scene_t* scene = scene__static__alloc();
    voxel_t anchor = {0, 0, ' '}; // Anchor at (0,0)

    // Add voxels from the map string
    if (scene__instance__add_all_from_string(scene, map, &anchor) != 0) {
        printf("Failed to add voxels from string.\n");
        scene__instance__free(scene);
        return 1;
    }

    // Print the resulting scene
    printf("Scene:\n");
    scene__instance__print(scene);

    // Clean up
    scene__instance__free(scene);
    return 0;
}

int main(){
    main_test_iterators();
    main_test_from_string();
}
