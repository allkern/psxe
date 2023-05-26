git clone https://github.com/ocornut/imgui

$IMGUI_DIR = "..\imgui"
$SDL2_DIR = "..\sdl2\x86_64-w64-mingw32"

mkdir -Force -Path build-win32 > $null

Set-Location build-win32

Write-Output "Building ImGui..."

c++ -I"`"$($IMGUI_DIR)`"" `
    -I"`"$($IMGUI_DIR)\backends`"" `
    -I"`"$($SDL2_DIR)\include\SDL2`"" `
    "`"$($IMGUI_DIR)\backends\imgui_impl_sdl2.cpp`"" `
    "`"$($IMGUI_DIR)\backends\imgui_impl_sdlrenderer.cpp`"" `
    "`"$($IMGUI_DIR)\*.cpp`"" `
    -c -m64

Set-Location ..

Write-Output "Building debugger fonts..."

ld -r -b binary -o "build-win32\font.o" "res\UbuntuMono-Regular.ttf"

gcc -c psx/log.c