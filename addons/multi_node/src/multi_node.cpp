#include "multi_node.h"
#include "multi_node_collider.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>

using namespace godot;

MultiNode::MultiNode() {}
MultiNode::~MultiNode() {}

void MultiNode::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			set_process_internal(true);
		} break;

		case NOTIFICATION_READY: {
			// set_all_transforms fires instances_changed during property loading,
			// before children are in the tree. Re-emit now that all children
			// are connected so they can sync the loaded transforms.
			if (_instance_count > 0) {
				mark_all_dirty();
				_emit_and_clear_dirty();
			}
		} break;

		case NOTIFICATION_INTERNAL_PROCESS: {
			// Auto-flush: if transforms were changed outside batch mode,
			// emit one signal at end of frame instead of per-call.
			if (_auto_dirty && !_batch_mode) {
				_auto_dirty = false;
				_emit_and_clear_dirty();
			}
		} break;
	}
}

// ---------------------------------------------------------------------------
// use_transforms
// ---------------------------------------------------------------------------

void MultiNode::set_use_transforms(bool p_enable) {
	_use_transforms = p_enable;
	update_configuration_warnings();
	// Notify children so they can update their own warnings.
	for (int i = 0; i < get_child_count(); i++) {
		Node *child = get_child(i);
		if (child) child->update_configuration_warnings();
	}
}

bool MultiNode::get_use_transforms() const {
	return _use_transforms;
}

PackedStringArray MultiNode::_get_configuration_warnings() const {
	PackedStringArray warnings;
	if (!_use_transforms) {
		warnings.push_back("Transforms are disabled. Sub-nodes that require spatial data (MultiNode3D and its children: Mesh, Collider, Audio, etc.) will not function. Only MultiNodeSub-level features (data arrays, compute shaders) are available.");
	}
	return warnings;
}

