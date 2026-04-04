# MultiNodeArea

**Inherits:** [MultiNode3D](MultiNode3D.md)

Detection zones for each instance via PhysicsServer3D areas.

## Description

Creates a PhysicsServer3D area for each active instance in the parent MultiNode. Areas detect overlapping bodies without applying physics forces, making them ideal for spatial queries and trigger-based gameplay logic. Each area is positioned at its instance's world transform (with the MultiNode3D offset applied) and updated automatically when instances move.

Use cases include trigger zones, pickup detection, proximity alerts, and checkpoints. The area pool is managed internally -- areas are created when instances become active and removed when they become inactive.

Assign a `Shape3D` resource to define the detection volume. All areas share the same shape, collision layer, and collision mask. To identify which instance an overlapping area belongs to, use `get_instance_from_area()` with the area RID reported by the physics system.

## Properties

| Name | Type | Default |
|------|------|---------|
| [shape](#shape) | Shape3D | `null` |
| [collision_layer](#collision_layer) | int | `1` |
| [collision_mask](#collision_mask) | int | `1` |
| [monitorable](#monitorable) | bool | `false` |

## Property Descriptions

### shape

```
Shape3D shape = null
```

- *Setter:* `set_shape(value: Shape3D)`
- *Getter:* `get_shape() -> Shape3D`

The collision shape used for all area instances. All areas share this single shape resource. Supports any Shape3D subclass: BoxShape3D, SphereShape3D, CapsuleShape3D, CylinderShape3D, ConvexPolygonShape3D, ConcavePolygonShape3D, etc.

Changing the shape rebuilds all active areas. Set to `null` to disable area detection.

---

### collision_layer

```
int collision_layer = 1
```

- *Setter:* `set_collision_layer(value: int)`
- *Getter:* `get_collision_layer() -> int`

The physics layers this area occupies. Other physics objects detect this area when their collision mask overlaps with this layer value. This is a bitmask -- use powers of two to assign areas to specific layers (e.g., `1` = layer 1, `4` = layer 3, `5` = layers 1 and 3).

---

### collision_mask

```
int collision_mask = 1
```

- *Setter:* `set_collision_mask(value: int)`
- *Getter:* `get_collision_mask() -> int`

The physics layers this area scans for overlapping bodies and areas. Only objects whose collision layer overlaps with this mask will be detected. This is a bitmask -- combine layers with bitwise OR.

---

### monitorable

```
bool monitorable = false
```

- *Setter:* `set_monitorable(value: bool)`
- *Getter:* `get_monitorable() -> bool`

If `true`, other Area3D nodes can detect these areas. If `false`, the areas are invisible to other areas' `body_entered`/`area_entered` signals, which reduces physics server overhead.

Set to `true` when you need other areas in your scene to react to MultiNodeArea instances (e.g., an NPC's detection zone sensing a MultiNode checkpoint). Leave `false` when you only need to query overlaps from the MultiNode side.

## Methods

| Return | Name |
|--------|------|
| RID | [get_instance_area_rid](#get_instance_area_rid) |
| int | [get_instance_from_area](#get_instance_from_area) |

## Method Descriptions

### get_instance_area_rid

```
RID get_instance_area_rid(index: int)
```

Returns the PhysicsServer3D area RID for the given instance index. Use this RID for direct PhysicsServer3D queries, such as calling `PhysicsServer3D.area_get_shape_count()` or setting up custom monitoring.

Returns an empty RID if the instance index is out of range or the instance is not active.

---

### get_instance_from_area

```
int get_instance_from_area(area_rid: RID)
```

Maps a PhysicsServer3D area RID back to its instance index. This is the reverse lookup of `get_instance_area_rid()` and is the primary way to identify which instance triggered a physics callback.

Returns `-1` if the RID does not belong to any instance managed by this node. Typical usage is inside an `area_entered` or `body_entered` callback on another node, where you receive the collider RID and need to know which MultiNode instance it corresponds to.
