#include "rectangle.h"

// Implementation of rectangle__instance__add
int rectangle__instance__add(rectangle_t* self, int x, int y) {
    if (!self) {
        return -1; // Return error code if the pointer is NULL
    }

    // Add the offsets to the rectangle's coordinates
    self->x += x;
    self->y += y;

    return 0; // Success
}
