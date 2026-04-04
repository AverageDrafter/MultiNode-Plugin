# MultiNode for Godot 4.6

**Compositional instancing system for Godot 4.6.** One parent node, 15 specialized child types, thousands of instances in a single draw call. Mesh, collision, animation, audio, particles, lights, labels, decals, raycasts, and areas -- compose what you need, skip what you don't.

**Version:** 0.2.0-alpha (Windows x86_64)

## Installation

1. Download or clone this repository
2. Copy the `addons/multi_node/` folder into your Godot project's `addons/` directory
3. Open your project in Godot 4.6+
4. Go to **Project > Project Settings > Plugins** and enable **MultiNode**
5. Done -- MultiNode types are now available in the "Add Node" dialog

Your project structure should look like:
```
your_project/
  addons/
    multi_node/
      bin/
      doc_classes/
      icons/
      plugin.cfg
      plugin.gd
      multi_node.gdextension
      ...
  project.godot
```

## Quick Start

1. Add a **MultiNode** as the parent
2. Add child nodes for the behaviors you need:
   - **MultiNodeMesh** -- GPU-instanced rendering
   - **MultiNodeCollider** -- physics bodies (static, kinematic, or rigid)
   - **MultiNodeLight** -- per-instance dynamic lights
   - ...any combination of the 15 sub-node types
3. Add instances via code (`add_instance()`) or use **MultiNodeInstancer** for array/scatter generation
4. Each child operates on the shared instance list -- add a mesh and a collider, and every instance gets both

## Node Types

### Parent
| Node | Purpose |
|------|---------|
| **MultiNode** | Parent that owns instance count, transforms, and shared data |

### Generator
| Node | Purpose |
|------|---------|
| **MultiNodeInstancer** | Array, mesh-fill, and manual instance generation |

### Rendering
| Node | Purpose |
|------|---------|
| **MultiNodeMesh** | GPU-instanced mesh rendering via MultiMesh |
| **MultiNodeSprite** | Billboard sprite sheets with per-instance animation frames |
| **MultiNodeLabel** | Billboard text labels via shared atlas |
| **MultiNodeParticle** | GPU particle emitters per instance |
| **MultiNodeDecal** | Projected decals per instance |
| **MultiNodeLight** | Omni or spot lights per instance |

### Physics
| Node | Purpose |
|------|---------|
| **MultiNodeCollider** | Static, kinematic, or rigid bodies via PhysicsServer3D |
| **MultiNodeArea** | Detection zones via PhysicsServer3D |
| **MultiNodeRaycast** | Per-instance raycasting with hit results |

### Other
| Node | Purpose |
|------|---------|
| **MultiNodeAnimator** | Shared animation with per-instance playback and sync |
| **MultiNodeAudio** | Software-mixed spatial audio for all instances |

### Base Classes (not used directly)
| Node | Purpose |
|------|---------|
| **MultiNodeSub** | Base for all children -- active filtering, compute shaders, per-instance data |
| **MultiNode3D** | Adds transform offset to spatial children |
| **MultiNodeVisual3D** | Adds shadow/render layer/tint to visual children |

## Active Filtering

Every sub-node has two filtering layers:

- **instance_every** (int) -- pattern filter. `1` = all, `N` = every Nth, `-N` = complement
- **instance_range** (String) -- Excel-style ranges. `"0:99,-50:59,150"` includes 0-99 except 50-59, plus 150

This lets different sub-nodes operate on different subsets of instances. Give one mesh to every 3rd instance, another to the rest. Light only the first 100. Collide all of them.

## Compute Shaders

Any sub-node can run a GLSL compute shader on its instances. Set `compute_code` to GLSL and `compute_enabled = true`. The shader receives transforms, active flags, and per-instance data via storage buffers.

## Building from Source

Precompiled Windows binaries are included. To build from source (or for Linux/macOS):

```bash
cd addons/multi_node
git clone https://github.com/godotengine/godot-cpp.git -b 4.6 --depth 1
pip install scons
scons target=template_debug    # Debug build
scons target=template_release  # Release build
```

Requires: Python 3, SCons, C++ compiler (MSVC or MinGW-w64 on Windows).

## Requirements

- Godot 4.6+ (standard or .NET build)
- Windows x86_64 (precompiled), or build from source for other platforms

## License

MIT -- see [LICENSE](LICENSE)

## Links

- [Source & Development](https://github.com/AverageDrafter/MultiNode-Plugin-Working)
- [Issues & Feedback](https://github.com/AverageDrafter/MultiNode-Plugin/issues)
