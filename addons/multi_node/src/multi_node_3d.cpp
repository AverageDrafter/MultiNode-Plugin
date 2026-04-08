#include "multi_node_3d.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

MultiNode3D::MultiNode3D() {}
MultiNode3D::~MultiNode3D() {}

PackedStringArray MultiNode3D::_get_configuration_warnings() const {
	PackedStringArray warnings = MultiNodeSub::_get_configuration_warnings();
	if (_parent && !_parent->get_use_transforms()) {
		warnings.push_back("This node requires transforms, but the parent MultiNode has use_transforms disabled. It will not function correctly.");
	}
	return warnings;
}

void MultiNode3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_offset_position", "position"), &MultiNode3D::set_offset_position);
	ClassDB::bind_method(D_METHOD("get_offset_position"), &MultiNode3D::get_offset_position);

	ClassDB::bind_method(D_METHOD("set_offset_rotation", "rotation"), &MultiNode3D::set_offset_rotation);
	ClassDB::bind_method(D_METHOD("get_offset_rotation"), &MultiNode3D::get_offset_rotation);

	ClassDB::bind_method(D_METHOD("set_offset_scale", "scale"), &MultiNode3D::set_offset_scale);
	ClassDB::bind_method(D_METHOD("get_offset_scale"), &MultiNode3D::get_offset_scale);

	ClassDB::bind_method(D_METHOD("set_use_instance_basis", "use"), &MultiNode3D::set_use_instance_basis);
	ClassDB::bind_method(D_METHOD("get_use_instance_basis"), &MultiNode3D::get_use_instance_basis);

	ClassDB::bind_method(D_METHOD("get_transform_offset"), &MultiNode3D::get_transform_offset);

	ADD_GROUP("Transform Offset", "offset_");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "offset_position", PROPERTY_HINT_NONE, "suffix:m"), "set_offset_position", "get_offset_position");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "offset_rotation", PROPERTY_HINT_NONE, "radians_as_degrees"), "set_offset_rotation", "get_offset_rotation");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "offset_scale"), "set_offset_scale", "get_offset_scale");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_instance_basis"), "set_use_instance_basis", "get_use_instance_basis");
}

void MultiNode3D::_update_transform_offset() {
	Basis basis;
	basis.set_euler(_offset_rotation);
	basis.scale_local(_offset_scale);
	_transform_offset = Transform3D(basis, _offset_position);
	_has_offset = (_transform_offset != Transform3D());
	_on_active_changed();
}

void MultiNode3D::set_offset_position(const Vector3 &p_pos) {
	_offset_position = p_pos;
	_update_transform_offset();
}

Vector3 MultiNode3D::get_offset_position() const { return _offset_position; }

void MultiNode3D::set_offset_rotation(const Vector3 &p_rot) {
	_offset_rotation = p_rot;
	_update_transform_offset();
}

Vector3 MultiNode3D::get_offset_rotation() const { return _offset_rotation; }

void MultiNode3D::set_offset_scale(const Vector3 &p_scale) {
	_offset_scale = p_scale;
	_update_transform_offset();
}

Vector3 MultiNode3D::get_offset_scale() const { return _offset_scale; }

void MultiNode3D::set_use_instance_basis(bool p_use) {
	_use_instance_basis = p_use;
	_on_active_changed();
}

bool MultiNode3D::get_use_instance_basis() const { return _use_instance_basis; }

Transform3D MultiNode3D::get_transform_offset() const { return _transform_offset; }

Transform3D MultiNode3D::compute_instance_transform(const Transform3D &p_instance_xform) const {
	Transform3D base = p_instance_xform;
	if (!_use_instance_basis) {
		// Strip rotation/scale — only keep position.
		base = Transform3D(Basis(), base.origin);
	}
	if (_has_offset) {
		base = base * _transform_offset;
	}
	return base;
}
