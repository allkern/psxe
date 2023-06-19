if (Test-Path "SDL2-2.26.5") {
    Remove-Item -Recurse "SDL2-2.26.5"
}

$DEV_URL = "https://github.com/libsdl-org/SDL/releases/download/release-2.26.5/SDL2-devel-2.26.5-mingw.zip"
$WIN32_URL = "https://github.com/libsdl-org/SDL/releases/download/release-2.26.5/SDL2-2.26.5-win32-x86.zip"
$WIN64_URL = "https://github.com/libsdl-org/SDL/releases/download/release-2.26.5/SDL2-2.26.5-win32-x64.zip"

Invoke-WebRequest -URI $DEV_URL -OutFile "sdl2.zip"
Expand-Archive "sdl2.zip" -DestinationPath "."

Invoke-WebRequest -URI $WIN32_URL -OutFile "sdl2-win32.zip"
Expand-Archive "sdl2-win32.zip" -DestinationPath "sdl2-win32"

Invoke-WebRequest -URI $WIN64_URL -OutFile "sdl2-win64.zip"
Expand-Archive "sdl2-win64.zip" -DestinationPath "sdl2-win64"

Remove-Item "sdl2.zip"
Remove-Item "sdl2-win32.zip"
Remove-Item "sdl2-win64.zip"