# MultiNodeCollider

**Inherits:** MultiNode3D

Creates and manages physics bodies for each instance via PhysicsServer3D.

## Description

MultiNodeCollider creates a physics body for each active instance in the parent MultiNode. Bodies can be static, kinematic, or rigid. Rigid bodies read their transforms back from the physics engine each frame, automatically skipping sleeping bodies. Inactive instances have their bodies removed from the physics space. Use `get_instance_from_body()` to identify which instance was hit in collision callbacks.

## Properties

| Name | Type | Default |
|------|------|---------|
| [shape](#shape) | Shape3D | `null` |
| [body_type](#body_type) | BodyType | `BODY_TYPE_STATIC (0)` |
| [collision_layer](#collision_layer) | int | `1` |
| [collision_mask](#collision_mask) | int | `1` |
| [self_collision](#self_collision) | bool | `true` |
| [preserve_velocity](#preserve_velocity) | bool | `false` |

## Property Descriptions

### shape

- **Type:** Shape3D
- **Default:** `null`
- **Setter:** `set_shape(value)`
- **Getter:** `get_shape()`

The collision shape used for every instance. All instances share this single shape resource. Assign any Shape3D (BoxShape3D, SphereShape3D, ConvexPolygonShape3D, etc.).

---

### body_type

- **Type:** int (BodyType)
- **Default:** `0` (`BODY_TYPE_STATIC`)
- **Setter:** `set_body_type(value)`
- **Getter:** `get_body_type()`

The physics body type for all instances. `Static` = immovable, no physics simulation. `Kinematic` = moved by code, pushes rigid bodies. `Rigid` = full physics simulation with gravity and collisions, transforms are read back from the physics engine each frame.

---

### collision_layer

- **Type:** int
- **Default:** `1`
- **Setter:** `set_collision_layer(value)`
- **Getter:** `get_collision_layer()`

The physics layer bitmask that these bodies exist on. Other bodies detect collisions with these instances based on their `collision_mask`.

---

### collision_mask

- **Type:** int
- **Default:** `1`
- **Setter:** `set_collision_mask(value)`
- **Getter:** `get_collision_mask()`

The physics layer bitmask that these bodies scan for collisions against. Only bodies on matching layers are detected.

---

### self_collision

- **Type:** bool
- **Default:** `true`
- **Setter:** `set_self_collision(value)`
- **Getter:** `get_self_collision()`

When enabled, instances can collide with each other. When disabled, rigid body instances are placed on an internal physics layer (layer 20) to prevent self-collision. Note: this layer override only affects rigid bodies -- static and kinematic bodies always use your configured `collision_layer` since they cannot self-collide. External objects must include layer 20 in their collision mask to detect rigid bodies with `self_collision` disabled.

---

### preserve_velocity

- **Type:** bool
- **Default:** `false`
- **Setter:** `set_preserve_velocity(value)`
- **Getter:** `get_preserve_velocity()`

When enabled, linear and angular velocity are saved to the parent MultiNode's velocity bus when a body is removed from the physics space (deactivated), and restored when it re-enters. Use for seamless collider hot-swapping between LOD levels.

## Methods

| Return | Name |
|--------|------|
| int | [get_instance_from_body](#get_instance_from_body)(body_rid: RID) |
| RID | [get_body_rid](#get_body_rid)(index: int) |

## Method Descriptions

### get_instance_from_body

```
int get_instance_from_body(body_rid: RID)
```

Map a physics body RID back to an instance index. Returns `-1` if the RID does not belong to this node. Use in collision callbacks to identify which instance was involved.

---

### get_body_rid

```
RID get_body_rid(index: int)
```

Get the PhysicsServer3D body RID for a specific instance. Use for direct physics queries or applying forces to individual instances.

## Enumerations

### enum BodyType

| Name | Value | Description |
|------|-------|-------------|
| BODY_TYPE_STATIC | `0` | Immovable body. No physics simulation. |
| BODY_TYPE_KINEMATIC | `1` | Code-driven body that pushes other objects. |
| BODY_TYPE_RIGID | `2` | Fully simulated rigid body with gravity and collisions. |
