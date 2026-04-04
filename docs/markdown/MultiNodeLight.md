# MultiNodeLight

**Inherits:** MultiNodeVisual3D

Dynamic omni or spot lights for each instance.

## Description

MultiNodeLight creates a RenderingServer light for each active instance in the parent MultiNode. Supports omni (point) and spot light types. Use cases include headlights, warning beacons, task lights, and streetlamps. Light count should be kept reasonable -- use instance_every to reduce the number of active lights for performance.

## Properties

| Name | Type | Default |
|------|------|---------|
| [light_type](#light_type) | LightType | `LIGHT_OMNI (0)` |
| [light_color](#light_color) | Color | `Color(1, 1, 1, 1)` |
| [light_energy](#light_energy) | float | `1.0` |
| [light_range](#light_range) | float | `10.0` |
| [spot_angle](#spot_angle) | float | `45.0` |
| [shadow_enabled](#shadow_enabled) | bool | `false` |

## Property Descriptions

### light_type

- **Type:** LightType
- **Default:** `LIGHT_OMNI (0)`
- **Setter:** `set_light_type(value)`
- **Getter:** `get_light_type()`

The type of light created for each instance. Omni = point light radiating in all directions. Spot = directional cone light aimed along the instance's forward axis.

---

### light_color

- **Type:** Color
- **Default:** `Color(1, 1, 1, 1)`
- **Setter:** `set_light_color(value)`
- **Getter:** `get_light_color()`

The base color of all lights. Per-instance color overrides are applied via `set_instance_color()` from the MultiNodeVisual3D base class.

---

### light_energy

- **Type:** float
- **Default:** `1.0`
- **Setter:** `set_light_energy(value)`
- **Getter:** `get_light_energy()`

The brightness multiplier for all lights. Higher values produce brighter illumination. Per-instance energy can be set via `set_instance_energy()`.

---

### light_range

- **Type:** float
- **Default:** `10.0`
- **Setter:** `set_light_range(value)`
- **Getter:** `get_light_range()`

The maximum distance in meters that each light illuminates. Surfaces beyond this range receive no light.

---

### spot_angle

- **Type:** float
- **Default:** `45.0`
- **Setter:** `set_spot_angle(value)`
- **Getter:** `get_spot_angle()`

The half-angle of the spot light cone in degrees. Only used when light_type is LIGHT_SPOT. Larger angles produce a wider cone.

---

### shadow_enabled

- **Type:** bool
- **Default:** `false`
- **Setter:** `set_shadow_enabled(value)`
- **Getter:** `get_shadow_enabled()`

When enabled, each light casts shadows. This is expensive -- each shadow-casting light requires a separate shadow map render pass. Use sparingly with large instance counts.

## Methods

| Return | Name |
|--------|------|
| void | [set_instance_energy](#set_instance_energy)(index: int, energy: float) |

## Method Descriptions

### set_instance_energy

```
void set_instance_energy(index: int, energy: float)
```

Override the brightness for a single instance's light. Use for flickering effects, dimming, or per-instance intensity variation.

## Enumerations

### enum LightType

| Constant | Value | Description |
|----------|-------|-------------|
| LIGHT_OMNI | `0` | Point light radiating in all directions. |
| LIGHT_SPOT | `1` | Directional cone light. |
