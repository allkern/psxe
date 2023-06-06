$SDL2_DIR = "sdl2\x86_64-w64-mingw32"
$PSX_DIR = "."

mkdir -Force -Path bin > $null

gcc -I"`"$($PSX_DIR)`"" `
    -I"`"$($SDL2_DIR)\include`"" `
    -I"`"$($SDL2_DIR)\include\SDL2`"" `
    "build-win32\*.o" `
    "psx\*.c" `
    "psx\dev\*.c" `
    "frontend\*.c" `
    -o "bin\psxe.exe" `
    -L"`"$($SDL2_DIR)\lib`"" `
    -m64 -lSDL2main -lSDL2 -Wno-overflow `
    -g -Ofast -Wall -pedantic -DLOG_USE_COLOR