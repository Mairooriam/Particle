@echo off
if not exist build (
    mkdir build
)
cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Debug ../src
ninja clean
ninja application
cd ..