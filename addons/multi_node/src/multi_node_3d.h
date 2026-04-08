#ifndef MULTI_NODE_3D_H
#define MULTI_NODE_3D_H

#include "multi_node_sub.h"
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {

/// Base class for spatial MultiNode children (Mesh, Collider, Label, etc.).
/// Adds a transform offset (position, rotation, scale) applied to each instance,
/// displayed in the inspector like a standard Node3D transform.
class MultiNode3D : public MultiNodeSub {
	GDCLASS(MultiNode3D, MultiNodeSub)

protected:
	static void _bind_methods();

	Transform3D _transform_offset;
	bool _has_offset = false; // Cached: _transform_offset != identity.

private:
	Vector3 _offset_position;
	Vector3 _offset_rotation; // Radians internally, degrees in inspector.
	Vector3 _offset_scale = Vector3(1, 1, 1);
	bool _use_instance_basis = true;

	void _update_transform_offset();

public:
	MultiNode3D();
	~MultiNode3D();

	PackedStringArray _get_configuration_warnings() const override;

	void set_offset_position(const Vector3 &p_pos);
	Vector3 get_offset_position() const;

	void set_offset_rotation(const Vector3 &p_rot);
	Vector3 get_offset_rotation() const;

	void set_offset_scale(const Vector3 &p_scale);
	Vector3 get_offset_scale() const;

	void set_use_instance_basis(bool p_use);
	bool get_use_instance_basis() const;

	Transform3D get_transform_offset() const;

	/// Get the final transform for an instance, applying offset and basis flag.
	Transform3D compute_instance_transform(const Transform3D &p_instance_xform) const;
};

} // namespace godot

#endif // MULTI_NODE_3D_H
