# Euzebia3D - Step-by-Step Rendering Explanation (EN)

This document is for someone who:
- receives the project from someone else,
- has never programmed in C for Raspberry Pi Pico,
- wants to understand "what happens from a 3D model to a pixel on the LCD".

You do not need to know all the 3D math details to understand this description.

---

## 1. What does this actually do?

The project renders a 3D scene in software (CPU), without a GPU.

So:
1. it takes a 3D model (vertices + triangles),
2. rotates/moves it,
3. projects it to a 2D screen,
4. converts triangles into pixels,
5. sends the whole image to the LCD through DMA.

At the moment, the project uses **triangle sorting** (painter's algorithm), not a z-buffer.

---

## 2. Most important files (project map)

- `Euzebia3D.c`
  Main program loop, where you can see "what happens every frame".
- `libs/renderer/renderer.c`
  The core of 3D rendering: clipping, culling, sorting, rasterization, shading, texturing.
- `libs/cameraFactory/camera.c`
  Camera: view matrix (`vMatrix`) and projection matrix (`pMatrix`).
- `libs/meshFactory/meshFactory.c`
  Creation of Mesh objects from data embedded in the project.
- `libs/painter/painter.c`
  Screen buffer + DMA transfer to LCD.
- `libs/display/display.c`
  LCD initialization.
- `libs/hardware/hardware.c`
  SPI, GPIO, PWM, etc. initialization.
- `libs/storage/gfx.c`
  Model and texture data compiled into firmware.

---

## 3. How the program starts (step by step)

In `main()` (`Euzebia3D.c`) this happens:

1. Pico clock is set to 300 MHz.
2. Hardware layer starts (`init_hardware`).
3. LCD starts (`init_display`).
4. Painter starts (buffer + DMA).
5. Renderer starts (`init_renderer`) and render scale is set (`set_scale(1)`).
6. Scene objects are created:
   - 2 meshes (currently 2 mugs),
   - 1 point light,
   - 1 camera.
7. Enter `while(1)` loop - this is rendering of consecutive frames.

---

## 4. What happens in every frame?

In each loop iteration:

1. Transformations are changed (e.g. object rotation, camera movement).
2. Camera is updated (`update_camera`) - matrices are recalculated.
3. Renderer clears the scene triangle list (`clean_scene`).
4. Each model is added to the scene (`add_model_to_scene`).
5. Renderer draws the entire scene (`render_scene`).
6. Painter sends final buffer to LCD (`draw_buffer`).
7. Buffer is cleared with background color for the next frame.

So, simply: **compute -> draw -> send -> clear -> repeat**.

---

## 5. What are Mesh, Material, Triangle?

### Mesh
A Mesh is a 3D object:
- list of vertices,
- list of triangles (faces),
- UV (texture coordinates),
- normals,
- material.

### Material
Material tells the renderer:
- whether to use a texture (`texture` + `textureSize`),
- whether to use a flat color (`diffuse`),
- whether it is a skybox (`isSkyBox`).

### Triangle
The renderer finally works on triangles.
Each triangle has:
- 3 points,
- UV,
- lighting data.

---

## 6. Most important part: 3D rendering pipeline

Below is the main path "from model to pixels":

### Stage A: model transformations

For each model:
- its data is copied to working buffers,
- transformations are applied (rotation/translation/scale).

This happens in `add_model_to_scene`.

### Stage B: camera (view + projection)

Each vertex goes through:
1. view matrix (`vMatrix`) - "how the camera looks",
2. projection matrix (`pMatrix`) - "how to convert 3D to 2D".

After this step we have clip-space coordinates (`x, y, z, w`).

### Stage C: clipping (near plane)

This is the recently improved stage.

Instead of discarding the whole triangle when part of it is behind the camera:
- the triangle is clipped to near plane (`z > 0`),
- new edge vertices are interpolated,
- UV and light are interpolated too.

Effect: fewer "disappearing" triangles near the camera.

### Stage D: triangulation after clipping

After clipping, a polygon can have:
- 3 vertices -> remains 1 triangle,
- 4 vertices -> split into 2 triangles.

### Stage E: perspective divide

For each vertex:
- `screen_x = x / w`,
- `screen_y = y / w`.

This gives 2D screen position.

### Stage F: back-face culling

If a triangle "faces away" from the camera, it is not drawn.
This saves CPU time.

### Stage G: writing triangles to scene list

Triangles go into static array `scene[]`.
There is a limit (`MAX_TRIANGLES_IN_SCENE = 1500`).

### Stage H: triangle sorting

Before drawing, triangles are sorted by depth (average `z`):
- far first,
- near later.

This replaces z-buffer.

### Stage I: rasterization (triangle -> pixels)

Each triangle is drawn scanline-style:
- line by line,
- pixel by pixel.

There are no per-pixel barycentrics anymore.
`tri(...)` drives two scanline edges, and `rasterize(...)` interpolates along `x`.

### Stage J: pixel color

For each pixel:
1. Get color (texture or flat color).
2. Shading (light):
   - interpolation of per-vertex light,
   - clamp (to prevent values from "blowing up").
3. Write RGB565 color into LCD buffer.

### Stage K: send full image to LCD

`painter.draw_buffer()`:
- takes the entire frame buffer,
- sends it via DMA, in chunks, through SPI to the display.

---

## 6A. Renderer math (detailed)

This section describes formulas and operations used by the code in more detail.

### 6A.1. Fixed-point (Q20.12)

The project uses fixed-point number representation:
- `SHIFT_FACTOR = 12`
- `SCALE_FACTOR = 1 << 12 = 4096`

Meaning:
- `1.0` is `4096`
- `0.5` is `2048`

Basic operations:
- `fixed = real * 4096`
- `real = fixed / 4096`
- `fixed_mul(a,b) ~= (a*b) >> 12`
- `fixed_div(a,b) ~= (a<<12) / b`

Why this:
- on Pico, this is usually faster and more predictable than using full `float` everywhere,
- easier to control CPU and memory cost.

### 6A.2. Rotation (quaternions)

In `rotate(...)` rotation is computed using quaternions:
- `theta = w * 2*pi`
- `q = [cos(theta/2), axis * sin(theta/2)]`
- result: `v' = q * v * q^-1`

Rotation axis (`axis`) is normalized before use.

### 6A.3. View matrix (camera)

In `camera.c`:
- `forward = normalize(pos - target)`
- `right = normalize(up x forward)` (in code via `mul_vectors`)
- `up = normalize(forward x right)`

Then the view matrix is built:
- camera axes go into rotational part,
- translation is `-dot(pos, axis)`.

This gives classic world -> camera-space transform.

### 6A.4. Perspective projection matrix

Code uses perspective form with `znear`, `zfar`, `aspect`, `tan(fov/2)`.

Logical formula is classic:
- `p00 = 1/(tan(fov/2)*aspect)`
- `p11 = 1/tan(fov/2)`
- `p22 = -(zf+zn)/(zf-zn)`
- `p23 = -1`
- `p32 = -(2*zf*zn)/(zf-zn)`

In code, everything is in fixed-point.

### 6A.5. Clip-space and near clipping

After `vMatrix` and `pMatrix`, each vertex has `(x,y,z,w)` in clip-space.

Near-plane inside condition:
- point is "inside" when `z > 0`.

For an edge crossing near-plane, intersection point is computed:
- `t = (znear - za)/(zb - za)` where `znear = 1` (in fixed),
- linear interpolation:
  `P = A + t*(B-A)`.

Interpolated at the same time:
- `x,y,z,w`,
- `uvx,uvy`,
- `light`.

Clipping algorithm works like Sutherland-Hodgman for one triangle:
- input: 3 vertices,
- output: 0..4 vertices,
- if 4 vertices, it is split into 2 triangles (triangle fan).

### 6A.6. Perspective divide and screen transform

For each vertex after clipping:
- `xs = x / w`
- `ys = y / w`

Then shift to render-target center:
- `screen_x = xs + render_width_half`
- `screen_y = ys + render_height_half`

### 6A.7. Back-face culling

In 2D, sign of "double area" is computed:

`area2 = (bx-ax)*(cy-ay) - (by-ay)*(cx-ax)`

In code, triangle is drawn when `area2 >= 0`.

### 6A.8. Triangle sorting (instead of z-buffer)

Each triangle gets depth key:

`depth = (za + zb + zc)/3`

Then `qsort` orders triangles from far to near (painter's algorithm).

### 6A.9. Scanline lerp in rasterization (no barycentrics)

Rasterization now works as:
- `tri(...)` sorts vertices by `y` and splits triangle into upper/lower part,
- both active edges are advanced incrementally in `x` (accumulators `q`, `q2`),
- together with `x`, edge attributes are advanced: `L`, `U`, `V`, `W`,
- `rasterize(...)` gets both scanline endpoints and computes x-steps: `dLdx`, `dUdx`, `dVdx`, `dWdx`.

This removes per-pixel barycentric evaluation.

Interpolation precision is boosted with:
- `LIGHT_LERP_SHIFT` for light,
- `UV_LERP_SHIFT` for `U` and `V`.

### 6A.10. Perspective-correct UV and texture sampling

In `render_scene(...)`, prepared values are:
- `W = 1/z`,
- `U = u * W`,
- `V = v * W`.

Rasterization interpolates `U`, `V`, `W`.
At pixel stage, proper UV is reconstructed:
- `u = U / W`
- `v = V / W`

Because `U` and `V` run with extra precision (`UV_LERP_SHIFT`),
they are shifted back before `texturing(...)` (`U >> UV_LERP_SHIFT`, `V >> UV_LERP_SHIFT`).

Then:
- multiply by texture size,
- clamp to `[1, size-2]`.

Sampling:
- 4 texels are fetched (`c00,c10,c01,c11`),
- result is 2x2 channel average (simple box filter).

This is not full bilinear with fractional UV-dependent weights; it is equal averaging of 4 samples.

### 6A.11. Lighting

First (geometry stage), per vertex:
- `N = normalize(normal)`
- `Ldir = normalize(lightPos - vertexPos)`
- `Li = clamp(dot(N, Ldir), 0, 1)`

Then (pixel stage):
- `L` is interpolated scanline-wise (`dL01`, `dL02`, `dL12`, then `dLdx`),
- internally with extra precision via `LIGHT_LERP_SHIFT`,
- `shading(...)` receives `L >> LIGHT_LERP_SHIFT`.

Then:
- multiply material color by light color (RGB565 per channel),
- multiply by light intensity,
- extra safety clamps (`INTENSITY_MAX`, `MAX_LIGHT_FACTOR`),
- final clamp to RGB565 ranges (`R:0..31`, `G:0..63`, `B:0..31`).

### 6A.12. Output scaling

Renderer can work at lower internal resolution (`render_scale > 1`).

Each computed pixel is then written as a block:
- block size: `output_scale x output_scale`
- this gives cheap upscale to LCD.

### 6A.13. Important mathematical consequences of current approach

- UVs are perspective-correct (`u/z`, `v/z`, `1/z`), so texture "swimming" is much lower than with affine interpolation.
- Fixed-point quantization is still present (`UV_LERP_SHIFT`, `LIGHT_LERP_SHIFT`), so very sharp gradients can still show subtle bands/seams.
- Triangle sorting by average `z` does not perfectly solve mutually intersecting geometry cases.
- Clipping is currently near-plane only, not all 6 frustum planes.
- Code is intentionally optimized for compromise: quality vs CPU/RAM cost on Pico 2.

---

## 7. Why no z-buffer?

In this project z-buffer was tested, but:
- there were artifacts (image looked noisy/unstable),
- memory cost was too high for project constraints.

So currently triangle sorting is used.

Downside: with intersecting objects, draw-order artifacts can appear.

---

## 8. What has already been improved recently?

1. Per-frame allocations were reduced:
   - scratch buffers are reused.
2. Near-plane clipping was improved:
   - triangles crossing near plane do not disappear as a whole,
   - they are clipped and triangulated.
3. Rasterization moved to full scanline lerp:
   - no per-pixel barycentrics,
   - UV is carried as `U/V/W` (perspective-correct),
   - light and UV use dedicated precision shifts.

This improves rendering stability and reduces memory churn.

---

## 9. What is intentionally "simple" / still incomplete?

This is not an AAA engine, but a conscious Pico + demoscene compromise.

Current limitations:
- no z-buffer,
- no full clipping on all frustum planes,
- scene triangle limit (for memory reasons),
- small test asset set,
- not all modules are used in `main` (intentional, because focus is on 3D renderer quality).

---

## 10. Easiest way to read this code as a new person

Best order:

1. `Euzebia3D.c`
   See frame loop.
2. `libs/renderer/IRenderer.h`
   See renderer API.
3. `libs/renderer/renderer.c`
   Read in order: `add_model_to_scene` -> `render_scene` -> `tri` -> `rasterize`.
4. `libs/cameraFactory/camera.c`
   Understand where matrices come from.
5. `libs/painter/painter.c`
   See how pixel reaches LCD.

---

## 11. One-frame pseudocode (super simple)

```text
while (true):
    update object and camera animations
    update_camera()

    clean_scene()

    for each mesh:
        transform vertices
        compute clip-space
        clip triangles to near plane
        reject back-face
        add triangles to scene[]

    sort scene[] by depth

    for each triangle in scene[]:
        rasterize
        for each pixel:
            fetch color (texture/flat)
            compute light
            write to framebuffer

    send framebuffer to LCD via DMA
    clear framebuffer
```

---

## 12. Glossary (non-academic wording)

- **Vertex**: point in 3D.
- **Triangle**: 3 vertices, smallest part of a model.
- **UV**: coordinates that tell where to fetch color from texture.
- **Normal**: vector that tells "which way the surface faces".
- **Clipping**: cutting geometry to visible area.
- **Culling**: rejecting triangles that would not be visible anyway.
- **Rasterization**: converting a triangle into pixels.
- **Framebuffer**: image buffer in RAM.
- **DMA**: hardware transfer without CPU handling each copy operation.

---

## 13. One-sentence summary

This project is a lightweight software 3D renderer for Pico 2: model -> transformations -> clipping -> triangle sorting -> rasterization -> buffer -> DMA to LCD.
