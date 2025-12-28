@echo off
REM Create build directory if it doesn't exist (relative to parent of misc)
if not exist build (
    mkdir build
)

REM Change to build directory
cd build

cmake -G "Ninja" ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DANIMATED=OFF ^
    ../src


REM Build the project in Debug mode using Ninja
ninja

REM Move this to cmake if works
copy NUL mir.lock

REM Go back to src directory
cd ..
