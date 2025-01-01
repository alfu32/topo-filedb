#!/bin/bash
rm -rf bin

mkdir -p bin/o

zig cc question_01.c -o bin/question_01
zig cc scene/rectangle.c scene/voxel.c scene/scene.c question_02.c -o bin/question_02


zig cc -c -fPIC scene/rectangle.c -o bin/o/rectangle.o
zig cc -c -fPIC scene/voxel.c -o bin/o/voxel.o
zig cc -c -fPIC scene/scene.c -o bin/o/scene.o
zig cc -shared -o bin/libscene.so bin/o/rectangle.o bin/o/voxel.o bin/o/scene.o

zig cc -o bin/rectangle.test scene/rectangle.test.c -Lbin -lscene
zig cc -o bin/voxel.test scene/voxel.test.c -Lbin -lscene
zig cc -o bin/scene.test scene/scene.test.c -Lbin -lscene
