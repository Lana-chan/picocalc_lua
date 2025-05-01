# Minimal Lua for Picocalc

This is a simple lua interpreter for PicoCalc. It contains a REPL, basic API to draw graphics, read keys and access the SD filesystem.

* Based on https://github.com/JeremyGrosser/picolua
* Keyboard and lcd drivers based on https://github.com/hisptoot/picocalc_luckfox_lyra
* Font comes from https://polyducks.itch.io/kitchen-sink-textmode-font
* Filesystem based on https://github.com/elehobica/pico_fatfs.git

## Dependencies

    sudo apt update
    sudo apt install build-essential gcc-arm-none-eabi git cmake python3

    git clone --recurse-submodules https://github.com/benob/picocalc_lua
    cd picocalc_lua

## Build

    mkdir build
    cd build
    cmake .. -DPICO_BOARD=pico2 # change depending on your board
    make

## Run

Hold BOOTSEL button, connect USB.

    sudo cp picolua.uf2 /dev/disk/by-label/RP2350/

## Usage

    Ctrl-Alt-F1   Reboot in BOOTSEL mode
    Ctrl-Alt-Del  Reboot 
    Enter         Excute
    Ctrl-C        Clear line
    Ctrl-L        Clear screen
    Up/Down       Small history

## Examples

    lua> a=2
    lua> b=2
    print(a*b)
    4

## Binding to Pico SDK and other functions

A few simple bindings for SDK functions have been added as examples. Here we turn the LED on:

    lua> LED=25
    lua> set_output(LED, true)
    lua> set_pin(LED, true)

Complete API (in addition to standard lua stuff)

    -- filesystem
    fp = fs_open(path, mode) -- mode = 1 to read, 2 to write (see FatFS docs)
    fs_close(fp)
    data = fs_read(fp, size)
    size = fs_write(fp, data)
    fs_seek(fp, where)
    where = fs_tell(fp)
    fs_eof(fp)
    size = fs_size(fp)
    fs_sync(fp)
    dir = fs_opendir(path)
    fs_closedir(dir)
    size, is_dir, name = fs_readdir(dir)
    size, is_dir, name = fs_stat(path)
    fs_unlink(path)
    fs_rename(old_path, new_path)
    fs_mkdir(path)
    fs_chdir(path)
    fs_mount(path, mode)
    total_kb, free_kb = fs_getfree(path)

    -- pico/pico2
    bytes = get_total_memory()
    bytes = get_free_memory()
    reset() -- reboot
    bootsel() -- reboot as usb disk for flashing firmware
    set_output(pin, mode)
    set_pin(pin, value)
    value = get_pin(pin)

    -- keyboard input
    state, modifiers, key = keyboard_wait()
    state, modifiers, key = keyboard_poll() -- key = 0 if no event

    -- drawing
    draw_text(x, y, fg, bg, text)
    draw_clear()
    color = draw_color_from_rgb(r, g, b)
    r, g, b = draw_color_to_rgb(color)
    color = draw_color_from_hsv(h, s, v)
    h, s, v = draw_color_to_hsv(color)
    color = draw_color_add(c1, c2)
    color = draw_color_subtract(c1, c2)
    color = draw_color_mul(c1, factor)
    draw_point(x, y, color)
    draw_rect(x, y, width, height, color)
    draw_fill_rect(x, y, width, height, color)
    draw_line(x1, y1, x2, y2, color)
    draw_circle(x, y, radius, color)
    draw_fill_circle(x, y, radius, color)
    draw_polygon(points, color) -- points is table {x1, y1, x2, y2, ...}
    draw_fill_polygon(points, color)
    draw_triangle_shaded(c1, x1, y1, c2, y1, y2, c3, y1, y2)

## Notes

The lua-5.4.6 distribution is copied from the release source tarball with the following modifications:
- Added src/CMakeLists.txt, which lists all of the .c files except lua.c and luac.c
- Changed `LUA_32BITS` to `1` in luaconf.h

## References

- [PicoCalc by Clockwork](https://www.clockworkpi.com/picocalc)
- [Getting Started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
- [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf)
- [Lua 5.4 Reference Manual](https://www.lua.org/manual/5.4/manual.html)
- [Programming in Lua](https://www.lua.org/pil/)
