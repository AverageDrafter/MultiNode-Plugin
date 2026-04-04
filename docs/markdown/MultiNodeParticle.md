# MultiNodeParticle

**Inherits:** [MultiNodeVisual3D](MultiNodeVisual3D.md)

GPU particle emitters for each instance, sharing a common material and mesh.

## Description

Creates a GPUParticles3D child node per active instance in the parent MultiNode. All emitters share the same process material and draw mesh, ensuring visual consistency while allowing per-instance control over emission.

Each emitter is positioned at its instance's world transform (with the MultiNode3D offset applied) and updated automatically when instances move. Emitters are created when instances become active and removed when they become inactive.

Keep instance counts reasonable when using this node -- each GPUParticles3D emitter has GPU overhead for simulation and rendering. For large instance counts (thousands), consider whether particles are necessary for all instances or if `instance_every` / `instance_range` filtering can limit the emitter count.

The `process_material` property accepts any ParticleProcessMaterial (or custom shader material) that defines particle behavior: velocity, gravity, color gradients, emission shape, etc. The `draw_mesh` defines the geometry each particle renders as -- typically a QuadMesh for billboard particles.

Inherits shadow casting, render layers, and per-instance color tinting from MultiNodeVisual3D. Per-instance colors are applied to the emitters, allowing tinted particles per instance.

## Properties

| Name | Type | Default |
|------|------|---------|
| [process_material](#process_material) | Material | `null` |
| [draw_mesh](#draw_mesh) | Mesh | `null` |
| [amount](#amount) | int | `16` |
| [lifetime](#lifetime) | float | `1.0` |
| [emitting](#emitting) | bool | `true` |
| [speed_scale](#speed_scale) | float | `1.0` |
| [explosiveness](#explosiveness) | float | `0.0` |
| [randomness](#randomness) | float | `0.0` |
| [one_shot](#one_shot) | bool | `false` |
| [pre_process_time](#pre_process_time) | float | `0.0` |
| [fixed_fps](#fixed_fps) | int | `0` |

## Property Descriptions

### process_material

```
Material process_material = null
```

- *Setter:* `set_process_material(value: Material)`
- *Getter:* `get_process_material() -> Material`

The material that defines particle behavior. Typically a ParticleProcessMaterial controlling velocity, acceleration, color over lifetime, emission shape, gravity, and other simulation parameters. All emitters share this material.

Changing the material updates all existing emitters immediately.

---

### draw_mesh

```
Mesh draw_mesh = null
```

- *Setter:* `set_draw_mesh(value: Mesh)`
- *Getter:* `get_draw_mesh() -> Mesh`

The mesh used to render each particle. Common choices are QuadMesh (for billboard sprites), BoxMesh, or SphereMesh. All emitters share this mesh resource.

If `null`, particles use the default quad mesh provided by Godot's GPUParticles3D.

---

### amount

```
int amount = 16
```

- *Setter:* `set_amount(value: int)`
- *Getter:* `get_amount() -> int`

The number of particles emitted by each instance's emitter. Total particle count across all instances is `amount * active_instance_count`. Higher values produce denser effects but increase GPU load proportionally.

---

### lifetime

```
float lifetime = 1.0
```

- *Setter:* `set_lifetime(value: float)`
- *Getter:* `get_lifetime() -> float`

The time in seconds each particle lives before being recycled. Longer lifetimes mean more particles alive simultaneously (for the same emission rate), producing trails or lingering effects. Shorter lifetimes produce snappier, more contained effects.

---

### emitting

```
bool emitting = true
```

- *Setter:* `set_emitting(value: bool)`
- *Getter:* `get_emitting() -> bool`

Global emission toggle for all instances. When `true`, all active instances emit particles. When `false`, no instances emit (existing particles continue until their lifetime expires). Per-instance emission can be controlled independently via `set_instance_emitting()`.

---

### speed_scale

```
float speed_scale = 1.0
```

- *Setter:* `set_speed_scale(value: float)`
- *Getter:* `get_speed_scale() -> float`

Global speed multiplier for particle simulation. `1.0` is normal speed, `2.0` makes particles simulate at double speed, `0.5` creates a slow-motion effect. Applied to all emitters.

---

### explosiveness

```
float explosiveness = 0.0
```

- *Setter:* `set_explosiveness(value: float)`
- *Getter:* `get_explosiveness() -> float`

Controls how particles are emitted over time. `0.0` emits particles at a steady rate throughout the lifetime. `1.0` emits all particles at once in a single burst. Values between create a mix. Useful for explosion effects or burst-style emissions.

Range: 0.0 to 1.0.

---

### randomness

```
float randomness = 0.0
```

- *Setter:* `set_randomness(value: float)`
- *Getter:* `get_randomness() -> float`

Adds random variation to the emission timing. `0.0` emits at regular intervals, `1.0` makes emission timing fully random. Breaks up the uniform look of steady emission.

Range: 0.0 to 1.0.

---

### one_shot

```
bool one_shot = false
```

- *Setter:* `set_one_shot(value: bool)`
- *Getter:* `get_one_shot() -> bool`

If `true`, each emitter plays through one full cycle and then stops. No new particles are created after the initial burst. Use `restart_instance()` or `restart_all()` to trigger the effect again.

---

### pre_process_time

```
float pre_process_time = 0.0
```

- *Setter:* `set_pre_process_time(value: float)`
- *Getter:* `get_pre_process_time() -> float`

Time in seconds to simulate before the first frame. Causes particles to appear already in-flight when the emitter starts, avoiding the "empty startup" look. Useful for ambient effects that should look established from the start (e.g., fire, smoke, rain).

---

### fixed_fps

```
int fixed_fps = 0
```

- *Setter:* `set_fixed_fps(value: int)`
- *Getter:* `get_fixed_fps() -> int`

If greater than `0`, particle simulation runs at this fixed framerate instead of every rendered frame. Reduces GPU load by simulating less frequently. `0` means simulate every frame (the default). Typical values: 30 or 60.

## Methods

| Return | Name |
|--------|------|
| void | [set_instance_emitting](#set_instance_emitting) |
| bool | [get_instance_emitting](#get_instance_emitting) |
| void | [restart_instance](#restart_instance) |
| void | [restart_all](#restart_all) |

## Method Descriptions

### set_instance_emitting

```
void set_instance_emitting(index: int, emitting: bool)
```

Sets the emission state for a single instance's particle emitter. When `false`, the emitter stops creating new particles (existing particles continue until they expire). When `true`, emission resumes.

This is independent of the global `emitting` property -- both must be `true` for an instance to emit.

---

### get_instance_emitting

```
bool get_instance_emitting(index: int)
```

Returns the per-instance emission state. Returns `true` if the instance is set to emit, `false` otherwise. Note that even if this returns `true`, the instance will not emit if the global `emitting` property is `false`.

---

### restart_instance

```
void restart_instance(index: int)
```

Restarts the particle emitter for a single instance. Clears all existing particles and begins a fresh emission cycle. Useful for `one_shot` effects that need to be retriggered.

---

### restart_all

```
void restart_all()
```

Restarts all particle emitters across all active instances. Each emitter clears its particles and begins a fresh emission cycle.
