#!/bin/bash
rm -rf bin

mkdir -p bin/o


echo "=== compiling questions ====================================================================="
zig cc src/question_01.c -o bin/question_01
zig cc libscene/rectangle.c libscene/voxel.c libscene/scene.c src/question_02.c -o bin/question_02


echo "=== compiling libscene units ==================================================================================="
zig cc -c -fPIC libscene/rectangle.c -o bin/o/rectangle.o
zig cc -c -fPIC libscene/voxel.c -o bin/o/voxel.o
zig cc -c -fPIC libscene/scene.c -o bin/o/scene.o
zig cc -c -fPIC libfiledb/filedb.c -o bin/o/filedb.o
echo "=== compiling libscene dll ====================================================================================="
zig cc -shared -o bin/libscene.so bin/o/rectangle.o bin/o/voxel.o bin/o/scene.o
echo "=== compiling libfiledb dll ===================================================================================="
zig cc -shared -o bin/libfiledb.so bin/o/filedb.o -lcrypto

echo "=== compiling rectangle tests =================================================================================="
zig cc -o bin/rectangle.test libscene/rectangle.test.c -Lbin -lscene
echo "=== compiling voxel tests ======================================================================================"
zig cc -o bin/voxel.test libscene/voxel.test.c -Lbin -lscene
echo "=== compiling scene tests ======================================================================================"
zig cc -o bin/scene.test libscene/scene.test.c -Lbin -lscene
echo "=== compiling filedb tests ====================================================================================="
zig cc -o bin/filedb.test libfiledb/filedb.test.c -Lbin -lfiledb

echo "=== compiling filedbpp dll ====================================================================================="
g++ -shared -o bin/libfiledbpp.so libfiledb/filedbpp.cpp -Lbin -lssl -lcrypto -static-libgcc -static-libstdc++
echo "=== compiling filedbpp test ===================================================================================="
g++ -o bin/filedbpp.test libfiledb/filedbpp.test.cpp -Lbin -lfiledbpp
echo "=== compiling filedbpp gtest ===================================================================================="
g++ -o bin/filedbpp.gtest libfiledb/filedbpp.gtest.cpp libfiledb/filedbpp.cpp  -lssl -lcrypto -static-libgcc -static-libstdc++ -lgtest -lgtest_main -pthread

