# MultiNodeRaycast

**Inherits:** [MultiNode3D](MultiNode3D.md)

Per-instance raycasting for ground detection, line of sight, and surface probing.

## Description

Casts a ray from each active instance every physics frame. Results are stored per-instance and can be queried at any time. The ray origin is the instance's world position (with MultiNode3D offset applied), and the ray extends in `cast_direction` for `cast_length` units.

Automatically excludes sibling MultiNodeCollider bodies from raycast results. This prevents instances from hitting their own collision shapes, which is the expected behavior for ground detection and terrain following. If there is no sibling MultiNodeCollider, no exclusions are applied.

Raycast results are updated during `_physics_process` and remain valid until the next physics frame. Query methods return the most recent result. If an instance is not active, its raycast result is cleared (not colliding).

Common use cases: ground detection for vehicles or characters, line-of-sight checks for AI, terrain-following flight, wall detection, surface normal probing for alignment, and distance-to-ground calculations.

## Properties

| Name | Type | Default |
|------|------|---------|
| [cast_direction](#cast_direction) | Vector3 | `Vector3(0, -1, 0)` |
| [cast_length](#cast_length) | float | `10.0` |
| [collision_mask](#collision_mask) | int | `1` |
| [hit_from_inside](#hit_from_inside) | bool | `false` |

## Property Descriptions

### cast_direction

```
Vector3 cast_direction = Vector3(0, -1, 0)
```

- *Setter:* `set_cast_direction(value: Vector3)`
- *Getter:* `get_cast_direction() -> Vector3`

The direction the ray is cast in local space, relative to each instance's transform. The default value `(0, -1, 0)` casts straight down, which is typical for ground detection.

The direction is normalized internally. If `use_instance_basis` is enabled on the MultiNode3D parent, the ray direction rotates with each instance's basis, allowing forward-facing raycasts for line-of-sight checks.

---

### cast_length

```
float cast_length = 10.0
```

- *Setter:* `set_cast_length(value: float)`
- *Getter:* `get_cast_length() -> float`

The maximum length of the ray in world units. Longer rays are more expensive. Set this to the minimum distance needed for your use case -- for ground detection, slightly more than the expected maximum height above ground.

---

### collision_mask

```
int collision_mask = 1
```

- *Setter:* `set_collision_mask(value: int)`
- *Getter:* `get_collision_mask() -> int`

The physics layers the ray detects. Only colliders whose collision layer overlaps with this mask will produce a hit. This is a bitmask -- combine layers with bitwise OR.

---

### hit_from_inside

```
bool hit_from_inside = false
```

- *Setter:* `set_hit_from_inside(value: bool)`
- *Getter:* `get_hit_from_inside() -> bool`

If `true`, the ray detects colliders even when the ray origin is inside the collider shape. If `false`, rays starting inside a shape pass through it without registering a hit.

Enable this when instances may overlap with geometry and you still need accurate hit detection (e.g., instances embedded in terrain during spawning).

## Methods

| Return | Name |
|--------|------|
| bool | [is_instance_colliding](#is_instance_colliding) |
| Vector3 | [get_instance_hit_point](#get_instance_hit_point) |
| Vector3 | [get_instance_hit_normal](#get_instance_hit_normal) |
| float | [get_instance_hit_distance](#get_instance_hit_distance) |
| RID | [get_instance_hit_rid](#get_instance_hit_rid) |

## Method Descriptions

### is_instance_colliding

```
bool is_instance_colliding(index: int)
```

Returns `true` if the ray for the given instance hit a collider during the most recent physics frame. Returns `false` if no collision occurred, the instance is inactive, or the index is out of range.

---

### get_instance_hit_point

```
Vector3 get_instance_hit_point(index: int)
```

Returns the world-space position where the ray intersected a collider for the given instance. Only valid when `is_instance_colliding()` returns `true`. Returns `Vector3.ZERO` if there was no collision.

---

### get_instance_hit_normal

```
Vector3 get_instance_hit_normal(index: int)
```

Returns the surface normal at the collision point for the given instance. The normal points away from the collider surface. Useful for aligning instances to terrain slopes or calculating reflection angles.

Returns `Vector3.ZERO` if there was no collision.

---

### get_instance_hit_distance

```
float get_instance_hit_distance(index: int)
```

Returns the distance from the ray origin to the hit point for the given instance. This is the straight-line distance along the ray direction, not the Euclidean distance between two arbitrary points.

Returns `0.0` if there was no collision.

---

### get_instance_hit_rid

```
RID get_instance_hit_rid(index: int)
```

Returns the PhysicsServer3D collider RID that the ray hit for the given instance. Use this to identify what object was hit via PhysicsServer3D queries.

Returns an empty RID if there was no collision.
