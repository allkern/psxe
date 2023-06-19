#!/bin/sh

# Build emulator
make clean && make

# Create bundle filesystem
mkdir -p psxe.app/Contents/MacOS/Libraries

# Move executable to folder
mv bin/psxe psxe.app/Contents/MacOS

# Make executable
chmod 777 psxe.app/Contents/MacOS/psxe

# Bundle required dylibs
dylibbundler -b -x ./psxe.app/Contents/MacOS/psxe -d ./psxe.app/Contents/Libraries/ -p @executable_path/../Libraries/ -cd

# Move plist to Contents folder
mv Info.plist psxe.app/Contents/Info.plist