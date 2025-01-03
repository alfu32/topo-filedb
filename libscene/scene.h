#ifndef __scene_h__
#define __scene_h__
#include <stdlib.h>
#include "rectangle.h"
#include "voxel.h"

typedef struct {
    voxel_t** map;
    size_t* count;
    size_t* capacity;
} scene_s;

typedef scene_s scene_t;
typedef scene_s scene_slice_t;

typedef void (*voxel_for_each_fn)(scene_t* scene, voxel_t* voxel,int i);
typedef voxel_t* (*voxel_map_fn)(scene_t* scene, voxel_t* voxel,int i);
typedef int (*voxel_filter_fn)(scene_t* scene, voxel_t* voxel,int i);

// scene_t methods prototypes
scene_t* scene__static__alloc();
int scene__instance__add_all_from_string(scene_t* self, const char* definition,voxel_t* anchor);
// adds the voxel pointer as is, without copying or null check
int scene__instance__add_voxel(scene_t* self, voxel_t* voxel);
// creates and adds a voxel to the scene
int scene__instance__add_voxel_at(scene_t* self, int x, int y,char content);
// returns the removed voxel
voxel_t* scene__instance__remove_voxel_at(scene_t* self, int x, int y);

int scene__instance__remove_voxel(scene_t* self, voxel_t* voxel);
// iterates through all voxels in the scene 
int scene__instance__for_each(scene_t* self, voxel_for_each_fn voxel);
// returns a new scene containing the mapped voxels
scene_t* scene__instance__map(scene_t* self, voxel_map_fn voxel);
// returns a new scene containing a shallow copy of the filtered voxels
scene_slice_t* scene__instance__slice(scene_t* self, voxel_filter_fn voxel);
// returns the reference of the voxel found at x,y or null if not found
voxel_t* scene__instance__find_voxel_at(scene_t* self, int x, int y);
/** 
 * Find the scene  index of voxel
 * returns -1 if not found
 */ 
int scene__instance__index_of(scene_t* self, voxel_t* voxel);
/**
 * collects all the voxels neighbouring the given coordinates except for the voxel at the given coordinates
 * the voxel at the given point is assumed to exist
 * returns a new slice containig the collected voxels
**/
scene_slice_t* scene__instance__find_neighbours(scene_t* self, int x, int y);
// copies the scene and voxels into a new scene and returns it
scene_t* scene__instance__deep_copy(scene_t* self);
// copies the scene and the voxel pointers, returns a new slice basically
scene_slice_t* scene__instance__shallow_copy(scene_t* self);
// given the start point x,y finds all the connected voxels ( that neighbour each other ) and returns them as a shallow copy
scene_slice_t* scene__instance__island_at(scene_t* self, int x, int y);

// basically calculates the minimum and maximum x and y coordinates of the
rectangle_t* scene__instance__bounding_rectangle(scene_t* self);
int scene__instance__clear(scene_t* self);
/**
 * prints the entire scene to the screen/terminal as a rectangular buffer
 * empty spaces will be printed as empty chars.
 * it determines the bounding rectangle first
 */
char* scene__instance__to_string(scene_t* self);
void scene__instance__print(scene_t* self);
void scene_slice__instance__free(scene_slice_t* self);
void scene__instance__free(scene_t* self);

#endif