void MultiNode::_bind_methods() {
	// --- Signals ---
	ADD_SIGNAL(MethodInfo("instances_changed"));

	// --- Properties ---
	ClassDB::bind_method(D_METHOD("set_use_transforms", "enable"), &MultiNode::set_use_transforms);
	ClassDB::bind_method(D_METHOD("get_use_transforms"), &MultiNode::get_use_transforms);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_transforms"), "set_use_transforms", "get_use_transforms");

	ClassDB::bind_method(D_METHOD("set_instance_count", "count"), &MultiNode::set_instance_count);
	ClassDB::bind_method(D_METHOD("get_instance_count"), &MultiNode::get_instance_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "instance_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "set_instance_count", "get_instance_count");

	// Serializes the full transform array into the scene file (storage only, hidden from inspector).
	// Uses plain Array wrappers — TypedArray<Transform3D> doesn't match Variant::ARRAY in property lookup.
	ClassDB::bind_method(D_METHOD("_set_transform_buffer", "arr"), &MultiNode::_set_transform_buffer);
	ClassDB::bind_method(D_METHOD("_get_transform_buffer_arr"), &MultiNode::_get_transform_buffer_arr);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "transform_buffer", PROPERTY_HINT_ARRAY_TYPE, "Transform3D",
					 PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_NO_EDITOR),
			"_set_transform_buffer", "_get_transform_buffer_arr");

	ClassDB::bind_method(D_METHOD("set_instance_limit", "limit"), &MultiNode::set_instance_limit);
	ClassDB::bind_method(D_METHOD("get_instance_limit"), &MultiNode::get_instance_limit);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "instance_limit", PROPERTY_HINT_RANGE, "0,1000000,1,or_greater"), "set_instance_limit", "get_instance_limit");

	// --- Instance management ---
	ClassDB::bind_method(D_METHOD("add_instance", "transform"), &MultiNode::add_instance, DEFVAL(Transform3D()));
	ClassDB::bind_method(D_METHOD("remove_instance", "index"), &MultiNode::remove_instance);
	ClassDB::bind_method(D_METHOD("clear_instances"), &MultiNode::clear_instances);

	// --- Transform API ---
	ClassDB::bind_method(D_METHOD("set_instance_transform", "index", "transform"), &MultiNode::set_instance_transform);
	ClassDB::bind_method(D_METHOD("get_instance_transform", "index"), &MultiNode::get_instance_transform);
	ClassDB::bind_method(D_METHOD("set_all_transforms", "transforms"), &MultiNode::set_all_transforms);
	ClassDB::bind_method(D_METHOD("set_positions", "positions"), &MultiNode::set_positions);
	ClassDB::bind_method(D_METHOD("get_all_positions"), &MultiNode::get_all_positions);
	ClassDB::bind_method(D_METHOD("get_transform_buffer"), &MultiNode::get_transform_buffer);

	// --- Dirty tracking ---
	ClassDB::bind_method(D_METHOD("mark_dirty", "index"), &MultiNode::mark_dirty);
	ClassDB::bind_method(D_METHOD("mark_all_dirty"), &MultiNode::mark_all_dirty);
	ClassDB::bind_method(D_METHOD("is_dirty", "index"), &MultiNode::is_dirty);
	ClassDB::bind_method(D_METHOD("get_dirty_count"), &MultiNode::get_dirty_count);

	// --- Batch mode ---
	ClassDB::bind_method(D_METHOD("begin_batch"), &MultiNode::begin_batch);
	ClassDB::bind_method(D_METHOD("end_batch"), &MultiNode::end_batch);
	ClassDB::bind_method(D_METHOD("notify_transforms_changed"), &MultiNode::notify_transforms_changed);

	// --- Per-instance data ---
	ClassDB::bind_method(D_METHOD("set_data_stride", "stride"), &MultiNode::set_data_stride);
	ClassDB::bind_method(D_METHOD("get_data_stride"), &MultiNode::get_data_stride);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "data_stride", PROPERTY_HINT_RANGE, "1,64,1"), "set_data_stride", "get_data_stride");

	ClassDB::bind_method(D_METHOD("set_instance_data", "index", "channel", "value"), &MultiNode::set_instance_data);
	ClassDB::bind_method(D_METHOD("get_instance_data", "index", "channel"), &MultiNode::get_instance_data);
	ClassDB::bind_method(D_METHOD("set_instance_data_vec", "index", "values"), &MultiNode::set_instance_data_vec);
	ClassDB::bind_method(D_METHOD("get_instance_data_vec", "index"), &MultiNode::get_instance_data_vec);

	// --- Sub-node data stride ---
	ClassDB::bind_method(D_METHOD("set_sub_data_stride", "stride"), &MultiNode::set_sub_data_stride);
	ClassDB::bind_method(D_METHOD("get_sub_data_stride"), &MultiNode::get_sub_data_stride);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sub_data_stride", PROPERTY_HINT_RANGE, "4,64,4"), "set_sub_data_stride", "get_sub_data_stride");

	// --- Instance velocity bus ---
	ClassDB::bind_method(D_METHOD("set_instance_linear_velocity", "index", "velocity"), &MultiNode::set_instance_linear_velocity);
	ClassDB::bind_method(D_METHOD("get_instance_linear_velocity", "index"), &MultiNode::get_instance_linear_velocity);
	ClassDB::bind_method(D_METHOD("set_instance_angular_velocity", "index", "velocity"), &MultiNode::set_instance_angular_velocity);
	ClassDB::bind_method(D_METHOD("get_instance_angular_velocity", "index"), &MultiNode::get_instance_angular_velocity);

	// --- Physics helpers ---
	ClassDB::bind_method(D_METHOD("apply_impulse", "index", "impulse"), &MultiNode::apply_impulse);
	ClassDB::bind_method(D_METHOD("apply_force", "index", "force"), &MultiNode::apply_force);
	ClassDB::bind_method(D_METHOD("set_linear_velocity", "index", "velocity"), &MultiNode::set_linear_velocity);

}

// ---------------------------------------------------------------------------
// Instance count
// ---------------------------------------------------------------------------

void MultiNode::set_instance_count(int p_count) {
	int clamped = MAX(0, p_count);
	if (_instance_limit > 0 && clamped > _instance_limit) {
		clamped = _instance_limit;
	}
	int old_count = _instance_count;
	_instance_count = clamped;
	_resize_buffers(old_count, _instance_count);
	mark_all_dirty();
	_emit_and_clear_dirty();
}

int MultiNode::get_instance_count() const {
	return _instance_count;
}

// ---------------------------------------------------------------------------
// Instance limit
// ---------------------------------------------------------------------------

void MultiNode::set_instance_limit(int p_limit) {
	_instance_limit = MAX(0, p_limit);
	if (_instance_limit > 0 && _instance_count > _instance_limit) {
		set_instance_count(_instance_limit);
	}
}

int MultiNode::get_instance_limit() const {
	return _instance_limit;
}

// ---------------------------------------------------------------------------
// Add / remove / clear
// ---------------------------------------------------------------------------

