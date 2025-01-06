#!/bin/bash
rm -rf bin

mkdir -p bin/o

zig cc src/question_01.c -o bin/question_01
zig cc libscene/rectangle.c libscene/voxel.c libscene/scene.c src/question_02.c -o bin/question_02


zig cc -c -fPIC libscene/rectangle.c -o bin/o/rectangle.o
zig cc -c -fPIC libscene/voxel.c -o bin/o/voxel.o
zig cc -c -fPIC libscene/scene.c -o bin/o/scene.o
zig cc -c -fPIC libfiledb/filedb.c -o bin/o/filedb.o
zig cc -shared -o bin/libscene.so bin/o/rectangle.o bin/o/voxel.o bin/o/scene.o
zig cc -shared -o bin/libfiledb.so bin/o/filedb.o -lcrypto

zig cc -o bin/rectangle.test libscene/rectangle.test.c -Lbin -lscene
zig cc -o bin/voxel.test libscene/voxel.test.c -Lbin -lscene
zig cc -o bin/scene.test libscene/scene.test.c -Lbin -lscene
zig cc -o bin/filedb.test libfiledb/filedb.test.c -Lbin -lfiledb

g++ -o database_test database_test.cpp database.cpp record.cpp -lgtest -lgtest_main -pthread

