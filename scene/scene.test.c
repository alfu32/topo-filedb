#include <stdio.h>
#include <stdbool.h>
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
    scene_slice_t* sliced_scene = scene__instance__slice(scene, &filter_voxels_with_content_a);
    scene__instance__for_each(sliced_scene, &print_voxel);

    // Cleanup
    scene_slice__instance__free(sliced_scene);
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

    char* b=scene__instance__to_string(scene);
    printf("Scene Buffer:\n%s",b);

    // Clean up
    free(b);
    scene__instance__free(scene);
    return 0;
}

int test_copy_and_neighbours() {
    scene_t* scene = scene__static__alloc();
    scene__instance__add_voxel_at(scene, 5, 5, 'A');
    scene__instance__add_voxel_at(scene, 5, 6, 'B');
    scene__instance__add_voxel_at(scene, 6, 5, 'C');
    scene__instance__add_voxel_at(scene, 6, 6, 'D');
    scene__instance__add_voxel_at(scene, 4, 4, 'E');

    // Find neighbors of voxel at (5, 5)
    scene_slice_t* neighbours = scene__instance__find_neighbours(scene, 5, 5);
    printf("Neighbours of (5, 5):\n");
    scene__instance__print(neighbours);

    // Deep copy of the scene
    scene_t* deep_copy = scene__instance__deep_copy(scene);
    printf("\nDeep Copy of Scene:\n");
    scene__instance__print(deep_copy);

    // Shallow copy of the scene
    scene_slice_t* shallow_copy = scene__instance__shallow_copy(scene);
    printf("\nShallow Copy of Scene:\n");
    scene__instance__print(shallow_copy);

    // Cleanup
    scene_slice__instance__free(neighbours);
    scene_slice__instance__free(shallow_copy);
    scene__instance__free(scene);
    scene__instance__free(deep_copy);

    return 0;
}

// Utility function to identify and print all islands in a scene
void identify_and_print_islands(scene_t* scene) {
    // Array to mark visited voxels
    //printf("// Array to mark visited voxels\n");
    //fflush(stdout);
    bool* visited = (bool*)calloc(*(scene->count), sizeof(bool));
    if (!visited) {
        printf("Memory allocation failed.\n");
        return;
    }

    size_t island_count = 0;

    // Iterate through all voxels in the scene
    for (size_t i = 0; i < *(scene->count); i++) {
        //printf("Testing voxel x:%d y:%d\n",scene->map[i]->x,scene->map[i]->y);
        //fflush(stdout);

        if (visited[i]) continue; // Skip already visited voxels

        voxel_t* voxel = scene->map[i];
        if (!voxel) continue;

        // Find the island starting at this voxel
        //printf("finding island at x:%d y:%d\n",scene->map[i]->x,scene->map[i]->y);
        //fflush(stdout);
        scene_slice_t* island = scene__instance__island_at(scene, voxel->x, voxel->y);

        if(!island) {
            //printf("no island at x:%d y:%d\n",scene->map[i]->x,scene->map[i]->y);
            //fflush(stdout);
            continue;
        }

        // Mark all voxels in the island as visited
        for (size_t j = 0; j < *(island->count); j++) {
            for (size_t k = 0; k < *(scene->count); k++) {
                if (scene->map[k] == island->map[j]) {
                    visited[k] = true;
                    break;
                }
            }
        }

        // Print the island
        printf("Island %zu:\n", ++island_count);
        scene__instance__print(island);
        printf("\n");

        // Free the island slice
        scene_slice__instance__free(island);
    }

    free(visited);
}

int test_identify_and_print_islands() {
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
    scene__instance__print(scene);
    printf("Scene contains %zu voxels:\n", *(scene->count));
    if (!scene || !scene->map || !scene->count || !scene->capacity) {
        printf("Error: Scene is not properly initialized.\n");
        return 1;
    }

    // Identify and print all islands
    printf("Identifying islands...\n");
    identify_and_print_islands(scene);

    // Cleanup
    scene__instance__free(scene);

    return 0;
}

int main(){
    printf("=== main_test_iterators =======================================:\n");
    main_test_iterators();
    printf("=== main_test_from_string =======================================:\n");
    main_test_from_string();
    printf("=== test_copy_and_neighbours =======================================:\n");
    test_copy_and_neighbours();
    printf("=== test_identify_and_print_islands =======================================:\n");
    test_identify_and_print_islands();
}
