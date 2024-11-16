# psxe
A simple and portable Sony PlayStation emulator and emulation library written in C

## Screenshots
| Windows  | Ubuntu | macOS |
| ------------- | ------------- | ------------- 
| ![Mega Man X6 (USA)](https://github.com/allkern/psxe/assets/15825466/34dde8f9-eedb-4b44-a08d-c17026df2ff2) | ![Bloody Roar 2 - Bringer of the New Age (Europe)](https://github.com/allkern/psxe/assets/15825466/41a6dc67-b0ba-442f-bed6-7b207c0db4dd) | ![Parodius (Europe)](https://github.com/allkern/psxe/assets/15825466/9ab291d9-ec47-4997-92d3-23e38982ae45) |
| ![Spyro 2 - Ripto's Rage (USA)](https://github.com/allkern/psxe/assets/15825466/e161ab66-af57-4327-9a94-8b2591a0012a) | ![Namco Museum Vol. 1 (USA)](https://github.com/allkern/psxe/assets/15825466/67ea61e4-5f30-470c-a978-23e0755850b6) | ![Darius Gaiden (Japan)](https://github.com/allkern/psxe/assets/15825466/0c55118c-ab42-40e5-b34a-7594528080bf) |

### CI status
![Windows](https://github.com/allkern/psx/actions/workflows/windows.yml/badge.svg)
![macOS](https://github.com/allkern/psx/actions/workflows/macos.yml/badge.svg)
![Ubuntu](https://github.com/allkern/psx/actions/workflows/ubuntu.yml/badge.svg)

## Running
You can download the latest automated build for your platform on Releases. If your system isn't supported, you can easily build the emulator from source, instructions on "Building" below.

In order to run the emulator, you will need a BIOS file. You can either get one from the internet or [dump it from your own console](https://www.youtube.com/watch?v=u8eHp0COcBo).

Most BIOS versions are confirmed to work.

Use the `-b` or `--bios` setting to configure the BIOS file.

## Progress
All components have been implemented, Memory card support is temporarily disabled.

<img src="https://github.com/allkern/psxe/assets/15825466/199c20e4-4e7e-4d0a-a033-eda347034ed5" width="12" height="12"/> CPU </br>
<img src="https://github.com/allkern/psxe/assets/15825466/199c20e4-4e7e-4d0a-a033-eda347034ed5" width="12" height="12"/> DMA </br>
<img src="https://github.com/allkern/psxe/assets/15825466/199c20e4-4e7e-4d0a-a033-eda347034ed5" width="12" height="12"/> GPU </br>
<img src="https://github.com/allkern/psxe/assets/15825466/199c20e4-4e7e-4d0a-a033-eda347034ed5" width="12" height="12"/> SPU </br>
<img src="https://github.com/allkern/psxe/assets/15825466/199c20e4-4e7e-4d0a-a033-eda347034ed5" width="12" height="12"/> MDEC </br>
<img src="https://github.com/allkern/psxe/assets/15825466/199c20e4-4e7e-4d0a-a033-eda347034ed5" width="12" height="12"/> GTE </br>
<img src="https://github.com/allkern/psxe/assets/15825466/199c20e4-4e7e-4d0a-a033-eda347034ed5" width="12" height="12"/> Timers </br>
<img src="https://github.com/allkern/psxe/assets/15825466/199c20e4-4e7e-4d0a-a033-eda347034ed5" width="12" height="12"/> CDROM </br>
<img src="https://github.com/allkern/psxe/assets/15825466/0ed1fe97-de2f-47de-bb30-82286e6c5fa0" width="12" height="12"/> Memory cards </br>

## Building
Building the emulator should be easy, just use the scripts provided in this repo.

On Windows, the `build-deps.ps1` script downloads SDL2 and unzips it. If you want to run the emulator standalone, you will have to move the SDL2 DLL to the same folder where the executable is located.

**If you already have SDL2 on your system**, you can skip running `build-deps.ps1`. Though you will have to edit `build-win.ps1` to point the `SDL2_DIR` variable to your installation path.

On Ubuntu, you will also need to install `libsdl2-dev`, you can get it from `apt` like so:
```
sudo apt update
sudo apt upgrade
sudo apt install libsdl2-dev
```

Building on macOS requires installing SDL2 and dylibbundler, this can be done using `brew`:
```
brew install sdl2
brew install dylibbundler
```

Assuming you did everything described above, you should be able to build the emulator by using the following commands.

### Windows
```
git clone https://github.com/allkern/psxe
cd psxe
./build-deps
./build-win64.ps1
```
On rare cases these scripts might not work (PowerShell/Windows bugs). If so, please open an issue on the issues tab with information about your system so we can make sure we cover the maximum amount of systems. 

### Ubuntu
```
git clone https://github.com/allkern/psxe
cd psxe
make clean && make
```

### macOS
```
git clone https://github.com/allkern/psxe
cd psxe
./build.sh
```

## Acknowledgements
This project uses external open source code that can be found on the following GitHub repos:
- argparse.c: https://github.com/cofyc/argparse
- log.c (slightly modified): https://github.com/rxi/log.c
- tomlc99: https://github.com/cktan/tomlc99

Their original licenses are respected and apply to the code in this project.

As always, thanks to all original developers for their amazing work.
