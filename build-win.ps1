$SDL2_DIR = "SDL2-2.26.5\x86_64-w64-mingw32"
$PSX_DIR = "."

mkdir -Force -Path bin > $null

gcc -I"`"$($PSX_DIR)`"" `
    -I"`"$($SDL2_DIR)\include`"" `
    -I"`"$($SDL2_DIR)\include\SDL2`"" `
    "psx\*.c" `
    "psx\dev\*.c" `
    "frontend\*.c" `
    -o "bin\psxe.exe" `
    -L"`"$($SDL2_DIR)\lib`"" `
    -m64 -lSDL2main -lSDL2 -Wno-overflow `
    -Wall -pedantic -DLOG_USE_COLOR `
    -ffast-math -Ofast