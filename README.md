# Euzebia3D

Software 3D renderer and demo framework for Raspberry Pi Pico 2, built for demoscene productions.

Core characteristics:
- fixed-point math (`SHIFT_FACTOR = 12`)
- CPU triangle rasterization
- DMA-driven LCD framebuffer upload (`320x240`, RGB565)
- triangle sorting (painter's algorithm) instead of z-buffer

## Rendering Docs

- PL: [RENDERING_GUIDE_PL.md](./RENDERING_GUIDE_PL.md)
- EN: [RENDERING_GUIDE_EN.md](./RENDERING_GUIDE_EN.md)

## Current Rendering Approach

The renderer used to rely on z-buffering.
Currently, z-buffer was replaced with triangle sorting due to rendering correctness issues in the previous approach.

What is implemented now in `libs/renderer/renderer.c`:
- back-face culling in screen space
- scene triangle collection with cap `MAX_TRIANGLES_IN_SCENE = 1500`
- depth sort by average triangle depth (far-to-near draw order)
- affine UV interpolation with clamped sampling
- simple 2x2 texture averaging (box-filter-like, always enabled)
- per-vertex diffuse lighting with interpolation and intensity clamps

## Project Layout

- `Euzebia3D.c`: main app loop, hardware/display/painter/renderer setup, scene creation (camera/light/meshes), frame rendering.
- `libs/renderer`: transform + projection + triangle setup/rasterization + triangle sorting.
- `libs/painter`: full framebuffer operations, DMA transfer to LCD, sprites/text/gradient/fade/post-process helpers.
- `libs/meshFactory`, `libs/cameraFactory`, `libs/lightFactory`, `libs/puppetFactory`: object/factory modules for scene and animation elements.
- `libs/arithmetics`: fixed-point arithmetic, vectors/quaternions, trig lookup helpers.
- `libs/hardware`, `libs/display`: low-level board and LCD control.
- `libs/file_reader`: SD/FAT + WAV playback support (present in codebase; not used in current `main` loop).
- `libs/storage`: embedded assets (models/textures/fonts/sprites/post-processing data).
- `assets`: source assets (e.g. OBJ) used for conversion.
- `tools`: Python converters/exporters used to generate embedded asset data.

## Asset Pipeline

Runtime meshes/textures are loaded from embedded arrays in `libs/storage/gfx.c` (`get_model`, `get_image`).

Typical workflow:
1. edit source assets in `assets/`
2. convert/export with scripts from `tools/`
3. update generated data in `libs/storage/*`

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
- No explicit near-plane clipping is implemented; geometry behind the camera is rejected per-vertex.
- With triangle sorting, intersecting geometry can still produce painter-order artifacts in edge cases.
