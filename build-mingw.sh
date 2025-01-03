#!/bin/bash

# Get the current commit hash
COMMIT_HASH=$(git rev-parse HEAD)
echo "$COMMIT_HASH"

# Get the latest tag
TAG=$(git describe --tags --abbrev=0 2>/dev/null)
echo "$TAG"

# If TAG is empty, use the first argument as the tag
if [ -z "$TAG" ]; then
    TAG="$1"
fi

rm -rf bin

mkdir -p bin/o
####### 
####### echo "=== compiling question_01.exe _________________============================================================="
####### x86_64-w64-mingw32-gcc src/question_01.c -o bin/question_01.exe
####### echo "=== compiling question_02.exe _________________============================================================="
####### x86_64-w64-mingw32-gcc libscene/rectangle.c libscene/voxel.c libscene/scene.c src/question_02.c -o bin/question_02.exe
####### echo "=== compiling question_01.dll _________________============================================================="
####### x86_64-w64-mingw32-gcc -shared src/question_01.c -o bin/question_01.dll
####### echo "=== compiling question_02.dll _________________============================================================="
####### x86_64-w64-mingw32-gcc -shared libscene/rectangle.c libscene/voxel.c libscene/scene.c src/question_02.c -o bin/question_02.dll
####### 
####### 
####### echo "=== compiling libscene.dll ____________________============================================================="
####### x86_64-w64-mingw32-gcc -c -fPIC libscene/rectangle.c -o bin/o/rectangle.o
####### x86_64-w64-mingw32-gcc -c -fPIC libscene/voxel.c -o bin/o/voxel.o
####### x86_64-w64-mingw32-gcc -c -fPIC libscene/scene.c -o bin/o/scene.o
####### x86_64-w64-mingw32-gcc -shared -o bin/libscene.dll bin/o/rectangle.o bin/o/voxel.o bin/o/scene.o
####### 
####### FLAGS="-std=c99"
####### FLAGS="$FLAGS -D_DEFAULT_SOURCE"
####### FLAGS="$FLAGS -Wno-missing-braces"
####### FLAGS="$FLAGS -s"
####### FLAGS="$FLAGS -DPLATFORM_DESKTOP"
####### FLAGS="$FLAGS -I./libfiledb"
####### FLAGS="$FLAGS -I./libscene"
####### FLAGS="$FLAGS -lcrypto"
####### FLAGS="$FLAGS -lwinmm"
####### 
####### echo "=== compiling rectangle.test.exe ______________============================================================="
####### x86_64-w64-mingw32-gcc -o bin/rectangle.test.exe libscene/rectangle.test.c $FLAGS -Lbin -lscene
####### echo "=== compiling voxel.test.exe __________________============================================================="
####### x86_64-w64-mingw32-gcc -o bin/voxel.test.exe libscene/voxel.test.c $FLAGS -Lbin -lscene
####### echo "=== compiling scene.test.exe __________________============================================================="
####### x86_64-w64-mingw32-gcc -o bin/scene.test.exe libscene/scene.test.c $FLAGS -Lbin -lscene
####### echo "# Compile the shared library (DLL)"
####### # Compile the shared library (DLL)
####### 
####### echo "=== compiling rectangle.test.dll ______________============================================================="
####### x86_64-w64-mingw32-gcc -shared -o bin/rectangle.test.dll libscene/rectangle.test.c -Wl,--output-def,bin/rectangle.test.def $FLAGS -Lbin -lscene
####### echo "=== compiling voxel.test.dll __________________============================================================="
####### x86_64-w64-mingw32-gcc -shared -o bin/voxel.test.dll libscene/voxel.test.c -Wl,--output-def,bin/voxel.test.def $FLAGS -Lbin -lscene
####### echo "=== compiling scene.test.dll __________________============================================================="
####### x86_64-w64-mingw32-gcc -shared -o bin/scene.test.dll libscene/scene.test.c -Wl,--output-def,bin/scene.test.def $FLAGS -Lbin -lscene


FLAGS="-std=c99"
FLAGS="$FLAGS -D_DEFAULT_SOURCE"
FLAGS="$FLAGS -Wno-missing-braces"
FLAGS="$FLAGS -s"
FLAGS="$FLAGS -DPLATFORM_DESKTOP"
FLAGS="$FLAGS -I./libfiledb"
# FLAGS="$FLAGS -lcrypto"
# FLAGS="$FLAGS -lwinmm"


export LDD_PATH="$LDD_PATH:$(pwd)/bin"
echo "$LDD_PATH"
export PATH="$PATH:$(pwd)/bin"
echo "$PATH"

echo "=== compiling filedb.o ________________________============================================================="
x86_64-w64-mingw32-gcc -c -fPIC libfiledb/filedb.c -o bin/o/filedb.o
echo "=== compiling libfiledb.dll ___________________============================================================="
x86_64-w64-mingw32-gcc -shared -o bin/libfiledb.dll bin/o/filedb.o
echo "=== compiling filedb.test.exe _________________============================================================="
x86_64-w64-mingw32-gcc -o bin/filedb.test.exe libfiledb/filedb.test.c $FLAGS -Lbin -lfiledb
echo "=== compiling filedb.test.dll _________________============================================================="
x86_64-w64-mingw32-gcc -shared -o bin/filedb.test.dll libfiledb/filedb.test.c -Wl,--output-def,bin/filedb.test.def $FLAGS -Lbin -lfiledb
# Create a compressed archive with 7z
# "C:\Program Files\7-Zip\7z.exe" a "build/voxd31-${TAG}-x86_64-windows.7z" ./build/win64/*
