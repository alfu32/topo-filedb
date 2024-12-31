#ifndef __voxel_h__
#define __voxel_h__


typedef struct {
    int x,y;
    char content;
} voxel_s;

typedef voxel_s voxel_t;

voxel_t* voxel__instance__new(int x,int y,char content);
// creates and returns a new copy of self
voxel_t* voxel__instance__deep_copy(voxel_t* self);
int voxel__instance__free(voxel_t* self);

#endif