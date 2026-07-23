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
- optional 2x2 texture averaging (box-filter-like), controlled at build time
- per-vertex diffuse lighting with scanline interpolation and intensity clamps

## Project Layout

- `Euzebia3D.c`: Engine API example with scene creation and the main render loop.
- `libs/engine_api`: the public Euzebia3D API; owns engine initialization and delegates calls to internal modules.
- `libs/renderer`: transform + projection + triangle setup/rasterization + triangle sorting.
- `libs/painter`: full framebuffer operations, DMA transfer to LCD, sprites/text/gradient/fade/post-process helpers.
- `libs/meshFactory`, `libs/cameraFactory`, `libs/lightFactory`, `libs/puppetFactory`: object/factory modules for scene and animation elements.
- `libs/arithmetics`: fixed-point arithmetic, vectors/quaternions, trig lookup helpers.
- `libs/hardware`, `libs/display`: low-level board and LCD control.
- `libs/audio_player`: SD/FAT + WAV playback support on Pico and miniaudio-backed WAV playback on Windows; not used in current `main` loop.
- `storage`: embedded models, textures, fonts, sprites, puppets, and post-processing data.
- `assets`: source assets (e.g. OBJ) used for conversion.
- `tools`: Python converters/exporters used to generate embedded asset data.

## Asset Pipeline

Runtime assets are accessed by the engine through its internal storage module, with data defined in `storage/*.c`.

Typical workflow:
1. edit source assets in `assets/`
2. convert/export with scripts from `tools/`
3. update generated data in `storage/*`

## Using the Engine API

Applications should include only `engineApi.h` and keep one
`e3d_EngineContext`. Do not initialize or call the painter, renderer, storage,
or object factories directly.

Minimal scene setup:

```c
#include "engineApi.h"

static e3d_EngineContext engine_ctx;

int main(void) {
  e3d_InitEngine(&engine_ctx);

  e3d_Material *material =
      e3d_Material_CreateTexturedMat(&engine_ctx, 0, 0.0f, 0.0f, false);
  e3d_Mesh *mesh = e3d_Mesh_CreateMesh(&engine_ctx, material, 1);
  e3d_Light *light = e3d_Light_CreatePointLight(
      &engine_ctx, -10.0f, 3.0f, 15.0f, 15.0f, 0xffff);
  e3d_Camera *camera = e3d_Camera_CreateCamera(
      &engine_ctx, 0.0f, 75.0f, 100.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f);

  e3d_Renderer_SetLight(&engine_ctx, light);
  e3d_Renderer_SetCamera(&engine_ctx, camera);

  for (;;) {
    e3d_Renderer_CleanScene(&engine_ctx);
    e3d_Renderer_AddModelToScene(&engine_ctx, mesh);
    e3d_Renderer_RenderScene(&engine_ctx);
    e3d_Buffer_DrawBuffer(&engine_ctx);
    e3d_Buffer_ClearBuffer(&engine_ctx, 0);
  }
}
```

All public engine-owned types use the `e3d_` prefix. API functions follow the
`e3d_<Module>_<Operation>` naming scheme, for example
`e3d_Painter_Print()` and `e3d_Renderer_RenderScene()`.

For CMake consumers, link only the aggregate API target:

```cmake
target_link_libraries(my_demo PRIVATE Euzebia3D_libs)
```

Storage access remains an implementation detail. Its accessor implementation is
centralized in `storage/storage.c`, which includes the generated data sources
from the other `storage/*.c` files.

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
- `tools/calc_margin.py`: calculates positions for centered text used by scroller/text layouts.

## Build

Requirements:
- CMake
- C compiler toolchain for selected platform
- for `PICO`: Raspberry Pi Pico SDK `2.3.0` + Pico toolchain for RP2350
- for `WINDOWS`: SDL3 development package and miniaudio header (for example via `vcpkg`)

### Select Platform

Select the active platform with the PowerShell script:

```powershell
.\tools\select_platform.ps1
```

The script asks for Pico or Windows and generates the root `CMakeLists.txt` only
for that platform. For non-interactive use, pass `-Platform Pico` or
`-Platform Windows`. The build scripts select their platform automatically.

Use separate build directories per platform to avoid cache conflicts.

Useful build flags:
- `EUZEBIA3D_DEBUG_STAGE_ENABLED=OFF` compiles out the volatile `*_debug_stage` variables and writes to them.
- `EUZEBIA3D_RENDERER_SHADING_ENABLED=OFF` disables per-pixel lighting/shading.
- `EUZEBIA3D_RENDERER_TEXTURE_FILTER_2X2_ENABLED=OFF` switches texture sampling from 2x2 filtering to nearest texel.
- `EUZEBIA3D_RENDERER_PERSPECTIVE_CORRECT_UV_ENABLED=OFF` uses affine UV interpolation instead of perspective-correct UVs.
- `EUZEBIA3D_RENDERER_SCENE_SORT_ENABLED=OFF` skips scene triangle depth sorting.

### Build for Windows

Configure:

```bash
cmake -S . -B build-windows
```

If SDL3 is installed via `vcpkg`, configure with toolchain:

```bash
cmake -S . -B build-windows -DCMAKE_TOOLCHAIN_FILE=C:/Repos/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
```

With `vcpkg`, install the Windows dependencies first:

```bash
vcpkg install sdl3:x64-windows miniaudio:x64-windows
```

Build:

```bash
cmake --build build-windows --config Release
```

Output target: `Euzebia3D_PC.exe`.

`tools/build_windows.ps1` creates the Visual Studio solution and builds it. Use `-Clean` to recreate `build-windows` from scratch.

### Build for Raspberry Pi Pico

Configure:

```bash
cmake -S . -B build-pico -DCMAKE_BUILD_TYPE=Release
```

Build:

```bash
cmake --build build-pico
```

Output target: `Euzebia3D.elf` (plus additional Pico outputs, including UF2).

`tools/build_pico.ps1` builds the firmware and copies the UF2 to a connected Pico in BOOTSEL mode. Use `-Clean` to recreate `build-pico`; `tools/debug_pico.ps1` builds the Debug variant and starts OpenOCD/GDB.

Notes:
- `PICO_BOARD` is set to `pico2` in `CMakeLists.txt`.
- `pico_vscode.cmake` is auto-included when present at `~/.pico-sdk/cmake/pico-vscode.cmake`.

## Practical Notes

- `e3d_Renderer_SetRendererScale(...)` controls internal render resolution scaling. The engine initializes it to `1` (full `320x240` internal rendering).
- The painter uses a full framebuffer (`BUFFER_SIZE = 153600` bytes) and streams it via DMA in chunks.
- Triangles crossing the near plane are clipped before perspective divide; geometry fully behind camera is rejected.
- With triangle sorting, intersecting geometry can still produce painter-order artifacts in edge cases.

## Third-Party Code

`libs/audio_player/fatfs` contains FatFS R0.15a by ChaN. FatFS is redistributed under the license notice included in its source files.

When redistributing FatFS source, keep the copyright notice, redistribution condition, and warranty/liability disclaimer from the FatFS files.
