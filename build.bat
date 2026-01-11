@echo off
if not exist build (
    mkdir build
)
cd build
cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug ../src
set CLICOLOR_FORCE=1
ninja

REM Go back to src directory
cd ..