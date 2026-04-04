# MultiNodeInstanceData

**Inherits:** Resource

Serializable snapshot of instance transforms and optional custom data for MultiNode.

## Description

MultiNodeInstanceData stores an array of Transform3D and an optional flat PackedFloat32Array for per-instance custom data. Save as .tres or .res and assign to a MultiNodeInstancer's gen_instance_data property to load instances from a saved snapshot. Use the "Save Instance Data" button on MultiNodeInstancer to create snapshots.

## Properties

| Name | Type | Default |
|------|------|---------|
| [transforms](#transforms) | Array | `[]` |
| [custom_data](#custom_data) | PackedFloat32Array | `PackedFloat32Array()` |

## Property Descriptions

### transforms

- **Type:** Array
- **Default:** `[]`
- **Setter:** `set_transforms(value)`
- **Getter:** `get_transforms()`

Array of Transform3D, one per saved instance.

---

### custom_data

- **Type:** PackedFloat32Array
- **Default:** `PackedFloat32Array()`
- **Setter:** `set_custom_data(value)`
- **Getter:** `get_custom_data()`

Optional flat float array for per-instance custom data. Length should be instance_count * data_stride.

## Methods

| Return | Name |
|--------|------|
| int | [get_instance_count](#get_instance_count)() |

## Method Descriptions

### get_instance_count

```
int get_instance_count()
```

Returns the number of instances stored in this resource (transforms array size).
