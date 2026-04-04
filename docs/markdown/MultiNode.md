# MultiNode

**Inherits:** Node3D

Compositional instancing parent that manages a shared list of instance transforms and data.

## Description

MultiNode is the root of a compositional instancing system. It owns the master instance list -- transforms, dirty flags, and per-instance data -- shared by all child nodes. Add MultiNodeSub-derived children (Mesh, Collider, Audio, etc.) to define what each instance is and does. One MultiNode with sub-nodes replaces thousands of individual scene nodes.

## Properties

| Name | Type | Default |
|------|------|---------|
| [use_transforms](#use_transforms) | bool | `true` |
| [instance_count](#instance_count) | int | `0` |
| [instance_limit](#instance_limit) | int | `0` |
| [data_stride](#data_stride) | int | `4` |
| [sub_data_stride](#sub_data_stride) | int | `4` |

## Property Descriptions

### use_transforms

- **Type:** bool
- **Default:** `true`
- **Setter:** `set_use_transforms(value)`
- **Getter:** `get_use_transforms()`

When enabled, the parent stores a Transform3D per instance. Disable to save memory when you only need data arrays and compute shaders without spatial positioning. Sub-nodes that require transforms (MultiNode3D and below) will not function when disabled.

---

### instance_count

- **Type:** int
- **Default:** `0`
- **Setter:** `set_instance_count(value)`
- **Getter:** `get_instance_count()`

The current number of instances. Set this directly to resize the instance list. Triggers a full sync to all child sub-nodes. Not exposed in the inspector -- use `add_instance()` and `remove_instance()` from scripts.

---

### instance_limit

- **Type:** int
- **Default:** `0`
- **Setter:** `set_instance_limit(value)`
- **Getter:** `get_instance_limit()`

Maximum number of instances allowed. Set to `0` for unlimited. `add_instance()` returns `-1` when the limit is reached. If the current count exceeds a newly set limit, instances are trimmed.

---

### data_stride

- **Type:** int
- **Default:** `4`
- **Setter:** `set_data_stride(value)`
- **Getter:** `get_data_stride()`

Number of floats stored per instance in the parent-level data array. Use for shared game state accessible from all sub-nodes and GDScript. Minimum 1, maximum 64.

---

### sub_data_stride

- **Type:** int
- **Default:** `4`
- **Setter:** `set_sub_data_stride(value)`
- **Getter:** `get_sub_data_stride()`

Number of floats per instance in each sub-node's private data array. Must be a multiple of 4 for GPU alignment (automatically rounded up). Controls the stride for all sub-node data arrays and compute shader binding 2.

## Methods

| Return | Name |
|--------|------|
| int | [add_instance](#add_instance)(transform: Transform3D = Transform3D()) |
| void | [remove_instance](#remove_instance)(index: int) |
| void | [clear_instances](#clear_instances)() |
| void | [set_instance_transform](#set_instance_transform)(index: int, transform: Transform3D) |
| Transform3D | [get_instance_transform](#get_instance_transform)(index: int) |
| void | [set_all_transforms](#set_all_transforms)(transforms: Array) |
| void | [set_positions](#set_positions)(positions: PackedVector3Array) |
| Array | [get_transform_buffer](#get_transform_buffer)() |
| void | [mark_dirty](#mark_dirty)(index: int) |
| void | [mark_all_dirty](#mark_all_dirty)() |
| bool | [is_dirty](#is_dirty)(index: int) |
| int | [get_dirty_count](#get_dirty_count)() |
| void | [begin_batch](#begin_batch)() |
| void | [end_batch](#end_batch)() |
| void | [notify_transforms_changed](#notify_transforms_changed)() |
| void | [set_instance_data](#set_instance_data)(index: int, channel: int, value: float) |
| float | [get_instance_data](#get_instance_data)(index: int, channel: int) |
| void | [set_instance_data_vec](#set_instance_data_vec)(index: int, values: PackedFloat32Array) |
| PackedFloat32Array | [get_instance_data_vec](#get_instance_data_vec)(index: int) |

## Method Descriptions

### add_instance

```
int add_instance(transform: Transform3D = Transform3D())
```

Add a new instance with the given transform. Returns the new instance index, or `-1` if the `instance_limit` has been reached.

---

### remove_instance

```
void remove_instance(index: int)
```

Remove the instance at the given index. Uses swap-and-pop internally -- the last instance moves into the removed slot, so indices may change.

---

### clear_instances

```
void clear_instances()
```

Remove all instances. Resets the count to zero and frees all internal buffers.

---

### set_instance_transform

```
void set_instance_transform(index: int, transform: Transform3D)
```

Set the transform for a single instance. Marks it dirty and notifies children at end of frame (or at `end_batch()` if batching).

---

### get_instance_transform

```
Transform3D get_instance_transform(index: int)
```

Get the current transform for an instance.

---

### set_all_transforms

```
void set_all_transforms(transforms: Array)
```

Replace all instance transforms at once. Resizes the instance count to match the array size.

---

### set_positions

```
void set_positions(positions: PackedVector3Array)
```

Convenience method that sets identity-basis transforms from an array of positions. Resizes the instance count to match.

---

### get_transform_buffer

```
Array get_transform_buffer()
```

Returns all instance transforms as an array. Useful for serialization or bulk reads.

---

### mark_dirty

```
void mark_dirty(index: int)
```

Manually mark a single instance as dirty. Children will update it on next sync. Normally called automatically by `set_instance_transform()`.

---

### mark_all_dirty

```
void mark_all_dirty()
```

Mark all instances as dirty. Forces a full re-sync of all children on the next signal.

---

### is_dirty

```
bool is_dirty(index: int)
```

Check whether an instance is marked dirty since the last sync.

---

### get_dirty_count

```
int get_dirty_count()
```

Returns the number of currently dirty instances. Children use this to decide between sparse and full updates.

---

### begin_batch

```
void begin_batch()
```

Enter batch mode. Transform changes are accumulated silently without emitting signals. Call `end_batch()` when done to emit a single `instances_changed` signal.

---

### end_batch

```
void end_batch()
```

Exit batch mode and emit a single `instances_changed` signal for all accumulated changes.

---

### notify_transforms_changed

```
void notify_transforms_changed()
```

Manually trigger an `instances_changed` signal. Use when you modify transforms via direct buffer access from C++ and need children to update.

---

### set_instance_data

```
void set_instance_data(index: int, channel: int, value: float)
```

Set a single float in the parent-level per-instance data array. `channel` is the offset within the instance's data (`0` to `data_stride - 1`).

---

### get_instance_data

```
float get_instance_data(index: int, channel: int)
```

Read a single float from the parent-level per-instance data array.

---

### set_instance_data_vec

```
void set_instance_data_vec(index: int, values: PackedFloat32Array)
```

Set the entire data vector for a single instance. The array is clamped to `data_stride` length.

---

### get_instance_data_vec

```
PackedFloat32Array get_instance_data_vec(index: int)
```

Get the full data vector for a single instance as a PackedFloat32Array.

## Signals

### instances_changed

```
instances_changed()
```

Emitted when the instance list changes -- count, transforms, or data. All child sub-nodes listen to this signal to stay in sync.
