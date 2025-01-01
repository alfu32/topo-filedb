#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"
#include "scene/scene.h"

const char* map="\
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

int main(int argc, const char** argv) {

    // print_args(argc,argv);
    scene_t* m = scene__static__alloc();
    voxel_t anchor={0,0,'a'};
    
    scene__instance__add_all_from_string(m,map,&anchor);

    scene__instance__print(m);

    scene__instance__free(m);
    return 0;
}