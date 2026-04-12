# Euzebia3D

Software 3D renderer and demo framework for Raspberry Pi Pico 2, built for demoscene productions.

Core characteristics:
- fixed-point math (`SHIFT_FACTOR = 12`)
- CPU triangle rasterization
- DMA-driven LCD framebuffer upload (`320x240`, RGB565)
- triangle sorting (painter's algorithm) instead of z-buffer

## Current Rendering Approach

The renderer used to rely on z-buffering.
Currently, z-buffer was replaced with triangle sorting due to rendering correctness issues in the previous approach.

What is implemented now in `libs/renderer/renderer.c`:
- back-face culling in screen space
- near-plane clipping in clip space (`z > 0`) with triangle fan reconstruction after clipping
- scene triangle collection with cap `MAX_TRIANGLES_IN_SCENE = 1500`
- depth sort by average triangle depth (far-to-near draw order)
- scanline rasterization with edge stepping and per-pixel lerp
- perspective-correct UV interpolation (`U=u*(1/z)`, `V=v*(1/z)`, `W=1/z`) with clamped sampling
- simple 2x2 texture averaging (box-filter-like, always enabled)
- per-vertex diffuse lighting with scanline interpolation and intensity clamps

## Project Layout

- `Euzebia3D.c`: main app loop, hardware/display/painter/renderer setup, scene creation (camera/light/meshes), frame rendering.
- `libs/renderer`: transform + projection + triangle setup/rasterization + triangle sorting.
- `libs/painter`: full framebuffer operations, DMA transfer to LCD, sprites/text/gradient/fade/post-process helpers.
- `libs/meshFactory`, `libs/cameraFactory`, `libs/lightFactory`, `libs/puppetFactory`: object/factory modules for scene and animation elements.
- `libs/arithmetics`: fixed-point arithmetic, vectors/quaternions, trig lookup helpers.
- `libs/hardware`, `libs/display`: low-level board and LCD control.
- `libs/file_reader`: SD/FAT + WAV playback support (present in codebase; not used in current `main` loop).
- `storage`: embedded assets and storage API (models/textures/fonts/sprites/post-processing data, `get_storage()` access).
- `assets`: source assets (e.g. OBJ) used for conversion.
- `tools`: Python converters/exporters used to generate embedded asset data.

## Asset Pipeline

Runtime assets are loaded through `IStorage` (`get_model`, `get_image`, etc.), with data defined in `storage/*.c`.

Typical workflow:
1. edit source assets in `assets/`
2. convert/export with scripts from `tools/`
3. update generated data in `storage/*`

## Using Storage

Main entry point:
- include `storage.h`
- call `get_storage()` once during init
- pass the returned `IStorage*` to modules that need assets

Example (as in current app init flow):

```c
#include "storage.h"

static const IStorage *storage;

storage = get_storage();
painter->init_painter(display, hardware_core, storage);
meshFactory->init_mesh_factory(storage);
```

`IStorage` API includes:
- `get_font_by_index(uint8_t index)`
- `get_image(uint8_t image_index)`
- `get_model(uint8_t model_index)`
- `get_effect_table(uint8_t effect_index)`
- `get_effect_table_element(uint8_t effect_index, uint32_t e_index)`
- `get_raw_puppet(uint8_t puppetIndex)`
- `get_scroller_by_index(uint8_t index)`
- `get_sprite(uint8_t sprite_index)`

Implementation note:
- accessor implementation is centralized in `storage/storage.c`
- `storage/storage.c` is built as a single translation unit for storage and includes data sources from the other `storage/*.c` files

## Tools

`tools/` contains helper scripts for asset conversion/generation.

Run from repository root:

```bash
python tools/<script>.py
```

Requirements:
- Python 3
- Pillow (`pip install pillow`) for image scripts (`bmp_converter.py`, `texture_converter.py`, `font_exporter.py`)

Scripts:
- `tools/obj_exporter.py`: reads `assets/<fileName>.obj` and prints C arrays for vertices/faces/uv/normals to stdout. Input file name is set in script (`fileName = "..."`).
- `tools/texture_converter.py`: converts `assets/models/<file_name>.bmp` to RGB565 C array and writes `tools/img_converted.txt`. Input name is set in script (`file_name = "..."`).
- `tools/bmp_converter.py`: converts `assets/<sprite_name>.bmp` to RGB565 C array and writes `tools/img_converted.txt`. Input name is set in script (`sprite_name = "..."`).
- `tools/font_exporter.py`: converts `assets/letters.bmp` into packed font data and writes `assets/font_converted.txt`.
- `tools/barrel_distortion.py`: generates barrel distortion lookup table and writes `assets/barrel_dist.txt`.
- `tools/init_sin_cos.py`: generates trig lookup tables and writes `tools/sin_cos_atan.txt`.
- `tools/calc_margin.py`: prints centered `painter->print(...)` calls for prepared text lines (helper for scroller/text layout).

## Build

Requirements:
- Raspberry Pi Pico SDK `2.2.0`
- Pico toolchain for RP2350
- CMake + Ninja

Configure and build:

```bash
cmake -B build -G Ninja
cmake --build build
```

Output target: `Euzebia3D.elf` (plus additional Pico outputs, including UF2).

Notes:
- `PICO_BOARD` is set to `pico2` in `CMakeLists.txt`.
- `pico_vscode.cmake` is auto-included when present at `~/.pico-sdk/cmake/pico-vscode.cmake`.

## Practical Notes

- `renderer->set_scale(...)` controls internal render resolution scaling. Current `main` sets scale to `1` (full `320x240` internal rendering).
- The painter uses a full framebuffer (`BUFFER_SIZE = 153600` bytes) and streams it via DMA in chunks.
- Triangles crossing the near plane are clipped before perspective divide; geometry fully behind camera is rejected.
- With triangle sorting, intersecting geometry can still produce painter-order artifacts in edge cases.
