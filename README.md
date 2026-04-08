# MultiNode — Compositional Instancing for Godot 4.6

**One node. Thousands of instances. Full behavior.**

MultiNode is an open-source addon that brings real instancing to Godot — not just rendering, but collision, animation, audio, particles, raycasts, labels, lights, decals, and more. Drop in the child nodes you need, and every instance gets full behavior without thousands of individual scene nodes.

\---

## Introducing MultiNode

### What Is an Instance?

In game engines, an *instance* is a copy of an object. It shares the same mesh, material, and shape — but has its own position in the world. A forest of 10,000 trees? That's 10,000 instances of one tree.

Rendering engines handle this well. Godot's **MultiMesh** can render 10,000 identical meshes in a single draw call. Fast and cheap.

But a rendered mesh is just a picture. It can't collide with the player. It doesn't make sound. It can't be clicked, animated, or detected by game logic. The moment you need any of that, you're back to creating individual nodes — one `MeshInstance3D`, one `StaticBody3D`, one `CollisionShape3D`, one `AudioStreamPlayer3D`... *per instance*. At 10,000 instances, that's 40,000+ nodes. Your scene tree explodes, your editor crawls, and your frame rate dies.

**MultiMesh gives you fast pictures. MultiNode gives you fast everything.**

### How It Works

MultiNode is a parent node that owns a list of instance transforms. You add child nodes for the behaviors you need:

```
MultiNode                  ← owns 5,000 instance transforms
  ├── MultiNodeMesh          ← renders all 5,000 (1 GPU draw call)
  ├── MultiNodeCollider      ← 5,000 physics bodies (via PhysicsServer3D)
  ├── MultiNodeAudio         ← spatial audio for all 5,000
  └── MultiNodeAnimator      ← per-instance animation playback
```

**Composition over inheritance.** Each child handles one concern. Need collision? Add a `MultiNodeCollider`. Need sound? Add a `MultiNodeAudio`. Don't need particles? Don't add one. Every child operates on the same shared instance list. No duplication, no sync issues.

Each child can also filter *which* instances it affects. `instance\_every = 2` means every other instance. `instance\_range = "0:99,-50:59"` means instances 0–99 except 50–59. Use different meshes, shapes, or behaviors on subsets of instances.

\---

## Why This Matters: The Instancing Gap

### How Godot's MultiMesh Works (and Its Limits)

`MultiMeshInstance3D` stores a flat buffer of transforms and renders them in one draw call. That's all it does.

What MultiMesh **cannot** do:

* No per-instance collision or physics bodies
* No per-instance raycasting or click detection
* No per-instance audio
* No per-instance animation playback
* No per-instance particles, lights, labels, decals, or areas
* No per-instance game logic

The community has been asking for these capabilities:

