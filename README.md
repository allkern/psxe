
# psxe
A simple and portable Sony PlayStation emulator and emulation library written in C

## Screenshots
| Windows  | Ubuntu |
| ------------- | ------------- |
| ![windows](https://github.com/allkern/psx/assets/15825466/7aea1203-33cf-4b26-aedb-4d9bead44d67)  | ![screenshotfrom2023-06-1023-59-51](https://github.com/allkern/psx/assets/15825466/27ac5d8d-7945-4c92-b950-19a35fcbdc81)  |

### CI status
![Windows](https://github.com/allkern/psx/actions/workflows/windows.yml/badge.svg)
![macOS](https://github.com/allkern/psx/actions/workflows/macos.yml/badge.svg)
![Ubuntu](https://github.com/allkern/psx/actions/workflows/ubuntu.yml/badge.svg)

## Running
You can just download the latest automated build for your platform on Releases. If your system isn't supported, you can easily build the emulator from source, instructions on "Building" below.

In order to run the emulator, you will need a BIOS file, `SCPH1001.bin` specifically, you can either get it from the internet or [dump it from your own console](https://www.youtube.com/watch?v=u8eHp0COcBo). Just put it in the same directory where the executable is located. I will write a CLI parser really soon though, you will be able to just specify the location of the BIOS or a search folder.

Other BIOSes aren't currently supported, though I've only tested `SCPH1000.bin` so far.

## Progress
Here's a list of what's currently implemented:
- All CPU instructions, excluding GTE
- CPU quirks (Branch, Load delay slots)
- Bus structure
- OTC DMA (Burst), GPU DMA (Request, Linked)
- Several GPU registers
- GPU commands used by the BIOS
- Some CDROM commands (`Test`, `Getstat`)

## Building
Building the emulator should be easy, just use the scripts provided in this repo.

On Windows, the `build-deps.ps1` script downloads SDL2 and unzips it. If you want to run the emulator standalone, you'll have to move the SDL2 DLL to the same folder where the executable is located.

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
git clone https://github.com/allkern/psx
cd psx
./build-deps
./build-win.ps1
```

If `build-win.ps1` fails for whatever reason (*cough* Microsoft *cough*), you can try using the `build-win.bat` (notice the extension) script instead. If this also doesn't work for you, please open an issue on the Issues tab
### Ubuntu
```
git clone https://github.com/allkern/psx
cd psx
make clean && make
```

### macOS
```
git clone https://github.com/allkern/psx
cd psx
./build.sh
```
