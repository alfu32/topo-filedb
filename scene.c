#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scene.h"

// Helper macro for memory reallocation
#define SCENE_INITIAL_CAPACITY 10
#define SCENE_RESIZE_FACTOR 2

// Allocate a new scene instance
scene_t* scene__static__alloc() {
    scene_t* scene = (scene_t*)malloc(sizeof(scene_t));
    if (!scene) return NULL;

    scene->map = (voxel_t**)malloc(SCENE_INITIAL_CAPACITY * sizeof(voxel_t*));
    if (!scene->map) {
        free(scene);
        return NULL;
    }

    scene->count = (size_t*)malloc(sizeof(size_t));
    scene->capacity = (size_t*)malloc(sizeof(size_t));
    if (!scene->count || !scene->capacity) {
        free(scene->map);
        free(scene->count);
        free(scene->capacity);
        free(scene);
        return NULL;
    }

    *(scene->count) = 0;
    *(scene->capacity) = SCENE_INITIAL_CAPACITY;

    return scene;
}

// Add all voxels from the string definition relative to an anchor
int scene__instance__add_all_from_string(scene_t* self, const char* definition, voxel_t* anchor) {
    if (!self || !definition || !anchor) return -1;

    int anchor_x = anchor->x;
    int anchor_y = anchor->y;

    int x = 0, y = 0; // Coordinates for current character

    for (const char* p = definition; *p != '\0'; ++p) {
        if (*p == '\n') {
            // Newline means move to the next row
            x = 0;
            y++;
        } else {
            if (*p != ' ') {
                // Non-space character: create and add a voxel
                if (scene__instance__add_voxel_at(self, anchor_x + x, anchor_y + y, *p) != 0) {
                    return -1; // Return error if adding fails
                }
            }
            x++; // Move to the next column
        }
    }

    return 0; // Success
}

// Add a voxel to the scene without copying or checking
int scene__instance__add_voxel(scene_t* self, voxel_t* voxel) {
    if (!self || !voxel) return -1;

    // Resize if needed
    if (*(self->count) >= *(self->capacity)) {
        *(self->capacity) *= SCENE_RESIZE_FACTOR;
        voxel_t** new_map = (voxel_t**)realloc(self->map, *(self->capacity) * sizeof(voxel_t*));
        if (!new_map) return -1;
        self->map = new_map;
    }

    self->map[*(self->count)] = voxel;
    (*(self->count))++;

    return 0;
}

// Add a voxel by creating a new one
int scene__instance__add_voxel_at(scene_t* self, int x, int y, char content) {
    voxel_t* voxel = voxel__instance__new(x, y, content);
    if (!voxel) return -1;

    return scene__instance__add_voxel(self, voxel);
}

int scene__instance__remove_voxel(scene_t* self, voxel_t* voxel){
    voxel_t* r = scene__instance__remove_voxel_at(self, voxel->x, voxel->y);
    return r?1:-1;
}

// Remove a voxel at specified coordinates
voxel_t* scene__instance__remove_voxel_at(scene_t* self, int x, int y) {
    if (!self || !self->map || *(self->count) == 0) return NULL;

    for (size_t i = 0; i < *(self->count); i++) {
        if (self->map[i]->x == x && self->map[i]->y == y) {
            voxel_t* removed = self->map[i];

            // Shift remaining elements
            for (size_t j = i; j < *(self->count) - 1; j++) {
                self->map[j] = self->map[j + 1];
            }

            (*(self->count))--;
            return removed;
        }
    }
    return NULL; // No voxel found at the specified coordinates
}

// Find a voxel at specified coordinates
voxel_t* scene__instance__find_voxel_at(scene_t* self, int x, int y) {
    if (!self || *(self->count) == 0) return NULL;

    for (size_t i = 0; i < *(self->count); i++) {
        if (self->map[i]->x == x && self->map[i]->y == y) {
            return self->map[i];
        }
    }
    return NULL;
}

// Iterate through all voxels in the scene and apply the given function
int scene__instance__for_each(scene_t* self, voxel_for_each_fn fn) {
    if (!self || !fn || *(self->count) == 0) return -1;

    for (size_t i = 0; i < *(self->count); i++) {
        fn(self, self->map[i], (int)i);
    }

    return 0; // Success
}

// Return a new scene containing the mapped voxels
scene_t* scene__instance__map(scene_t* self, voxel_map_fn map_fn) {
    if (!self || !map_fn) return NULL;

    scene_t* new_scene = scene__static__alloc();
    if (!new_scene) return NULL;

    for (size_t i = 0; i < *(self->count); i++) {
        voxel_t* mapped_voxel = map_fn(self, self->map[i], (int)i);
        if (mapped_voxel) {
            scene__instance__add_voxel(new_scene, mapped_voxel);
        }
    }

    return new_scene;
}

// Return a new scene containing a shallow copy of the filtered voxels
scene_t* scene__instance__slice(scene_t* self, voxel_filter_fn filter_fn) {
    if (!self || !filter_fn) return NULL;

    scene_t* new_scene = scene__static__alloc();
    if (!new_scene) return NULL;

    for (size_t i = 0; i < *(self->count); i++) {
        if ((*filter_fn)(self, self->map[i], (int)i)) {
            scene__instance__add_voxel(new_scene, self->map[i]);
        }
    }

    return new_scene;
}

// Clear the scene
int scene__instance__clear(scene_t* self) {
    if (!self) return -1;

    for (size_t i = 0; i < *(self->count); i++) {
        voxel__instance__free(self->map[i]);
    }

    *(self->count) = 0;
    return 0;
}

// Print the scene as a grid
void scene__instance__print(scene_t* self) {
    if (!self || *(self->count) == 0) {
        printf("Scene is empty.\n");
        return;
    }

    rectangle_t* bounds = scene__instance__bounding_rectangle(self);
    if (!bounds) {
        printf("Failed to determine scene bounds.\n");
        return;
    }

    for (int y = bounds->y; y < bounds->y + bounds->h; y++) {
        for (int x = bounds->x; x < bounds->x + bounds->w; x++) {
            voxel_t* voxel = scene__instance__find_voxel_at(self, x, y);
            printf("%c", voxel ? voxel->content : ' ');
        }
        printf("\n");
    }

    free(bounds);
}

// Calculate the bounding rectangle
rectangle_t* scene__instance__bounding_rectangle(scene_t* self) {
    if (!self || *(self->count) == 0) return NULL;

    int min_x = self->map[0]->x, max_x = self->map[0]->x;
    int min_y = self->map[0]->y, max_y = self->map[0]->y;

    for (size_t i = 1; i < *(self->count); i++) {
        voxel_t* voxel = self->map[i];
        if (voxel->x < min_x) min_x = voxel->x;
        if (voxel->x > max_x) max_x = voxel->x;
        if (voxel->y < min_y) min_y = voxel->y;
        if (voxel->y > max_y) max_y = voxel->y;
    }

    rectangle_t* rect = (rectangle_t*)malloc(sizeof(rectangle_t));
    if (!rect) return NULL;

    rect->x = min_x;
    rect->y = min_y;
    rect->w = max_x - min_x + 1;
    rect->h = max_y - min_y + 1;

    return rect;
}

// Free the scene and its contents
void scene__instance__free_slice(scene_t* self) {
    if (!self) return;

    free(self->map);
    free(self->count);
    free(self->capacity);
    free(self);
}

// Free the scene and its contents
void scene__instance__free(scene_t* self) {
    if (!self) return;

    scene__instance__clear(self);

    free(self->map);
    free(self->count);
    free(self->capacity);
    free(self);
}
