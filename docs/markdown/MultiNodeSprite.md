# MultiNodeSprite

**Inherits:** MultiNodeVisual3D

Billboard sprites with sprite sheet animation for all instances in one draw call.

## Description

MultiNodeSprite renders billboard (or flat) sprites for each instance using a single MultiMesh draw call. Supports sprite sheet grids (hframes/vframes) with per-instance frame selection for walk cycles and animation states. Integrates with MultiNodeAnimator for automatic frame animation. Per-instance color modulation is supported.

## Properties

| Name | Type | Default |
|------|------|---------|
| [texture](#texture) | Texture2D | `null` |
| [pixel_size](#pixel_size) | float | `0.01` |
| [hframes](#hframes) | int | `1` |
| [vframes](#vframes) | int | `1` |
| [billboard](#billboard) | bool | `true` |
| [pixel_art](#pixel_art) | bool | `true` |

## Property Descriptions

### texture

- **Type:** Texture2D
- **Default:** `null`
- **Setter:** `set_texture(value)`
- **Getter:** `get_texture()`

The sprite sheet texture. Can be a single sprite or a grid of frames defined by hframes and vframes. All instances share this texture.

---

### pixel_size

- **Type:** float
- **Default:** `0.01`
- **Setter:** `set_pixel_size(value)`
- **Getter:** `get_pixel_size()`

World-space size of each texture pixel in meters. Controls how large the sprite appears in the 3D scene. The quad is sized to match one frame's pixel dimensions multiplied by this value.

---

### hframes

- **Type:** int
- **Default:** `1`
- **Setter:** `set_hframes(value)`
- **Getter:** `get_hframes()`

Number of horizontal frames (columns) in the sprite sheet grid. Set to 1 for a single-frame sprite.

---

### vframes

- **Type:** int
- **Default:** `1`
- **Setter:** `set_vframes(value)`
- **Getter:** `get_vframes()`

Number of vertical frames (rows) in the sprite sheet grid. Total frames available = hframes * vframes.

---

### billboard

- **Type:** bool
- **Default:** `true`
- **Setter:** `set_billboard(value)`
- **Getter:** `get_billboard()`

When enabled, sprites always face the camera (Y-locked billboard). When disabled, sprites are flat quads oriented by their instance transform.

---

### pixel_art

- **Type:** bool
- **Default:** `true`
- **Setter:** `set_pixel_art(value)`
- **Getter:** `get_pixel_art()`

When enabled, uses nearest-neighbor filtering for crisp pixel art. When disabled, uses linear mipmapped filtering for smooth scaling. Rebuilds the shader when changed.

## Methods

| Return | Name |
|--------|------|
| void | [set_instance_frame](#set_instance_frame)(index: int, frame: int) |
| int | [get_instance_frame](#get_instance_frame)(index: int) |

## Method Descriptions

### set_instance_frame

```
void set_instance_frame(index: int, frame: int)
```

Set the sprite sheet frame for a specific instance. Frame 0 is the top-left cell. Frames count left-to-right, top-to-bottom through the hframes * vframes grid.

---

### get_instance_frame

```
int get_instance_frame(index: int)
```

Get the current sprite sheet frame for a specific instance.
