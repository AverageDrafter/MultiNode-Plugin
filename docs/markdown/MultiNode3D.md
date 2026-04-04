# MultiNode3D

**Inherits:** MultiNodeSub

Base class for spatial MultiNode children. Adds a transform offset applied to each instance.

## Description

MultiNode3D extends MultiNodeSub with spatial awareness. It provides a configurable transform offset (position, rotation, scale) that is applied to every instance, and a flag to control whether instance rotation/scale is inherited. All concrete spatial sub-nodes (Mesh, Collider, Audio, etc.) inherit from this class. Requires the parent MultiNode to have `use_transforms` enabled.

## Properties

| Name | Type | Default |
|------|------|---------|
| [offset_position](#offset_position) | Vector3 | `Vector3(0, 0, 0)` |
| [offset_rotation](#offset_rotation) | Vector3 | `Vector3(0, 0, 0)` |
| [offset_scale](#offset_scale) | Vector3 | `Vector3(1, 1, 1)` |
| [use_instance_basis](#use_instance_basis) | bool | `true` |

## Property Descriptions

### offset_position

- **Type:** Vector3
- **Default:** `Vector3(0, 0, 0)`
- **Setter:** `set_offset_position(value)`
- **Getter:** `get_offset_position()`

Position offset applied to every instance, in local space. Use this to shift child geometry relative to the instance origin -- for example, raising a label above a character's head.

---

### offset_rotation

- **Type:** Vector3
- **Default:** `Vector3(0, 0, 0)`
- **Setter:** `set_offset_rotation(value)`
- **Getter:** `get_offset_rotation()`

Rotation offset applied to every instance, in radians (displayed as degrees in the inspector). Combines with `offset_position` and `offset_scale` to form the transform offset.

---

### offset_scale

- **Type:** Vector3
- **Default:** `Vector3(1, 1, 1)`
- **Setter:** `set_offset_scale(value)`
- **Getter:** `get_offset_scale()`

Scale offset applied to every instance. Multiplied into the transform offset along with position and rotation.

---

### use_instance_basis

- **Type:** bool
- **Default:** `true`
- **Setter:** `set_use_instance_basis(value)`
- **Getter:** `get_use_instance_basis()`

When enabled, each instance inherits the rotation and scale from its parent transform. When disabled, only the position is used -- useful for labels, lights, and other elements that should stay upright regardless of instance orientation.

## Methods

| Return | Name |
|--------|------|
| Transform3D | [get_transform_offset](#get_transform_offset)() |
| Transform3D | [compute_instance_transform](#compute_instance_transform)(instance_xform: Transform3D) |

## Method Descriptions

### get_transform_offset

```
Transform3D get_transform_offset()
```

Returns the combined transform offset built from `offset_position`, `offset_rotation`, and `offset_scale`.

---

### compute_instance_transform

```
Transform3D compute_instance_transform(instance_xform: Transform3D)
```

Compute the final transform for an instance by applying the offset and `use_instance_basis` flag to the given instance transform. Used internally by all spatial sub-nodes.
