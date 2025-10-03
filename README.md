# Minimal Lua for Picocalc

This is a simple lua interpreter for PicoCalc. It contains a REPL, basic API to draw graphics, read keys and access the SD filesystem.

* Based on https://github.com/JeremyGrosser/picolua
* Keyboard and lcd drivers based on https://github.com/hisptoot/picocalc_luckfox_lyra
* Font comes from https://polyducks.itch.io/kitchen-sink-textmode-font
* Filesystem based on https://github.com/elehobica/pico_fatfs.git
* Editor taken from https://github.com/cc-tweaked/CC-Tweaked

## Dependencies

```
sudo apt update
sudo apt install build-essential gcc-arm-none-eabi git cmake python3

git clone --recurse-submodules https://github.com/Lana-chan/picocalc_lua
cd picocalc_lua
```

## Build

```
mkdir build
cd build
cmake .. -DPICO_BOARD=pico2 # change depending on your board
make
```

## Run

Hold BOOTSEL button, connect USB.

```
sudo cp picolua.uf2 /dev/disk/by-label/RP2350/
```

## Usage

Ctrl-Alt-F1   Reboot in BOOTSEL mode
Ctrl-Alt-Del  Reboot 
Enter         Excute
Ctrl-C        Clear line
Ctrl-L        Clear screen
Up/Down       Small history

## Examples

```
lua> a=2
lua> b=2
print(a*b)
4
```

## Binding to Pico SDK and other functions

A few simple bindings for SDK functions have been added as examples. Here we turn the LED on:

```
lua> LED=25
lua> sys.setOutput(LED, true)
lua> sys.setPin(LED, true)
```

See [API.md](API.md) for full API documentation

## Notes

The lua-5.4.8 distribution is copied from the release source tarball with the following modifications:
- Added src/CMakeLists.txt, which lists all of the .c files except lua.c and luac.c
- Changed `LUA_32BITS` to `1` in luaconf.h
- `luaL_loadfilex` and `readable` reworked to use `pico_fatfs` calls in lauxlib.c and loadlib.c

## References

- [PicoCalc by Clockwork](https://www.clockworkpi.com/picocalc)
- [Getting Started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
- [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf)
- [Lua 5.4 Reference Manual](https://www.lua.org/manual/5.4/manual.html)
- [Programming in Lua](https://www.lua.org/pil/)
