#include <stdio.h>
#include "rectangle.h"

int main() {
    rectangle_t rect = {10, 20, 30, 40};

    printf("Before: x = %d, y = %d\n", rect.x, rect.y);

    if (rectangle__instance__add(&rect, 5, -10) == 0) {
        printf("After: x = %d, y = %d\n", rect.x, rect.y);
    } else {
        printf("Error: Invalid rectangle pointer\n");
    }

    return 0;
}
