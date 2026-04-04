# MultiNodeAnimator

**Inherits:** [MultiNodeSub](MultiNodeSub.md)

Per-instance animation playback sharing a single Animation resource.

## Description

Shares one Animation resource across all instances with independent playback state. Each instance has its own time position, speed scale, and playing/paused flag, enabling varied animation across thousands of instances without duplicating resources.

**Transform tracks** (position, rotation, scale) drive instance transforms additively. When an instance starts playing, its current transform is captured as the base. Animation values are then added on top of that base, so a position track with value `(0, 1, 0)` at time 0.5 lifts the instance 1 unit above where it started -- not to an absolute world position. This makes animations portable across all instance positions.

**Value and bezier tracks** target sibling sub-node properties. If the track path points to a sibling MultiNodeSub and that node has a `set_instance_X()` method matching the property name, the animator calls it per-instance automatically. For example, a track targeting `MultiNodeMesh:visible` would call `set_instance_visible(index, value)` on the sibling mesh node for each instance.

**Method tracks** prepend the instance index as the first argument. A method track calling `my_method(42.0)` at a given keyframe will actually call `my_method(instance_index, 42.0)` on the target node.

Two sync modes control how instances relate to each other in time:
- **SYNC_NONE**: Each instance animates independently from whenever it was played. Use for staggered starts, manual per-instance control, or event-driven playback.
- **SYNC_SYNC**: All instances share a base clock. The `sync_offset` property controls the time offset between consecutive instances as a fraction of the animation length (0.0 = lockstep, 0.5 = each instance offset by half the animation from the previous).

## Properties

| Name | Type | Default |
|------|------|---------|
| [animation](#animation) | Animation | `null` |
| [autoplay](#autoplay) | bool | `true` |
| [sync_mode](#sync_mode) | SyncMode | `SYNC_NONE (0)` |
| [sync_offset](#sync_offset) | float | `0.0` |
| [speed_scale](#speed_scale) | float | `1.0` |

## Property Descriptions

### animation

```
Animation animation = null
```

- *Setter:* `set_animation(value: Animation)`
- *Getter:* `get_animation() -> Animation`

The Animation resource shared by all instances. All tracks are bound to the parent MultiNode or sibling sub-nodes when assigned. Changing the animation re-binds all tracks and stops playback.

Supports position, rotation, scale, value, bezier, and method tracks. Audio and animation tracks within the Animation resource are not supported.

---

### autoplay

```
bool autoplay = true
```

- *Setter:* `set_autoplay(value: bool)`
- *Getter:* `get_autoplay() -> bool`

If `true`, all active instances begin playing automatically when the node enters the scene tree. Equivalent to calling `play_all()` during setup.

---

### sync_mode

```
SyncMode sync_mode = SYNC_NONE (0)
```

- *Setter:* `set_sync_mode(value: int)`
- *Getter:* `get_sync_mode() -> int`

Controls how instance playback times relate to each other. See [SyncMode](#enumerations) for values.

- `SYNC_NONE`: Instances are fully independent. Each starts from whatever time was specified in `play_instance()`.
- `SYNC_SYNC`: Instances share a global clock. Each instance's time is `base_time + index * sync_offset * animation_length`, producing wave-like staggered animation.

---

### sync_offset

```
float sync_offset = 0.0
```

- *Setter:* `set_sync_offset(value: float)`
- *Getter:* `get_sync_offset() -> float`

The time offset between consecutive instances when `sync_mode` is `SYNC_SYNC`, expressed as a fraction of the animation length.

- `0.0` = all instances animate in perfect lockstep.
- `0.5` = each instance is offset by half the animation length from the previous, creating an alternating pattern.
- `1.0` = wraps fully, equivalent to lockstep.

Small fractional values (e.g., `0.01`) create smooth wave effects across large instance counts.

---

### speed_scale

```
float speed_scale = 1.0
```

- *Setter:* `set_speed_scale(value: float)`
- *Getter:* `get_speed_scale() -> float`

Global playback speed multiplier applied to all instances. `1.0` is normal speed, `2.0` is double speed, `0.5` is half speed. Negative values play the animation in reverse.

Per-instance speed (set via `set_instance_speed()`) is multiplied with this value.

## Methods

| Return | Name |
|--------|------|
| void | [play_all](#play_all) |
| void | [stop_all](#stop_all) |
| void | [play_instance](#play_instance) |
| void | [stop_instance](#stop_instance) |
| void | [set_instance_speed](#set_instance_speed) |
| float | [get_instance_time](#get_instance_time) |

## Method Descriptions

### play_all

```
void play_all()
```

Starts playback for all active instances from the beginning of the animation. Each instance's base transform is captured at this point -- transform tracks are applied additively on top of these captured values.

---

### stop_all

```
void stop_all()
```

Stops playback for all instances and resets their time positions to zero. Transform tracks stop being applied.

---

### play_instance

```
void play_instance(index: int, from_time: float = 0.0)
```

Starts playback for a single instance from the specified time in seconds. The instance's current transform is captured as its base. If the instance is already playing, playback restarts from `from_time` and the base transform is recaptured.

---

### stop_instance

```
void stop_instance(index: int)
```

Stops playback for a single instance and resets its time position to zero.

---

### set_instance_speed

```
void set_instance_speed(index: int, speed: float)
```

Sets the playback speed multiplier for a single instance. This is multiplied with the global `speed_scale`. A value of `1.0` uses the global speed, `2.0` plays twice as fast as the global speed, and `0.0` effectively pauses the instance while keeping it in the "playing" state.

---

### get_instance_time

```
float get_instance_time(index: int)
```

Returns the current playback time in seconds for the given instance. Returns `0.0` if the instance is not playing or the index is out of range.

## Enumerations

### SyncMode

| Name | Value | Description |
|------|-------|-------------|
| SYNC_NONE | `0` | Each instance animates independently. Time is controlled per-instance via `play_instance()` and `set_instance_speed()`. |
| SYNC_SYNC | `1` | All instances share a base clock with staggered offsets. The offset between consecutive instances is controlled by `sync_offset`. |
