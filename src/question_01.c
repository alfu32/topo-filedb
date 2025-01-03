#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"

int balance_score(const char* str){
    int l= strlen(str);
    int score = 0;
    for(int i=0,j=l-1;i<l;i++,j--){
        char chr0=str[i];
        switch(chr0){
            case '(':score++;break;
            case ')':score--;break;
        }
    }
    return score;
}

int main(int argc, const char** argv) {

    // print_args(argc,argv);
    int score = balance_score(argv[1]);
    printf("%d",score);
    return score;
}