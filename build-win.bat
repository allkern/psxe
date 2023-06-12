@echo off

set PSX_DIR=.
set SDL2_DIR=SDL2-2.26.5\x86_64-w64-mingw32

if not exist "bin" (
    mkdir bin
)

gcc psx/*.c psx/dev/*.c frontend/*.c -o bin/psxe^
    -L"%SDL2_DIR%/lib"^
    -I"%SDL2_DIR%\include"^
    -I"%SDL2_DIR%\include\SDL2"^
    -I"%PSX_DIR%"^
    -I"."^
    -DOS_INFO="%OS_INFO%"^
    -DREP_VERSION="%VERSION_TAG%"^
    -DREP_COMMIT_HASH="%COMMIT_HASH%"^
    -g -DLOG_USE_COLOR -lSDL2 -lSDL2main^
    -Ofast -Wno-overflow -Wall -pedantic