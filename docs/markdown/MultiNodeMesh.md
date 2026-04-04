# MultiNodeMesh

**Inherits:** MultiNodeVisual3D

Renders a mesh for each instance using GPU-instanced MultiMesh.

## Description

MultiNodeMesh renders a shared mesh for every active instance in the parent MultiNode using a single MultiMesh draw call. Inactive instances are hidden via zero-scale transform. Supports per-instance color modulation and material overrides. This is the primary visual node for most use cases.

## Properties

| Name | Type | Default |
|------|------|---------|
| [mesh](#mesh) | Mesh | `null` |
| [material_override](#material_override) | Material | `null` |
| [use_instance_colors](#use_instance_colors) | bool | `false` |

## Property Descriptions

### mesh

- **Type:** Mesh
- **Default:** `null`
- **Setter:** `set_mesh(value)`
- **Getter:** `get_mesh()`

The mesh resource rendered for each instance. All instances share this single mesh. Assign any Mesh resource (BoxMesh, SphereMesh, imported .obj, etc.).

---

### material_override

- **Type:** Material
- **Default:** `null`
- **Setter:** `set_material_override(value)`
- **Getter:** `get_material_override()`

Optional material that overrides the mesh's built-in material for all instances. Leave empty to use the mesh's own material.

---

### use_instance_colors

- **Type:** bool
- **Default:** `false`
- **Setter:** `set_use_instance_colors(value)`
- **Getter:** `get_use_instance_colors()`

When enabled, per-instance colors from the MultiNodeVisual3D base class are passed to the MultiMesh. Your shader must read `COLOR` or `INSTANCE_COLOR` to use them. Enabling this allocates an extra 4 floats per instance in the GPU buffer.
