#ifndef MULTI_NODE_MARKER_H
#define MULTI_NODE_MARKER_H

#include "multi_node_3d.h"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {

/// Renders a visible marker at each instance position.
/// Useful for visualizing invisible instances (Audio, Collider, Area, etc.)
/// and for demonstrating what instances are. Editor-only by default.
///
/// Generates procedural wireframe geometry: Cross, Diamond, Sphere, or Axes.
/// Uses an unshaded, optionally depth-test-disabled material — draws on top
/// of geometry when always_on_top is enabled.
class MultiNodeMarker : public MultiNode3D {
	GDCLASS(MultiNodeMarker, MultiNode3D)

public:
	enum MarkerShape {
		SHAPE_CROSS   = 0, // 3-axis cross — minimal, readable at any size
		SHAPE_DIAMOND = 1, // Wireframe octahedron — classic position marker
		SHAPE_SPHERE  = 2, // Three orthogonal circles — volume/radius hint
		SHAPE_AXES    = 3, // RGB XYZ arrows — shows instance orientation
	};

protected:
	static void _bind_methods();
	void _notification(int p_what);

	void _on_sub_entered() override;
	void _on_sub_exiting() override;
	void _on_parent_sync() override;
	void _on_active_changed() override;

public:
	MultiNodeMarker();
	~MultiNodeMarker();

	void set_shape(MarkerShape p_shape);
	MarkerShape get_shape() const;

	void set_marker_size(float p_size);
	float get_marker_size() const;

	void set_marker_color(const Color &p_color);
	Color get_marker_color() const;

	void set_editor_only(bool p_editor_only);
	bool get_editor_only() const;

	void set_always_on_top(bool p_always_on_top);
	bool get_always_on_top() const;

private:
	MarkerShape _shape        = SHAPE_CROSS;
	float       _marker_size  = 0.5f;
	Color       _marker_color = Color(0.0f, 1.0f, 0.8f, 1.0f); // Teal — distinct from Godot gizmos
	bool        _editor_only    = true;
	bool        _always_on_top  = true;

	Ref<ArrayMesh>        _marker_mesh;
	Ref<StandardMaterial3D> _marker_material;
	RID _multimesh;
	RID _instance;
	int _current_count = 0;

	Ref<ArrayMesh>          _generate_mesh() const;
	Ref<StandardMaterial3D> _create_material() const;

	void _rebuild();
	void _cleanup_rids();
	void _sync_transforms();
};

} // namespace godot

VARIANT_ENUM_CAST(MultiNodeMarker::MarkerShape);

#endif // MULTI_NODE_MARKER_H
