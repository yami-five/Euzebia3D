# Euzebia3D

Renderer and toolchain for a Raspberry Pi Pico 2–based 3D demo. Fixed-point math, software rasterizer, DMA-driven LCD output.

## Project layout
- `Euzebia3D.c`: main loop, sets up hardware/display/painter/renderer, builds scene (meshes, light, camera) and draws each frame.
- `libs/renderer`: fixed-point triangle rasterizer, z-buffer, shading, texturing (optionally bilinear), viewport scaling.
- `libs/painter`: LCD back-buffer management, DMA upload, pixel/ sprite/ gradient utilities, post-process effects.
- `libs/meshFactory`, `libs/cameraFactory`, `libs/lightFactory`, `libs/puppetFactory`: creation helpers for scene objects and animation bones.
- `libs/arithmetics`: fixed-point utilities, vectors/quaternions, lookup tables.
- `libs/hardware`, `libs/display`, `libs/file_reader`: hardware I/O, LCD wiring, SD/FAT access, audio setup.
- `assets`: sample models/textures.
- `tools`: python helpers for textures/geometry export.

## Build
1. Install Raspberry Pi Pico SDK 2.2 and toolchain (RP2040/2350). Ensure `PICO_SDK_PATH` is set.
2. Configure and build:
   ```bash
   cmake -B build -G Ninja
   cmake --build build
   ```
3. The firmware target is `Euzebia3D.elf` (and UF2 via Pico SDK extra outputs).

## Renderer specifics
- Fixed-point (SHIFT_FACTOR=12). Rasterizer works on a downscaled render buffer (`render_scale`, default 2 → 160x120) and upsamples to LCD (`output_scale` in painter).
- Z-buffer: 32-bit depth per pixel (affine z → inverse) sized to render resolution; cleared each frame.
- Texturing: affine UV with half-texel bias and clamp; optional simple bilinear (averages 4 texels).
- Shading: per-vertex diffuse, interpolated light dot, ambient floor; light intensity clamped to avoid overflow.
- Rasterization: barycentric interpolation per scanline, per-pixel depth/UV/shading, then upscaled writes to painter buffer.

## Tips and gotchas
- Avoid huge models: render buffers/z-buffer are allocated per scale; keep `render_scale` at 2 on Pico unless memory allows.
- Textures: add gutter/padding around UV islands to minimize bleeding; current sampler clamps UV inside `[1, texSize-2]`.
- Near/far: current pipeline uses affine inverse z; no explicit clipping. Keep meshes in front of camera.
- Normals and light vectors must be normalized; `norm_vector_safe` skips zero-length vectors.
- Effects/post-process in painter allocate temporary buffers; watch stack sizes if adding VLAs.

## Extending
- To change resolution: call `renderer_set_scale(scale)` before `init_renderer`.
- To tweak lighting: adjust `AMBIENT_MIN` and `INTENSITY_MAX` in `renderer.c`.
- To disable bilinear: revert `texturing` to nearest-only sampling.
