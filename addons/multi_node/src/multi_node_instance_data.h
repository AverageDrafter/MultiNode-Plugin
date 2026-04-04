#ifndef MULTI_NODE_INSTANCE_DATA_H
#define MULTI_NODE_INSTANCE_DATA_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {

/// Serializable instance snapshot for MultiNode.
/// Stores an array of Transform3D for each instance, and an optional flat
/// PackedFloat32Array for per-instance custom data (stride matches MultiNode.data_stride).
/// Save as .tres / .res and drag onto the inspector Database picker to load.
class MultiNodeInstanceData : public Resource {
	GDCLASS(MultiNodeInstanceData, Resource)

	Array _transforms;               // Array[Transform3D]
	PackedFloat32Array _custom_data; // flat: instance_count * data_stride floats

protected:
	static void _bind_methods();

public:
	void set_transforms(const Array &p_transforms);
	Array get_transforms() const;

	void set_custom_data(const PackedFloat32Array &p_data);
	PackedFloat32Array get_custom_data() const;

	int get_instance_count() const;

	// Convenience: build resource from a TypedArray<Transform3D>
	void set_transforms_typed(const TypedArray<Transform3D> &p_transforms);
	TypedArray<Transform3D> get_transforms_typed() const;
};

} // namespace godot

#endif // MULTI_NODE_INSTANCE_DATA_H
