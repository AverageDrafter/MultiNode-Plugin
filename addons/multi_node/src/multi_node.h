#ifndef MULTI_NODE_H
#define MULTI_NODE_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

/// Compositional instancing parent. Owns the master instance list (transforms).
/// Child nodes (extending MultiNodeSub) define what each instance IS and DOES,
/// and own their own per-instance data arrays.
class MultiNode : public Node3D {
	GDCLASS(MultiNode, Node3D)

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	MultiNode();
	~MultiNode();

	PackedStringArray _get_configuration_warnings() const override;

	void set_use_transforms(bool p_enable);
	bool get_use_transforms() const;

	// --- Instance management ---
	void set_instance_count(int p_count);
	int get_instance_count() const;

	void set_instance_limit(int p_limit);
	int get_instance_limit() const;

	int add_instance(const Transform3D &p_transform = Transform3D());
	void remove_instance(int p_index);
	void clear_instances();

	// --- Transform API ---
	void set_instance_transform(int p_index, const Transform3D &p_transform);
	Transform3D get_instance_transform(int p_index) const;

	// --- Instance velocity bus (shared linear/angular velocity across collider swaps) ---
	void set_instance_linear_velocity(int p_index, const Vector3 &p_vel);
	Vector3 get_instance_linear_velocity(int p_index) const;
	void set_instance_angular_velocity(int p_index, const Vector3 &p_vel);
	Vector3 get_instance_angular_velocity(int p_index) const;
	void set_all_transforms(const TypedArray<Transform3D> &p_transforms);
	void set_positions(const PackedVector3Array &p_positions);
	PackedVector3Array get_all_positions() const;
	TypedArray<Transform3D> get_transform_buffer() const;

	// --- Dirty tracking ---
	void mark_dirty(int p_index);
	void mark_all_dirty();
	bool is_dirty(int p_index) const;
	int get_dirty_count() const;

	// --- Batch mode ---
	void begin_batch();
	void end_batch();
	void notify_transforms_changed();

	// --- Per-instance data (shared, parent-level) ---
	void set_data_stride(int p_stride);
	int get_data_stride() const;

	void set_instance_data(int p_index, int p_channel, float p_value);
	float get_instance_data(int p_index, int p_channel) const;

	void set_instance_data_vec(int p_index, const PackedFloat32Array &p_values);
	PackedFloat32Array get_instance_data_vec(int p_index) const;

	const PackedFloat32Array &get_instance_data_internal() const;

	// --- Sub-node data stride (controls all sub-node data arrays, multiples of 4) ---
	void set_sub_data_stride(int p_stride);
	int get_sub_data_stride() const;

	// --- Physics helpers (route to active collider child) ---
	void apply_impulse(int p_index, const Vector3 &p_impulse);
	void apply_force(int p_index, const Vector3 &p_force);
	void set_linear_velocity(int p_index, const Vector3 &p_velocity);

	// --- Direct access for C++ children (not bound to GDScript) ---
	const Vector<Transform3D> &get_transforms_internal() const;
	const PackedByteArray &get_dirty_flags_internal() const;

private:
	int _instance_count = 0;
	int _instance_limit = 0;
	bool _use_transforms = true;
	Vector<Transform3D> _transforms;
	PackedByteArray _dirty_flags;
	int _dirty_count = 0;
	bool _batch_mode = false;
	bool _auto_dirty = false; // True if changes were made outside batch mode, deferred to end of frame.

	int _data_stride = 4;
	PackedFloat32Array _instance_data;

	Vector<Vector3> _instance_lin_vel;
	Vector<Vector3> _instance_ang_vel;

	int _sub_data_stride = 4; // Multiples of 4, controls all sub-node data arrays.

	void _resize_buffers(int p_old_count, int p_new_count);
	void _clear_dirty();
	void _emit_and_clear_dirty();

	// Plain Array wrappers used only for scene serialization (ADD_PROPERTY).
	// TypedArray<Transform3D> doesn't match Variant::ARRAY in property lookup.
	void _set_transform_buffer(const Array &p_arr);
	Array _get_transform_buffer_arr() const;
};

} // namespace godot

#endif // MULTI_NODE_H
