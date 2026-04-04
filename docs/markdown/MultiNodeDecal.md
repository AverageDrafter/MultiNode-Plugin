# MultiNodeDecal

**Inherits:** MultiNodeVisual3D

Projected decals for each instance, using RenderingServer decals.

## Description

MultiNodeDecal creates a RenderingServer decal for each active instance in the parent MultiNode. Decals project a texture onto nearby surfaces. Use cases include shadow blobs, tire marks, damage indicators, selection rings, and ground markers. Per-instance color modulation is supported via the tint system.

## Properties

| Name | Type | Default |
|------|------|---------|
| [texture_albedo](#texture_albedo) | Texture2D | `null` |
| [decal_size](#decal_size) | Vector3 | `Vector3(2, 2, 2)` |
| [albedo_mix](#albedo_mix) | float | `1.0` |
| [normal_fade](#normal_fade) | float | `0.3` |
| [upper_fade](#upper_fade) | float | `0.3` |
| [lower_fade](#lower_fade) | float | `0.3` |

## Property Descriptions

### texture_albedo

- **Type:** Texture2D
- **Default:** `null`
- **Setter:** `set_texture_albedo(value)`
- **Getter:** `get_texture_albedo()`

The texture projected by the decal onto surfaces. All instances share this texture.

---

### decal_size

- **Type:** Vector3
- **Default:** `Vector3(2, 2, 2)`
- **Setter:** `set_decal_size(value)`
- **Getter:** `get_decal_size()`

The size of the decal projection volume in meters (width, height, depth). The decal projects along the Y axis within this box.

---

### albedo_mix

- **Type:** float
- **Default:** `1.0`
- **Setter:** `set_albedo_mix(value)`
- **Getter:** `get_albedo_mix()`

How much the decal's albedo texture blends with the underlying surface. 0.0 = no effect, 1.0 = full decal color.

---

### normal_fade

- **Type:** float
- **Default:** `0.3`
- **Setter:** `set_normal_fade(value)`
- **Getter:** `get_normal_fade()`

Controls how the decal fades based on the angle between the decal projection and the surface normal. Higher values make the decal fade more on angled surfaces.

---

### upper_fade

- **Type:** float
- **Default:** `0.3`
- **Setter:** `set_upper_fade(value)`
- **Getter:** `get_upper_fade()`

Controls the fade-out at the upper edge of the decal projection volume. Higher values produce a softer fade.

---

### lower_fade

- **Type:** float
- **Default:** `0.3`
- **Setter:** `set_lower_fade(value)`
- **Getter:** `get_lower_fade()`

Controls the fade-out at the lower edge of the decal projection volume. Higher values produce a softer fade.
