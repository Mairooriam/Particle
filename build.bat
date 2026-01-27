@echo off
if not exist build (
    mkdir build
    cmake --preset default
)

set CLICOLOR_FORCE=1
cmake --build build --config Debug