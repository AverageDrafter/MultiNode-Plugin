# MultiNodeVisual3D

**Inherits:** MultiNode3D

Base class for visual MultiNode children. Manages shadow casting, render layers, visibility range, and per-instance color tinting.

## Description

MultiNodeVisual3D is the base for all rendering-related sub-nodes (Mesh, Label, Sprite, Particle, Decal, Light). It owns shared visual properties pushed to the RenderingServer -- shadow casting, render layers, visibility range -- and a per-instance color tint array. Subclasses create the actual visual instances and call `_apply_visual_properties()` to push these settings.

## Properties

| Name | Type | Default |
|------|------|---------|
| [cast_shadow](#cast_shadow) | ShadowCastingSetting | `SHADOW_CASTING_ON (1)` |
| [render_layers](#render_layers) | int | `1` |
| [visibility_range_begin](#visibility_range_begin) | float | `0.0` |
| [visibility_range_end](#visibility_range_end) | float | `0.0` |
| [tint](#tint) | Color | `Color(1, 1, 1, 1)` |

## Property Descriptions

### cast_shadow

- **Type:** int (ShadowCastingSetting)
- **Default:** `1` (`SHADOW_CASTING_ON`)
- **Setter:** `set_cast_shadow(value)`
- **Getter:** `get_cast_shadow()`

Controls shadow casting for all instances rendered by this node. `Off` = no shadows. `On` = standard shadows. `Double-Sided` = shadows from both sides of faces. `Shadows Only` = invisible but still casts shadows.

---

### render_layers

- **Type:** int
- **Default:** `1`
- **Setter:** `set_render_layers(value)`
- **Getter:** `get_render_layers()`

The render layer bitmask for all instances on this node. Only cameras with matching `cull_mask` bits will see these instances. Use to separate visual layers (e.g., minimap, main view).

---

### visibility_range_begin

- **Type:** float
- **Default:** `0.0`
- **Setter:** `set_visibility_range_begin(value)`
- **Getter:** `get_visibility_range_begin()`

Minimum distance from the camera at which instances become visible. Set to `0` to show at all close distances. Use with `visibility_range_end` for LOD-style distance culling.

---

### visibility_range_end

- **Type:** float
- **Default:** `0.0`
- **Setter:** `set_visibility_range_end(value)`
- **Getter:** `get_visibility_range_end()`

Maximum distance from the camera at which instances are visible. Set to `0` to disable distance culling. Instances farther than this are hidden.

---

### tint

- **Type:** Color
- **Default:** `Color(1, 1, 1, 1)`
- **Setter:** `set_tint(value)`
- **Getter:** `get_tint()`

Color filter applied to all instances on this node. White means no effect. Multiplied with per-instance colors to produce the final color. Useful for global effects like damage flash or team coloring.

## Methods

| Return | Name |
|--------|------|
| void | [set_instance_color](#set_instance_color)(index: int, color: Color) |
| Color | [get_instance_color](#get_instance_color)(index: int) |
| void | [set_all_colors](#set_all_colors)(colors: PackedColorArray) |
| void | [resize_colors](#resize_colors)(count: int) |

## Method Descriptions

### set_instance_color

```
void set_instance_color(index: int, color: Color)
```

Set the color for a single instance. Multiplied by the node `tint` to produce the final color. White means the instance uses only the node tint.

---

### get_instance_color

```
Color get_instance_color(index: int)
```

Get the raw per-instance color (before tint multiplication).

---

### set_all_colors

```
void set_all_colors(colors: PackedColorArray)
```

Replace all per-instance colors at once. Triggers a bulk update to the rendering backend.

---

### resize_colors

```
void resize_colors(count: int)
```

Resize the per-instance color array to match the given count. New entries are filled with white (`Color(1, 1, 1, 1)`). Typically called internally when the instance count changes.

## Enumerations

### enum ShadowCastingSetting

| Name | Value | Description |
|------|-------|-------------|
| SHADOW_CASTING_OFF | `0` | No shadow casting. |
| SHADOW_CASTING_ON | `1` | Standard shadow casting. |
| SHADOW_CASTING_DOUBLE_SIDED | `2` | Cast shadows from both sides of faces. |
| SHADOW_CASTING_SHADOWS_ONLY | `3` | Invisible but still casts shadows. |
