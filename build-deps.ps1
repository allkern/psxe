Remove-Item -Recurse "SDL2-2.26.5"

$URL = "https://github.com/libsdl-org/SDL/releases/download/release-2.26.5/SDL2-devel-2.26.5-mingw.zip"

Invoke-WebRequest -URI $URL -OutFile "sdl2.zip"
Expand-Archive "sdl2.zip" -DestinationPath "."

Remove-Item "sdl2.zip"

Copy-Item -Path "SDL2-2.26.5\x86_64-w64-mingw32\bin\SDL2.dll" -Destination bin