@echo off
if not exist build (
    mkdir build
)
cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Debug ../src
set CLICOLOR_FORCE=1
ninja
cd ..