int MultiNode::add_instance(const Transform3D &p_transform) {
	if (_instance_limit > 0 && _instance_count >= _instance_limit) {
		return -1;
	}

	int index = _instance_count;
	_instance_count++;

	if (_use_transforms) {
		_transforms.push_back(p_transform);
	}
	_dirty_flags.push_back(1);
	_dirty_count++;

	// Grow instance data to match the new count.
	int old_data_size = _instance_data.size();
	_instance_data.resize(_instance_count * _data_stride);
	for (int i = old_data_size; i < _instance_data.size(); i++) {
		_instance_data.set(i, 0.0f);
	}

	// Grow velocity bus.
	_instance_lin_vel.push_back(Vector3());
	_instance_ang_vel.push_back(Vector3());

	if (_batch_mode) {
		// Deferred to end_batch().
	} else {
		_auto_dirty = true;
	}
	return index;
}

void MultiNode::remove_instance(int p_index) {
	if (p_index < 0 || p_index >= _instance_count) {
		return;
	}

	int last = _instance_count - 1;
	if (p_index != last) {
		// Swap-and-pop: move last instance into the removed slot.
		if (_use_transforms) {
			_transforms.write[p_index] = _transforms[last];
		}

		// Swap instance data too.
		for (int c = 0; c < _data_stride; c++) {
			_instance_data.set(p_index * _data_stride + c,
				_instance_data[last * _data_stride + c]);
		}

		// Swap velocity bus.
		_instance_lin_vel.write[p_index] = _instance_lin_vel[last];
		_instance_ang_vel.write[p_index] = _instance_ang_vel[last];
	}

	_instance_count--;
	if (_use_transforms) {
		_transforms.resize(_instance_count);
	}
	_dirty_flags.resize(_instance_count);
	_instance_data.resize(_instance_count * _data_stride);
	_instance_lin_vel.resize(_instance_count);
	_instance_ang_vel.resize(_instance_count);

	mark_all_dirty();
	if (_batch_mode) {
		// Deferred to end_batch().
	} else {
		_auto_dirty = true;
	}
}

void MultiNode::clear_instances() {
	_instance_count = 0;
	if (_use_transforms) {
		_transforms.clear();
	}
	_dirty_flags.resize(0);
	_dirty_count = 0;
	_instance_data.resize(0);
	_instance_lin_vel.resize(0);
	_instance_ang_vel.resize(0);
	_emit_and_clear_dirty();
}

// ---------------------------------------------------------------------------
// Internal buffer management
// ---------------------------------------------------------------------------

void MultiNode::_resize_buffers(int p_old_count, int p_new_count) {
	if (_use_transforms) {
		_transforms.resize(p_new_count);
		for (int i = p_old_count; i < p_new_count; i++) {
			_transforms.write[i] = Transform3D();
		}
	}

	_dirty_flags.resize(p_new_count);
	for (int i = p_old_count; i < p_new_count; i++) {
		_dirty_flags.set(i, 0);
	}

	// Resize instance data.
	int old_data_size = _instance_data.size();
	int new_data_size = p_new_count * _data_stride;
	_instance_data.resize(new_data_size);
	for (int i = old_data_size; i < new_data_size; i++) {
		_instance_data.set(i, 0.0f);
	}

	// Resize velocity bus.
	_instance_lin_vel.resize(p_new_count);
	_instance_ang_vel.resize(p_new_count);
	for (int i = p_old_count; i < p_new_count; i++) {
		_instance_lin_vel.write[i] = Vector3();
		_instance_ang_vel.write[i] = Vector3();
	}
}

// ---------------------------------------------------------------------------
// Dirty tracking
// ---------------------------------------------------------------------------

void MultiNode::mark_dirty(int p_index) {
	if (p_index >= 0 && p_index < _instance_count && _dirty_flags[p_index] == 0) {
		_dirty_flags.set(p_index, 1);
		_dirty_count++;
	}
}

void MultiNode::mark_all_dirty() {
	_dirty_flags.fill(1);
	_dirty_count = _instance_count;
}

bool MultiNode::is_dirty(int p_index) const {
	if (p_index < 0 || p_index >= _instance_count) {
		return false;
	}
	return _dirty_flags[p_index] == 1;
}

int MultiNode::get_dirty_count() const {
	return _dirty_count;
}

void MultiNode::_clear_dirty() {
	if (_dirty_count > 0) {
		_dirty_flags.fill(0);
		_dirty_count = 0;
	}
}

