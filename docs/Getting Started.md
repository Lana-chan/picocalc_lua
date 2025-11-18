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

PicoCalc Lua comes with an editor based on [kilo](https://github.com/snaptoken/kilo-src) built into the firmware. To launch it, call the `edit` command:

```lua
edit([filename])
```

The `filename` parameter is optional, and if omitted, the editor will prompt you for a filename before the first time saving. A new file will be created if a filename that doesn't exist is provided by either manner.

|               |                            |
| ------------- | -------------------------- |
| Shift+Arrows  | Page up/down, move by word |
| F1            | Save                       |
| F2            | Quit                       |
| F3            | Find                       |
| F4            | Mark mode (select)         |
| F5            | Run current file           |
| Ctrl-X        | Cut (in mark mode)         |
| Ctrl-C        | Copy (in mark mode)        |
| Ctrl-V        | Paste clipboard            |
| Ctrl-L        | Toggle line numbers        |

### ComputerCraft editor

Previously PicoCalc Lua relied on bundling an editor orignally made for the [CC: Tweaked](https://github.com/cc-tweaked/CC-Tweaked) project due to its ease of portability for being written in Lua. However, this editor consumes too much RAM to be used on RP2040 devices and is only suitable for RP2350.

The `main.lua` startup file included in `sd_files` has a commented out invocation to load this editor under the `ccedit` global function. If you enable it, you should see "Editor loaded" on your terminal at boot. In that case, you can start the editor like so:

```lua
ccedit("filename")
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