* [**Add collision detection / raycast to MultiMeshInstance3D**](https://github.com/godotengine/godot-proposals/issues/10828) — *"Want the player to be able to click on a spot in that scene and know where they clicked"*
* [**Allow specifying more custom instance data for MultiMesh**](https://github.com/godotengine/godot-proposals/issues/8666)
* [**Make placing MultiMesh instances manually easier**](https://github.com/godotengine/godot-proposals/issues/5352)

The common workaround — creating thousands of individual nodes — defeats the purpose of instancing entirely.

### How Other Engines Handle This

**Unity** offers two paths: **GPU Instancing** (`DrawMeshInstanced`) for fast rendering with zero per-instance behavior, or **DOTS/ECS** for full entity simulation — but DOTS requires rewriting your entire game in a different programming paradigm. Most developers describe the transition as "rewriting your game from scratch." There is no middle ground.

**Unreal Engine** has **Hierarchical Instanced Static Mesh (HISM)**, which can optionally enable per-instance collision — but each body is a separate PhysX/Chaos shape, creating a significant CPU bottleneck at high counts. Collision is all-or-nothing per component. No per-instance animation, audio, or gameplay logic. Unreal's **Mass Entity Framework** is powerful but poorly documented and requires deep C++ knowledge.

**Every major engine** treats instancing as a rendering optimization first, with per-instance behavior as an afterthought requiring significant custom engineering.

### MultiNode Closes the Gap

|Capability|Unity GPU|Unity DOTS|Unreal HISM|Godot MultiMesh|**MultiNode**|
|-|-|-|-|-|-|
|GPU-instanced rendering|Yes|Yes|Yes|Yes|**Yes**|
|Per-instance collision|No|Yes (rewrite)|Yes (expensive)|No|**Yes**|
|Per-instance animation|No|Partial|No|No|**Yes**|
|Per-instance audio|No|No|No|No|**Yes**|
|Per-instance particles|No|No|No|No|**Yes**|
|Per-instance raycasting|No|Manual|No|No|**Yes**|
|Per-instance lights|No|No|No|No|**Yes**|
|Per-instance labels|No|No|No|No|**Yes**|
|Compute shaders per instance|Manual|Burst|Niagara only|No|**Yes**|
|Compositional (mix \& match)|No|Somewhat|No|No|**Yes**|
|Setup complexity|Low|Very High|Low-Med|Low|**Low**|

MultiNode doesn't just close the instancing gap in Godot. It provides per-instance capabilities that no other engine offers in a single composable system. Unreal HISM comes closest for collision but lacks everything else. Unity DOTS is powerful but demands a complete architectural rewrite. MultiNode gives you full-stack instancing by adding child nodes — the same workflow you already know in Godot.

\---

## Performance

Tested with 3,000 rigid-body cubes with self-collision (Jolt physics):

|Metric|MultiNode|Standard Godot (MultiMesh)|Standard Godot (individual nodes)|
|-|-|-|-|
|Scene tree nodes|\~20|\~6,000+|9,000+|
|Scene file size|Small|Small|1.5 MB+|
|Peak FPS (active)|\~60|\~35|\~10|
|Settled FPS|\~90|\~90|Unstable|
|VRAM|Same|Same|Same|

**v0.2.1 optimizations:** Physics readback now skips sleeping bodies entirely (`cancel_batch`), dirty transform updates use sparse index lists instead of scanning all N instances, and fully-hidden MultiMeshes cost zero GPU time. The result: resting-state performance matches raw Godot, and active-state physics scales significantly better.

Included test scenes (`test_standard_scene.tscn` and `test_multinode_baseline_scene.tscn`) provide apples-to-apples comparison with identical spawn patterns, environment, and camera.

\---

## Node Reference

### MultiNode (Parent)

The root container. Owns the master list of instance transforms and per-instance data arrays. All child nodes read from this shared list. There is no Godot equivalent — you would manage transforms manually and synchronize every system yourself.

**Use cases:** Any scene with many copies of the same object — forests, crowds, projectiles, inventory items, building pieces, particle-like gameplay objects.

### MultiNodeInstancer

Generates instance transforms. Three modes: **Array** (grid layout), **Mesh Fill** (scatter across a mesh surface/volume), and **Manual** (click-to-place in the editor). Godot's MultiMesh has a basic "Populate" button — this is far more flexible.

**Use cases:** Procedural foliage, scattered rocks, hand-placed key instances, filling a mesh volume.

### MultiNodeMesh

GPU-instanced rendering via Godot's MultiMesh internally. Supports per-instance color tinting, cast shadows, and render layers. This is the same fast rendering path as `MultiMeshInstance3D`, integrated with the shared instance list.

**Use cases:** Rendering thousands of trees, rocks, buildings, projectiles, or any repeated visual.

### MultiNodeCollider

Creates one physics body per instance via `PhysicsServer3D` directly — no scene tree nodes. Supports **Static**, **Kinematic**, and **Rigid** body types with per-instance collision shapes. Handles velocity handoff when switching body types at runtime.

In Godot, the alternative is thousands of individual `StaticBody3D`/`RigidBody3D` nodes. In Unreal, HISM per-instance collision exists but is expensive and all-or-nothing. Unity has no built-in equivalent.

**Use cases:** Destructible walls (static → rigid on hit), physics foliage, kinematic platforms, large-scale rigid body simulations.

### MultiNodeArea

Per-instance detection zones via `PhysicsServer3D`. Detects when other bodies enter/exit each instance's area and reports which instance was entered. The alternative is thousands of `Area3D` nodes.

**Use cases:** Proximity triggers, pickup zones, damage fields, stealth detection ranges.

### MultiNodeRaycast

Casts a ray from each instance every physics frame. Reports hits with instance index, position, normal, and collider. The alternative is thousands of `RayCast3D` nodes or manual `PhysicsDirectSpaceState3D` queries.

**Use cases:** Ground detection for hovering objects, line-of-sight for AI, projectile scanning, shadow placement.

### MultiNodeAnimator

Per-instance animation using the **MultiAnimation** key-based resource. Each instance has independent key state, speed, and progress. Supports sequential (linear) and state machine (circular) topologies. Method calls fire on key arrival with the instance index. Sync modes allow lockstep or staggered playback.

No engine provides per-instance animation at this scale without an ECS rewrite.

**Use cases:** Bobbing platforms with staggered timing, crowd animations, blinking lights, per-unit state machines.

### MultiNodeAudio

Software-mixed spatial audio for all instances through a single audio bus. Attenuates and pans each instance's sound based on distance and direction to the listener. Supports looping, volume, amplification, and distance falloff.

No engine provides per-instance spatial audio out of the box.

**Use cases:** Ambient sounds (humming machines, rustling trees), notification sounds, environmental audio.

### MultiNodeParticle

Spawns a `GPUParticles3D` emitter at each instance's position. Supports process materials, draw meshes, and per-instance emission control.

**Use cases:** Smoke from chimneys, sparks from damaged machines, fire on torches, vehicle exhaust.

### MultiNodeLight

Omni or spot light at each instance's position via `RenderingServer`. Configurable color, energy, range, and shadow enable.

**Use cases:** Streetlights, torch glow, indicator lights, dynamic lighting for moving objects.

### MultiNodeLabel

Billboard text labels at each instance via atlas rendering + MultiMesh. Per-instance text, font size, outline, and color.

**Use cases:** Floating names, health bars, distance markers, debug info per instance.

### MultiNodeSprite

Billboard sprite sheets at each instance. Animation frames, per-instance frame selection, and configurable pixel size.

**Use cases:** 2D characters in 3D world, animated icons, retro-style NPCs, billboard vegetation.

### MultiNodeDecal

Projected decals at each instance's position via `RenderingServer`.

**Use cases:** Blood splatters, tire marks, scorch marks, footprints, terrain painting.

### MultiNodeMarker

Per-instance wireframe visualization. Four shapes: **Cross** (minimal position marker), **Diamond** (wireframe octahedron), **Sphere** (three orthogonal circles), and **Axes** (RGB XYZ arrows showing orientation). Editor-only by default, with always-on-top option.

**Use cases:** Visualizing invisible instances (audio sources, areas, raycasts), teaching what instances are, debugging spatial placement.

### MultiNodeSub (Base Class)

Base for all child nodes. Provides:

* **Active filtering** — `instance\_every` and `instance\_range` to control which instances each child processes
* **Per-instance data arrays** — flat float buffers accessible from GDScript, C++, and GPU shaders
* **Compute shaders** — set `compute\_code` to GLSL, enable, and it runs on the GPU every frame with instance transforms, active flags, and data as bindings

\---

## Getting Started

### Installation

Drop the addon into your Godot 4.6 project:

```
your\_project/
  addons/
    multi\_node/     ← copy this folder
  project.godot
```

Enable the plugin: **Project → Project Settings → Plugins → MultiNode → Enable**

### Quick Start

```gdscript
extends MultiNode

func \_ready() -> void:
    begin\_batch()
    for i: int in range(500):
        add\_instance(Transform3D(Basis(), Vector3(randf() \* 100, 20, randf() \* 100)))
    end\_batch()
```

Add child nodes in the editor for rendering, physics, audio — whatever you need.

### Building from Source

```bash
cd addons/multi\_node
git clone https://github.com/godotengine/godot-cpp.git -b master --depth 1
pip install scons
scons target=template\_debug    # Debug build
scons target=template\_release  # Release build
```

Requires: Godot 4.6, Python 3, SCons, C++ compiler (MSVC or MinGW-w64).

\---

## Status

**Open Alpha (v0.2.1)** — 17 node types + 1 resource, all working. The API may evolve based on community feedback.

**v0.2.1 changelog:**
* **MultiNodeMarker** — per-instance wireframe visualization (Cross, Diamond, Sphere, Axes) for debugging and teaching
* Major physics optimization: cancel_batch, sparse dirty tracking, cached active counts
* visible_instances=0 fix: fully-hidden MultiMeshes no longer waste GPU time on stale AABBs
* Area Jolt workaround: space-toggle instead of free_rid prevents broad-phase leaks
* request_sync: script-driven set_instance_active() changes are always processed
* Bug fixes across all spatial sub-nodes (centralized transforms, raycast stale-hits, instancer shrink)
* Comparison test scenes for benchmarking against standard Godot

* [Report issues](https://github.com/AverageDrafter/MultiNode-Plugin-Working/issues)
* [Discord community](https://discord.gg/tcYjECNYt7)

## License

MIT — Full source code included. Study it, modify it, ship it.

## Links

* **Source:** [AverageDrafter/MultiNode-Plugin-Working](https://github.com/AverageDrafter/MultiNode-Plugin-Working)
* **Distribution:** [AverageDrafter/MultiNode-Plugin](https://github.com/AverageDrafter/MultiNode-Plugin)

