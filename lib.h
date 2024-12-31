#ifndef __lib_h__
#define __lib_h__
#include <stdio.h>


void print_args(int argc, const char** argv) {
    for(int i=0;i<argc;i++){
        printf("args[%d] = %s\n",i,argv[i]);
    }
}
#endif