void MultiNode::_emit_and_clear_dirty() {
	_auto_dirty = false; // Clear deferred flag — we're emitting now.
	emit_signal("instances_changed");
	_clear_dirty();
}

// ---------------------------------------------------------------------------
// Transform API
// ---------------------------------------------------------------------------

void MultiNode::set_instance_transform(int p_index, const Transform3D &p_transform) {
	if (!_use_transforms || p_index < 0 || p_index >= _instance_count) {
		return;
	}
	_transforms.write[p_index] = p_transform;
	mark_dirty(p_index);
	if (_batch_mode) {
		// Explicit batch — signal deferred to end_batch().
	} else {
		// Auto-batch — signal deferred to end of frame.
		_auto_dirty = true;
	}
}

Transform3D MultiNode::get_instance_transform(int p_index) const {
	if (!_use_transforms || p_index < 0 || p_index >= _instance_count) {
		return Transform3D();
	}
	return _transforms[p_index];
}

void MultiNode::set_all_transforms(const TypedArray<Transform3D> &p_transforms) {
	if (!_use_transforms) return;
	set_instance_count(p_transforms.size());
	for (int i = 0; i < _instance_count; i++) {
		_transforms.write[i] = p_transforms[i];
	}
	mark_all_dirty();
	_emit_and_clear_dirty();
}

void MultiNode::set_positions(const PackedVector3Array &p_positions) {
	if (!_use_transforms) return;
	set_instance_count(p_positions.size());
	for (int i = 0; i < _instance_count; i++) {
		_transforms.write[i] = Transform3D(Basis(), p_positions[i]);
	}
	mark_all_dirty();
	_emit_and_clear_dirty();
}

PackedVector3Array MultiNode::get_all_positions() const {
	PackedVector3Array result;
	if (!_use_transforms) return result;
	result.resize(_instance_count);
	for (int i = 0; i < _instance_count; i++) {
		result.set(i, _transforms[i].origin);
	}
	return result;
}

TypedArray<Transform3D> MultiNode::get_transform_buffer() const {
	TypedArray<Transform3D> result;
	if (!_use_transforms) return result;
	result.resize(_instance_count);
	for (int i = 0; i < _instance_count; i++) {
		result[i] = _transforms[i];
	}
	return result;
}

// Plain Array wrappers for scene serialization (ADD_PROPERTY can't match TypedArray).
void MultiNode::_set_transform_buffer(const Array &p_arr) {
	TypedArray<Transform3D> typed;
	typed.resize(p_arr.size());
	for (int i = 0; i < p_arr.size(); i++) {
		typed[i] = p_arr[i];
	}
	set_all_transforms(typed);
}

Array MultiNode::_get_transform_buffer_arr() const {
	Array result;
	result.resize(_instance_count);
	for (int i = 0; i < _instance_count; i++) {
		result[i] = _transforms[i];
	}
	return result;
}

// ---------------------------------------------------------------------------
// Batch mode
// ---------------------------------------------------------------------------

void MultiNode::begin_batch() {
	_batch_mode = true;
}

void MultiNode::end_batch() {
	_batch_mode = false;
	_emit_and_clear_dirty();
}

void MultiNode::notify_transforms_changed() {
	_emit_and_clear_dirty();
}

// ---------------------------------------------------------------------------
// Direct access for C++ children
// ---------------------------------------------------------------------------

const Vector<Transform3D> &MultiNode::get_transforms_internal() const {
	return _transforms;
}

const PackedByteArray &MultiNode::get_dirty_flags_internal() const {
	return _dirty_flags;
}

// ---------------------------------------------------------------------------
// Per-instance data
// ---------------------------------------------------------------------------

void MultiNode::set_data_stride(int p_stride) {
	_data_stride = MAX(1, p_stride);
	// Re-resize data to match new stride.
	int new_size = _instance_count * _data_stride;
	int old_size = _instance_data.size();
	_instance_data.resize(new_size);
	for (int i = old_size; i < new_size; i++) {
		_instance_data.set(i, 0.0f);
	}
}

int MultiNode::get_data_stride() const {
	return _data_stride;
}

void MultiNode::set_instance_data(int p_index, int p_channel, float p_value) {
	int offset = p_index * _data_stride + p_channel;
	if (offset >= 0 && offset < _instance_data.size() && p_channel >= 0 && p_channel < _data_stride) {
		_instance_data.set(offset, p_value);
	}
}

