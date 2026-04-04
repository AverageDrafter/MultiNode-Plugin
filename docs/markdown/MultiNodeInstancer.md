# MultiNodeInstancer

**Inherits:** MultiNodeSub

Composable instance generator. Creates and manages instance transforms for its parent MultiNode.

## Description

MultiNodeInstancer generates instance transforms using one of three modes (Array, Mesh Fill, or Manual) and writes them to the parent MultiNode's instance list. Add as a child of MultiNode -- multiple generators can coexist, each owning a contiguous slice of the parent's instances. Tree order determines stacking order.

Features:

- **Array mode**: Grid/chain patterns with configurable spacing, rotation, and scale stepping.
- **Mesh Fill mode**: Scatter instances across a mesh surface or fill its volume.
- **Manual mode**: Hand-place individual instances with inspector controls and viewport clicking.
- **Animate**: Regenerate every frame for runtime mesh-fill animation or dynamic arrays.
- **Exclusion**: Mark individual instances as "broken away" so the generator stops overwriting their transforms -- ideal for swarm breakaway mechanics.
- **Scatter**: Apply random jitter, rotation, and scale variation on top of any generation mode.

## Properties

| Name | Type | Default |
|------|------|---------|
| [gen_mode](#gen_mode) | GeneratorMode | `GEN_ARRAY (0)` |
| [gen_count](#gen_count) | int | `10` |
| [gen_seed](#gen_seed) | int | `0` |
| [gen_animate](#gen_animate) | bool | `false` |
| [gen_array_origin](#gen_array_origin) | Vector3 | `Vector3(0, 0, 0)` |
| [gen_array_spacing](#gen_array_spacing) | Vector3 | `Vector3(2, 2, 2)` |
| [gen_array_rows](#gen_array_rows) | int | `0` |
| [gen_array_cols](#gen_array_cols) | int | `0` |
| [gen_array_flip_layers](#gen_array_flip_layers) | bool | `false` |
| [gen_array_follow_rotation](#gen_array_follow_rotation) | bool | `false` |
| [gen_array_rotation](#gen_array_rotation) | Vector3 | `Vector3(0, 0, 0)` |
| [gen_array_scale_step](#gen_array_scale_step) | float | `1.0` |
| [gen_fill_mesh](#gen_fill_mesh) | Mesh | `null` |
| [gen_fill_mode](#gen_fill_mode) | FillMode | `FILL_SURFACE (0)` |
| [gen_fill_align](#gen_fill_align) | bool | `true` |
| [gen_fill_min_distance](#gen_fill_min_distance) | float | `0.0` |
| [gen_scatter_enabled](#gen_scatter_enabled) | bool | `false` |
| [gen_scatter_jitter](#gen_scatter_jitter) | Vector3 | `Vector3(0, 0, 0)` |
| [gen_scatter_random_rotation](#gen_scatter_random_rotation) | bool | `false` |
| [gen_scatter_scale_min](#gen_scatter_scale_min) | float | `1.0` |
| [gen_scatter_scale_max](#gen_scatter_scale_max) | float | `1.0` |
| [gen_instance_data](#gen_instance_data) | MultiNodeInstanceData | `null` |

## Property Descriptions

### gen_mode

- **Type:** GeneratorMode
- **Default:** `GEN_ARRAY (0)`
- **Setter:** `set_generator_mode(value)`
- **Getter:** `get_generator_mode()`

The generation algorithm. Array creates grid/chain patterns. Mesh Fill scatters on/in a mesh. Manual allows hand-placement of individual instances.

---

### gen_count

- **Type:** int
- **Default:** `10`
- **Setter:** `set_generator_count(value)`
- **Getter:** `get_generator_count()`

Number of instances to generate. Hidden in Manual mode.

---

### gen_seed

- **Type:** int
- **Default:** `0`
- **Setter:** `set_generator_seed(value)`
- **Getter:** `get_generator_seed()`

Random seed for reproducible generation. 0 = non-deterministic. Hidden in Manual mode.

---

### gen_animate

- **Type:** bool
- **Default:** `false`
- **Setter:** `set_animate(value)`
- **Getter:** `get_animate()`

When enabled, the generator regenerates every frame during _process. Use for animated mesh fills, dynamic array reshaping, or runtime swarm patterns. Hidden in Manual mode.

---

### gen_array_origin

- **Type:** Vector3
- **Default:** `Vector3(0, 0, 0)`
- **Setter:** `set_array_origin(value)`
- **Getter:** `get_array_origin()`

Starting position for array generation.

---

### gen_array_spacing

- **Type:** Vector3
- **Default:** `Vector3(2, 2, 2)`
- **Setter:** `set_array_spacing(value)`
- **Getter:** `get_array_spacing()`

Distance between instances along each axis.

---

### gen_array_rows

- **Type:** int
- **Default:** `0`
- **Setter:** `set_array_rows(value)`
- **Getter:** `get_array_rows()`

Number of instances per row. Negative reverses direction. 0 = single row (no wrapping).

---

### gen_array_cols

- **Type:** int
- **Default:** `0`
- **Setter:** `set_array_cols(value)`
- **Getter:** `get_array_cols()`

Number of columns before wrapping to a new layer. Negative reverses direction. 0 = single column.

---

### gen_array_flip_layers

- **Type:** bool
- **Default:** `false`
- **Setter:** `set_array_flip_layers(value)`
- **Getter:** `get_array_flip_layers()`

Reverses the Y-axis direction for layer stacking.

---

### gen_array_follow_rotation

- **Type:** bool
- **Default:** `false`
- **Setter:** `set_array_follow_rotation(value)`
- **Getter:** `get_array_follow_rotation()`

When enabled, each instance is a chained transformation of the previous one (like links in a chain). When disabled, rotation is cosmetic per-index scaling.

---

### gen_array_rotation

- **Type:** Vector3
- **Default:** `Vector3(0, 0, 0)`
- **Setter:** `set_array_rotation(value)`
- **Getter:** `get_array_rotation()`

Per-instance rotation in degrees. In chained mode, each instance rotates relative to the previous. In grid mode, rotation accumulates per index.

---

### gen_array_scale_step

- **Type:** float
- **Default:** `1.0`
- **Setter:** `set_array_scale_step(value)`
- **Getter:** `get_array_scale_step()`

Scale multiplier applied per instance. 1.0 = uniform size. Values like 0.95 create shrinking chains.

---

### gen_fill_mesh

- **Type:** Mesh
- **Default:** `null`
- **Setter:** `set_fill_mesh(value)`
- **Getter:** `get_fill_mesh()`

The mesh geometry to fill with instances. Required for Mesh Fill mode.

---

### gen_fill_mode

- **Type:** FillMode
- **Default:** `FILL_SURFACE (0)`
- **Setter:** `set_fill_mode(value)`
- **Getter:** `get_fill_mode()`

Cover Surface distributes instances across the mesh surface weighted by triangle area. Fill Volume places instances inside the mesh using rejection sampling.

---

### gen_fill_align

- **Type:** bool
- **Default:** `true`
- **Setter:** `set_fill_align(value)`
- **Getter:** `get_fill_align()`

Align instance orientation to the surface normal at the placement point. Only affects Cover Surface mode.

---

### gen_fill_min_distance

- **Type:** float
- **Default:** `0.0`
- **Setter:** `set_fill_min_distance(value)`
- **Getter:** `get_fill_min_distance()`

Minimum distance between placed instances. Higher values produce more uniform spacing but may fail to place all requested instances.

---

### gen_scatter_enabled

- **Type:** bool
- **Default:** `false`
- **Setter:** `set_scatter_enabled(value)`
- **Getter:** `get_scatter_enabled()`

Apply random scatter effects after generation. Adds variation to any generation mode.

---

### gen_scatter_jitter

- **Type:** Vector3
- **Default:** `Vector3(0, 0, 0)`
- **Setter:** `set_scatter_jitter(value)`
- **Getter:** `get_scatter_jitter()`

Random position offset per axis. Each instance is displaced by a random value in the range [-jitter, +jitter] per axis.

---

### gen_scatter_random_rotation

- **Type:** bool
- **Default:** `false`
- **Setter:** `set_scatter_random_rotation(value)`
- **Getter:** `get_scatter_random_rotation()`

Add a random Y-axis rotation to each instance.

---

### gen_scatter_scale_min

- **Type:** float
- **Default:** `1.0`
- **Setter:** `set_scatter_scale_min(value)`
- **Getter:** `get_scatter_scale_min()`

Minimum random scale. When min equals max, no scale variation is applied.

---

### gen_scatter_scale_max

- **Type:** float
- **Default:** `1.0`
- **Setter:** `set_scatter_scale_max(value)`
- **Getter:** `get_scatter_scale_max()`

Maximum random scale.

---

### gen_instance_data

- **Type:** MultiNodeInstanceData
- **Default:** `null`
- **Setter:** `set_instance_data_resource(value)`
- **Getter:** `get_instance_data_resource()`

Optional resource to save/load generated instance transforms. Use Save Instance Data to snapshot the current state.

## Methods

| Return | Name |
|--------|------|
| void | [generate](#generate)() |
| void | [clear_and_notify](#clear_and_notify)() |
| void | [save_instance_data](#save_instance_data)() |
| void | [exclude_instance](#exclude_instance)(local_index: int) |
| void | [include_instance](#include_instance)(local_index: int) |
| bool | [is_instance_excluded](#is_instance_excluded)(local_index: int) |
| int | [get_owned_start](#get_owned_start)() |
| int | [get_owned_count](#get_owned_count)() |
| int | [local_to_parent_index](#local_to_parent_index)(local: int) |
| int | [parent_to_local_index](#parent_to_local_index)(parent: int) |

## Method Descriptions

### generate

```
void generate()
```

Run the generator. Creates transforms based on the current mode and settings, applies scatter if enabled, and writes results to the parent MultiNode. Multiple generators coordinate automatically -- each owns a contiguous slice of the parent's instances.

---

### clear_and_notify

```
void clear_and_notify()
```

Remove all instances owned by this generator and update the parent.

---

### save_instance_data

```
void save_instance_data()
```

Save the current generated transforms to the gen_instance_data resource. Creates a new MultiNodeInstanceData resource if none is assigned.

---

### exclude_instance

```
void exclude_instance(local_index: int)
```

Mark an instance as excluded (broken away). The generator will stop overwriting its transform, allowing physics, scripts, or other systems to control it independently. Index is local to this generator's owned slice.

---

### include_instance

```
void include_instance(local_index: int)
```

Remove the exclusion flag from an instance, returning it to generator control.

---

### is_instance_excluded

```
bool is_instance_excluded(local_index: int)
```

Check whether an instance is excluded from generator control.

---

### get_owned_start

```
int get_owned_start()
```

Returns the first parent index owned by this generator.

---

### get_owned_count

```
int get_owned_count()
```

Returns how many parent instances this generator owns.

---

### local_to_parent_index

```
int local_to_parent_index(local: int)
```

Convert a generator-local index to a parent MultiNode index.

---

### parent_to_local_index

```
int parent_to_local_index(parent: int)
```

Convert a parent MultiNode index to a generator-local index.

## Enumerations

### enum GeneratorMode

| Constant | Value | Description |
|----------|-------|-------------|
| GEN_ARRAY | `0` | Generate instances in grid or chained patterns. |
| GEN_MESH_FILL | `1` | Scatter instances across a mesh surface or fill its volume. |
| GEN_MANUAL | `2` | Hand-place instances one at a time via inspector controls. |

### enum FillMode

| Constant | Value | Description |
|----------|-------|-------------|
| FILL_SURFACE | `0` | Distribute instances across the mesh surface, weighted by triangle area. |
| FILL_VOLUME | `1` | Fill the mesh interior using rejection sampling within the AABB. |
