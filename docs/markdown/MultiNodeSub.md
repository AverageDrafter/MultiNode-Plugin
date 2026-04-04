# MultiNodeSub

**Inherits:** Node3D

Base class for all MultiNode child nodes. Handles parent discovery, active filtering, per-instance data, and compute shaders.

## Description

MultiNodeSub is the base for every child that attaches to a MultiNode parent. It manages which instances this node processes via two filtering layers (`instance_every` and `instance_range`) ANDed together, owns a per-instance float data array, and can optionally run a GLSL compute shader each frame. Must be a direct child of a MultiNode to function.

Active filtering uses two layers combined with AND logic:

- **instance_every**: Coarse pattern filter (every Nth instance).
- **instance_range**: Excel-style range string for fine-grained inclusion/exclusion.

An instance is active only if it passes both filters. For runtime overrides beyond these two layers, use `set_instance_active()` to directly manipulate the active array.

## Properties

| Name | Type | Default |
|------|------|---------|
| [active_count](#active_count) | int | `0` |
| [instance_every](#instance_every) | int | `1` |
| [instance_range](#instance_range) | String | `""` |
| [compute_code](#compute_code) | String | `""` |
| [compute_enabled](#compute_enabled) | bool | `false` |

## Property Descriptions

### active_count

- **Type:** int
- **Default:** `0`
- **Getter:** `get_active_count()`

Read-only. The number of instances currently active on this node after applying all filters.

---

### instance_every

- **Type:** int
- **Default:** `1`
- **Setter:** `set_instance_every(value)`
- **Getter:** `get_instance_every()`

Controls which instances this node processes. `1` = all instances. `2` = every other instance (0, 2, 4...). `3` = every third (0, 3, 6...). Negative values select the complement: `-2` processes the instances that `+2` skips (1, 3, 5...). `0` = disabled (no instances processed). Use paired positive/negative values on two nodes to split work across LOD levels.

---

### instance_range

- **Type:** String
- **Default:** `""`
- **Setter:** `set_instance_range(value)`
- **Getter:** `get_instance_range()`

Excel-style range string for fine-grained instance filtering. Tokens are comma-separated and processed left-to-right. Examples:

- `0:99` -- include indices 0 through 99
- `1:10,15,20:30` -- include 1-10, 15, and 20-30
- `-5` -- everything except index 5
- `0:99,-50:59` -- indices 0-99 except 50-59
- `1:10,-3,17:20,-18` -- 1-10 (minus 3), plus 17-20 (minus 18)

Empty string means no restriction (all instances pass).

---

### compute_code

- **Type:** String
- **Default:** `""`
- **Setter:** `set_compute_code(value)`
- **Getter:** `get_compute_code()`

GLSL compute shader source code. Runs once per instance per frame on a local RenderingDevice. The shader receives:

- **binding 0:** transforms (3 x vec4 per instance, read/write)
- **binding 1:** active flags (uint per instance, read)
- **binding 2:** instance data (sub_data_stride floats per instance, read/write)

Push constants: `delta` (float), `time` (float), `count` (uint), `stride` (uint).

---

### compute_enabled

- **Type:** bool
- **Default:** `false`
- **Setter:** `set_compute_enabled(value)`
- **Getter:** `get_compute_enabled()`

Enable or disable the compute shader dispatch. The shader compiles when this is enabled and `compute_code` is set. Disable to stop GPU dispatch without losing the shader code.

## Methods

| Return | Name |
|--------|------|
| void | [set_instance_active](#set_instance_active)(index: int, active: bool) |
| bool | [is_instance_active](#is_instance_active)(index: int) |
| void | [set_all_active](#set_all_active)(active: bool) |
| int | [get_active_count](#get_active_count)() |
| MultiNode | [get_multi_node_parent](#get_multi_node_parent)() |
| int | [get_data_stride](#get_data_stride)() |
| void | [set_instance_data](#set_instance_data)(index: int, channel: int, value: float) |
| float | [get_instance_data](#get_instance_data)(index: int, channel: int) |
| void | [set_instance_data_vec](#set_instance_data_vec)(index: int, values: PackedFloat32Array) |
| PackedFloat32Array | [get_instance_data_vec](#get_instance_data_vec)(index: int) |

## Method Descriptions

### set_instance_active

```
void set_instance_active(index: int, active: bool)
```

Directly set the active state of a single instance. Writes to the computed active array, bypassing the `instance_every` and `instance_range` filters. Note: the next call to `set_instance_every()` or `set_instance_range()` will recompute and overwrite this override.

---

### is_instance_active

```
bool is_instance_active(index: int)
```

Check whether a specific instance is active on this node (after all filters).

---

### set_all_active

```
void set_all_active(active: bool)
```

Set all instances to active or inactive at once. Directly writes to the active array, bypassing filters.

---

### get_active_count

```
int get_active_count()
```

Returns the number of currently active instances on this node.

---

### get_multi_node_parent

```
MultiNode get_multi_node_parent()
```

Returns the parent MultiNode, or `null` if this node is not a child of one.

---

### get_data_stride

```
int get_data_stride()
```

Returns this node's per-instance data stride (floats per instance). Inherited from the parent MultiNode's `sub_data_stride` setting.

---

### set_instance_data

```
void set_instance_data(index: int, channel: int, value: float)
```

Set a single float in this node's per-instance data array. `channel` is the offset (`0` to `data_stride - 1`). This data is private to this sub-node -- not shared with siblings.

---

### get_instance_data

```
float get_instance_data(index: int, channel: int)
```

Read a single float from this node's per-instance data array.

---

### set_instance_data_vec

```
void set_instance_data_vec(index: int, values: PackedFloat32Array)
```

Set the entire data vector for a single instance on this node.

---

### get_instance_data_vec

```
PackedFloat32Array get_instance_data_vec(index: int)
```

Get the full data vector for a single instance on this node.