float MultiNode::get_instance_data(int p_index, int p_channel) const {
	int offset = p_index * _data_stride + p_channel;
	if (offset >= 0 && offset < _instance_data.size() && p_channel >= 0 && p_channel < _data_stride) {
		return _instance_data[offset];
	}
	return 0.0f;
}

void MultiNode::set_instance_data_vec(int p_index, const PackedFloat32Array &p_values) {
	int base = p_index * _data_stride;
	int to_copy = MIN(p_values.size(), _data_stride);
	if (base < 0 || base + to_copy > _instance_data.size()) return;
	for (int i = 0; i < to_copy; i++) {
		_instance_data.set(base + i, p_values[i]);
	}
}

PackedFloat32Array MultiNode::get_instance_data_vec(int p_index) const {
	PackedFloat32Array result;
	int base = p_index * _data_stride;
	if (base < 0 || base + _data_stride > _instance_data.size()) return result;
	result.resize(_data_stride);
	for (int i = 0; i < _data_stride; i++) {
		result.set(i, _instance_data[base + i]);
	}
	return result;
}

const PackedFloat32Array &MultiNode::get_instance_data_internal() const {
	return _instance_data;
}

// ---------------------------------------------------------------------------
// Sub-node data stride
// ---------------------------------------------------------------------------

void MultiNode::set_sub_data_stride(int p_stride) {
	// Enforce multiples of 4, minimum 4.
	_sub_data_stride = MAX(4, ((p_stride + 3) / 4) * 4);
}

int MultiNode::get_sub_data_stride() const {
	return _sub_data_stride;
}

// ---------------------------------------------------------------------------
// Instance velocity bus
// ---------------------------------------------------------------------------

void MultiNode::set_instance_linear_velocity(int p_index, const Vector3 &p_vel) {
	if (p_index >= 0 && p_index < _instance_count) {
		_instance_lin_vel.write[p_index] = p_vel;
	}
}

Vector3 MultiNode::get_instance_linear_velocity(int p_index) const {
	if (p_index >= 0 && p_index < _instance_lin_vel.size()) {
		return _instance_lin_vel[p_index];
	}
	return Vector3();
}

void MultiNode::set_instance_angular_velocity(int p_index, const Vector3 &p_vel) {
	if (p_index >= 0 && p_index < _instance_count) {
		_instance_ang_vel.write[p_index] = p_vel;
	}
}

Vector3 MultiNode::get_instance_angular_velocity(int p_index) const {
	if (p_index >= 0 && p_index < _instance_ang_vel.size()) {
		return _instance_ang_vel[p_index];
	}
	return Vector3();
}

// ---------------------------------------------------------------------------
// Physics helpers — route to the active collider for a given instance
// ---------------------------------------------------------------------------

static RID _find_active_body(MultiNode *p_node, int p_index) {
	for (int i = 0; i < p_node->get_child_count(); i++) {
		MultiNodeCollider *collider = Object::cast_to<MultiNodeCollider>(p_node->get_child(i));
		if (collider && collider->is_instance_in_space(p_index)) {
			RID body = collider->get_body_rid(p_index);
			if (body.is_valid()) {
				return body;
			}
		}
	}
	return RID();
}

void MultiNode::apply_impulse(int p_index, const Vector3 &p_impulse) {
	if (p_index < 0 || p_index >= _instance_count) return;
	RID body = _find_active_body(this, p_index);
	if (body.is_valid()) {
		PhysicsServer3D::get_singleton()->body_apply_central_impulse(body, p_impulse);
	}
}

void MultiNode::apply_force(int p_index, const Vector3 &p_force) {
	if (p_index < 0 || p_index >= _instance_count) return;
	RID body = _find_active_body(this, p_index);
	if (body.is_valid()) {
		PhysicsServer3D::get_singleton()->body_apply_central_force(body, p_force);
	}
}

void MultiNode::set_linear_velocity(int p_index, const Vector3 &p_velocity) {
	if (p_index < 0 || p_index >= _instance_count) return;
	RID body = _find_active_body(this, p_index);
	if (body.is_valid()) {
		PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
		ps->body_set_state(body, PhysicsServer3D::BODY_STATE_SLEEPING, false);
		ps->body_set_state(body, PhysicsServer3D::BODY_STATE_LINEAR_VELOCITY, p_velocity);
	}
}

// (Instance generator code moved to MultiNodeInstanceGen.)


