#ifndef __rectangle_h__
#define __rectangle_h__


typedef struct {
    int x;
    int y;
    int w;
    int h;
} rectangle_s;

typedef rectangle_s rectangle_t;
int rectangle__instance__add(rectangle_t* self,int x,int y);

#endif