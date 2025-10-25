# PicoCalc Lua Getting Started

You can download the the latest binary releases from [here](https://github.com/Lana-chan/picocalc_lua/releases/latest). The binaries are tagged 2040 for RP2040-based boards (Pico 1) and 2350 for RP2350-based boards (Pico 2).

To flash an UF2 firmware to your Pico board, connect a USB cable to it while holding down the BOOTSEL button on it. The BOOTSEL button should be accessible through the grill slots on the back of the PicoCalc. Done correctly, the Pico should show up to your computer as a removable drive where you can simply copy the appropriate UF2 file to.

Alternatively, it's recommended to install [a UF2 bootloader](https://github.com/pelrun/uf2loader) to your board, which then can be used to load PicoCalc Lua from the SD card.

## Defaults

If an SD card is present and able to be mounted, PicoCalc Lua will try to access the following files at boot:

* `default.fnt`: to automatically set a custom terminal font before startup
* `main.lua`: to run as soon as the Lua machine is initialized, useful for loading modules or setting default values

## Running programs

To run a program, you can simply use `dofile` pointing at a valid file in your SD card:

```lua
dofile("script.lua")
```

To run some of the demos included in `sd_files`:

```lua
dofile("lua/mandelbrot.lua")
dofile("lua/bubble.lua")
dofile("lua/asteroids.lua")
```

Once you are ready to write your own progams, see the [API reference](API.md) for an overview of the custom functions in PicoCalc Lua.

## Editor

The editor currently bundled in the SD files was taken and slightly modified from the [CC: Tweaked](https://github.com/cc-tweaked/CC-Tweaked) project. Due to the nature of the editor, it uses about 200kB of memory and is therefore only usable in RP2350 boards due to the lack of memory available in RP2040 boards.

If you have the `main.lua` from `sd_files` included in your SD card, you should see "Editor loaded" on your terminal at boot. In that case, you can invoke the editor like so:

```lua
edit("filename")
```

where `"filename"` is a valid path in your SD card. In case the file doesn't exist, a new one will be created at that path.

## Fonts

You can convert any monospaced bitmap font to the GRASP format for use in PicoCalc Lua by using the [monobit](https://github.com/robhagemans/monobit) utility like so:

```bash
monobit-convert input.bdf to --format=grasp output.fnt
```

Once you have converted and copied the font files to your SD card, you can load them either by renaming them to `default.fnt` on the root of your card, or like so in runtime:

```lua
term.loadFont("output.fnt")
```