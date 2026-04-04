#include "multi_node_instance_data.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void MultiNodeInstanceData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_transforms", "transforms"), &MultiNodeInstanceData::set_transforms);
	ClassDB::bind_method(D_METHOD("get_transforms"), &MultiNodeInstanceData::get_transforms);
	ClassDB::bind_method(D_METHOD("set_custom_data", "data"), &MultiNodeInstanceData::set_custom_data);
	ClassDB::bind_method(D_METHOD("get_custom_data"), &MultiNodeInstanceData::get_custom_data);
	ClassDB::bind_method(D_METHOD("get_instance_count"), &MultiNodeInstanceData::get_instance_count);
	ClassDB::bind_method(D_METHOD("set_transforms_typed", "transforms"), &MultiNodeInstanceData::set_transforms_typed);
	ClassDB::bind_method(D_METHOD("get_transforms_typed"), &MultiNodeInstanceData::get_transforms_typed);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "transforms",
					 PROPERTY_HINT_ARRAY_TYPE, "Transform3D"),
			"set_transforms", "get_transforms");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "custom_data"),
			"set_custom_data", "get_custom_data");
}

void MultiNodeInstanceData::set_transforms(const Array &p_transforms) {
	_transforms = p_transforms;
}

Array MultiNodeInstanceData::get_transforms() const {
	return _transforms;
}

void MultiNodeInstanceData::set_custom_data(const PackedFloat32Array &p_data) {
	_custom_data = p_data;
}

PackedFloat32Array MultiNodeInstanceData::get_custom_data() const {
	return _custom_data;
}

int MultiNodeInstanceData::get_instance_count() const {
	return _transforms.size();
}

void MultiNodeInstanceData::set_transforms_typed(const TypedArray<Transform3D> &p_transforms) {
	_transforms = p_transforms;
}

TypedArray<Transform3D> MultiNodeInstanceData::get_transforms_typed() const {
	TypedArray<Transform3D> out;
	out.resize(_transforms.size());
	for (int i = 0; i < _transforms.size(); i++) {
		out[i] = _transforms[i];
	}
	return out;
}
