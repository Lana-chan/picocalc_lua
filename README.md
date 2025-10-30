# PicoCalc Lua

A Lua interpreter for PicoCalc. It contains a REPL, basic API to draw graphics, read keys and access the SD filesystem.

* Based on https://github.com/JeremyGrosser/picolua
* Keyboard and lcd drivers based on https://github.com/hisptoot/picocalc_luckfox_lyra
* Font comes from https://polyducks.itch.io/kitchen-sink-textmode-font
* Filesystem based on https://github.com/elehobica/pico_fatfs.git
* Lua-based Editor taken from https://github.com/cc-tweaked/CC-Tweaked
* C-based Editor taken from https://github.com/snaptoken/kilo-src

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

## Usage

|               |                        |
| ------------- | ---------------------- |
| Ctrl-Alt-F1   | Reboot in BOOTSEL mode |
| Ctrl-Alt-Del  | Reboot                 |
| Enter         | Excute                 |
| Ctrl-C        | Clear line             |
| Ctrl-L        | Clear screen           |
| Up/Down       | Small history          |

See [Getting Started](docs/Getting%20Started.md) and [API.md](docs/API.md) for full API documentation

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
