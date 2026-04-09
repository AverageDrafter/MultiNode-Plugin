#ifndef MULTI_NODE_MESH_H
#define MULTI_NODE_MESH_H

#include "multi_node_visual_3d.h"
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {

/// Renders a mesh for each active instance in the parent MultiNode.
/// Uses RenderingServer directly. Per-instance colors live on the
/// MultiNodeVisual3D base class.
/// Inactive instances are hidden via zero-scale transform.
class MultiNodeMesh : public MultiNodeVisual3D {
	GDCLASS(MultiNodeMesh, MultiNodeVisual3D)

protected:
	static void _bind_methods();
	void _notification(int p_what);

	void _on_sub_entered() override;
	void _on_sub_exiting() override;
	void _on_parent_sync() override;
	void _on_active_changed() override;
	RID _get_visual_instance_rid() const override;

	void _on_instance_color_changed(int p_index, const Color &p_color) override;
	void _on_colors_changed() override;

public:
	MultiNodeMesh();
	~MultiNodeMesh();

	void set_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_material_override(const Ref<Material> &p_material);
	Ref<Material> get_material_override() const;

	void set_use_instance_colors(bool p_enable);
	bool get_use_instance_colors() const;

private:
	Ref<Mesh> _mesh;
	Ref<Material> _material_override;
	bool _use_instance_colors = true;

	RID _multimesh;
	RID _instance;
	int _current_count = 0;

	void _rebuild();
	void _cleanup_rids();
	void _sync_transforms();
	void _sync_colors();
};

} // namespace godot

#endif // MULTI_NODE_MESH_H
