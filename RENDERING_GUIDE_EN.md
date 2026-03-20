# Euzebia3D - Rendering Explained (EN)

This document is for a person who:
- receives the project from someone else,
- has never programmed in C for Raspberry Pi Pico,
- wants to understand "what happens from a 3D model to a pixel on the LCD".

You do not need deep 3D math knowledge to follow this.

---

## 1. What does this project do?

The project renders a 3D scene in software (CPU), without a GPU.

In short, it:
1. takes a 3D model (vertices + triangles),
2. rotates/moves it,
3. projects it to a 2D screen,
4. converts triangles into pixels,
5. sends the whole frame to the LCD via DMA.

Right now, the project uses **triangle sorting** (painter's algorithm), not a z-buffer.

---

## 2. Most important files (project map)

- `Euzebia3D.c`
  Main program loop, shows what happens each frame.
- `libs/renderer/renderer.c`
  Core 3D renderer: clipping, culling, sorting, rasterization, shading, texturing.
- `libs/cameraFactory/camera.c`
  Camera: view matrix (`vMatrix`) and projection matrix (`pMatrix`).
- `libs/meshFactory/meshFactory.c`
  Creates Mesh objects from embedded project data.
- `libs/painter/painter.c`
  Framebuffer + DMA transfer to LCD.
- `libs/display/display.c`
  LCD initialization.
- `libs/hardware/hardware.c`
  SPI, GPIO, PWM, etc. initialization.
- `libs/storage/gfx.c`
  Models and textures embedded in firmware.

---

## 3. Program startup (step by step)

In `main()` (`Euzebia3D.c`):

1. Pico clock is set to 300 MHz.
2. Hardware layer starts (`init_hardware`).
3. LCD starts (`init_display`).
4. Painter starts (buffer + DMA).
5. Renderer starts (`init_renderer`) and render scale is set (`set_scale(1)`).
6. Scene objects are created:
   - 2 meshes (currently 2 mugs),
   - 1 point light,
   - 1 camera.
7. Program enters `while(1)` - frame rendering loop.

---

## 4. What happens every frame?

For each loop iteration:

1. Transformations are updated (e.g., object rotation, camera movement).
2. Camera is updated (`update_camera`) - matrices are recalculated.
3. Renderer clears scene triangle list (`clean_scene`).
4. Each model is added to scene (`add_model_to_scene`).
5. Renderer draws the full scene (`render_scene`).
6. Painter sends final buffer to LCD (`draw_buffer`).
7. Buffer is cleared with background color for next frame.

Simple view: **compute -> draw -> send -> clear -> repeat**.

---

## 5. What are Mesh, Material, Triangle?

### Mesh
A Mesh is a 3D object:
- vertex list,
- triangle list (faces),
- UV coordinates,
- normals,
- material.

### Material
Material tells renderer:
- use texture (`texture` + `textureSize`) or not,
- use flat color (`diffuse`) or not,
- whether it is skybox (`isSkyBox`).

### Triangle
Renderer finally works with triangles.
Each triangle has:
- 3 points,
- UV,
- lighting data.

---

## 6. Most important part: 3D rendering pipeline

Main path "from model to pixels":

### Stage A: model transformations

For each model:
- source data is copied into working buffers,
- transformations are applied (rotation/translation/scale).

Done in `add_model_to_scene`.

### Stage B: camera (view + projection)

Each vertex goes through:
1. view matrix (`vMatrix`) - "how camera looks",
2. projection matrix (`pMatrix`) - "how 3D maps to 2D".

After this, vertex is in clip-space (`x, y, z, w`).

### Stage C: clipping (near plane)

This was recently improved.

Instead of dropping full triangles when partly behind camera:
- triangle is clipped against near plane (`z > 0`),
- new edge vertices are interpolated,
- UV and lighting are interpolated too.

Result: fewer disappearing triangles near camera.

### Stage D: triangulation after clipping

After clipping, polygon can have:
- 3 vertices -> keep 1 triangle,
- 4 vertices -> split into 2 triangles.

### Stage E: perspective divide

For each vertex:
- `screen_x = x / w`,
- `screen_y = y / w`.

This gives 2D screen position.

### Stage F: back-face culling

If triangle faces away from camera, it is not drawn.
This saves CPU time.

### Stage G: write triangles into scene list

Triangles are stored in static array `scene[]`.
There is a hard limit (`MAX_TRIANGLES_IN_SCENE = 1500`).

### Stage H: triangle sorting

Before drawing, triangles are sorted by depth (average `z`):
- far first,
- near last.

This replaces z-buffer.

### Stage I: rasterization (triangle -> pixels)

Each triangle is drawn scanline by scanline:
- line by line,
- pixel by pixel.

Barycentric coordinates (`Ba/Bb/Bc`) are used internally.

### Stage J: final pixel color

For each pixel:
1. Get base color (texture or flat color).
2. Apply shading:
   - interpolate vertex lighting,
   - clamp values to safe range.
3. Write RGB565 pixel into LCD buffer.

### Stage K: send full frame to LCD

`painter.draw_buffer()`:
- takes whole framebuffer,
- sends it via DMA in chunks through SPI to LCD.

---

## 7. Why no z-buffer?

In this project z-buffer was tested, but:
- it produced visible artifacts (noisy/unstable output),
- memory cost was too high for current constraints.

So the project currently uses triangle sorting.

Trade-off: intersecting objects can still show draw-order artifacts.

---

## 8. What was improved recently?

1. Per-frame allocations were reduced:
   - scratch buffers are reused.
2. Near-plane clipping was improved:
   - triangles crossing near plane are no longer dropped as a whole,
   - they are clipped and triangulated.

This improves rendering stability and reduces memory churn.

---

## 9. What is intentionally still simple / incomplete?

This is not an AAA engine, it is a deliberate Pico + demoscene compromise.

Current limitations:
- no z-buffer,
- no full clipping on all frustum planes,
- scene triangle limit (memory reasons),
- small test asset set,
- not all modules are used in current `main` (intentional: focus is 3D renderer quality).

---

## 10. Best reading order for a new person

Recommended order:

1. `Euzebia3D.c`
   Understand frame loop first.
2. `libs/renderer/IRenderer.h`
   See renderer API.
3. `libs/renderer/renderer.c`
   Read in this order: `add_model_to_scene` -> `render_scene` -> `tri` -> `rasterize`.
4. `libs/cameraFactory/camera.c`
   Understand where matrices come from.
5. `libs/painter/painter.c`
   See how pixels reach LCD.

---

## 11. One-frame pseudocode (very simple)

```text
while (true):
    update object and camera animation
    update_camera()

    clean_scene()

    for each mesh:
        transform vertices
        compute clip-space coordinates
        clip triangles against near plane
        reject back-faces
        append triangles to scene[]

    sort scene[] by depth

    for each triangle in scene[]:
        rasterize
        for each pixel:
            fetch base color (texture/flat)
            compute lighting
            write pixel to framebuffer

    send framebuffer to LCD via DMA
    clear framebuffer
```

---

## 12. Glossary (plain language)

- **Vertex**: a point in 3D.
- **Triangle**: 3 vertices, smallest renderable part of model.
- **UV**: coordinates that select color from texture.
- **Normal**: vector telling which direction a surface faces.
- **Clipping**: cutting geometry to visible area.
- **Culling**: skipping triangles that should not be visible.
- **Rasterization**: converting triangle into pixels.
- **Framebuffer**: image buffer in RAM.
- **DMA**: hardware data transfer without CPU copying every byte manually.

---

## 13. One-sentence summary

This project is a lightweight software 3D renderer for Pico 2: model -> transforms -> clipping -> triangle sorting -> rasterization -> framebuffer -> DMA to LCD.
