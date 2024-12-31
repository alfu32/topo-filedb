#!/bin/bash
zig cc question_01.c -o question_01
zig cc rectangle.c voxel.c scene.c question_02.c -o question_02


zig cc rectangle.c rectangle.test.c -o rectangle.test.bin
zig cc rectangle.c voxel.c voxel.test.c -o voxel.test.bin
zig cc rectangle.c voxel.c scene.c scene.test.c -o scene.test.bin