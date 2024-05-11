git fetch --all --tags

$VERSION_TAG = git describe --always --tags --abbrev=0
$COMMIT_HASH = git rev-parse --short HEAD
$OS_INFO = (Get-WMIObject win32_operatingsystem).caption + " " + `
           (Get-WMIObject win32_operatingsystem).version + " " + `
           (Get-WMIObject win32_operatingsystem).OSArchitecture

$SDL2_DIR = "SDL2-2.26.5\i686-w64-mingw32"
$PSX_DIR = "."

mkdir -Force -Path bin > $null

gcc -I"`"$($PSX_DIR)`"" `
    -I"`"$($PSX_DIR)\psx`"" `
    -I"`"$($SDL2_DIR)\include\SDL2`"" `
    "psx\*.c" `
    "psx\dev\*.c" `
    "psx\input\*.c" `
    "psx\disc\*.c" `
    "frontend\*.c" `
    -o "bin\psxe.exe" `
    -DREP_VERSION="`"$($VERSION_TAG)`"" `
    -DREP_COMMIT_HASH="`"$($COMMIT_HASH)`"" `
    -DOS_INFO="`"$($OS_INFO)`"" `
    -L"$($SDL2_DIR)\lib" `
    -lSDL2main -lSDL2 -Wno-overflow `
    -Wall -pedantic -DLOG_USE_COLOR `
    -Wno-address-of-packed-member `
    -ffast-math -Ofast -g -flto

    Copy-Item -Path "$($SDL2_DIR)\bin\SDL2.dll" -Destination "bin"