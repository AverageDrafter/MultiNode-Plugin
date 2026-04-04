#include "multi_node_collider.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/world3d.hpp>

using namespace godot;

MultiNodeCollider::MultiNodeCollider() {}

MultiNodeCollider::~MultiNodeCollider() {
	_clear_bodies();
}

void MultiNodeCollider::_bind_methods() {
	ADD_GROUP("Physics", "");

	ClassDB::bind_method(D_METHOD("set_shape", "shape"), &MultiNodeCollider::set_shape);
	ClassDB::bind_method(D_METHOD("get_shape"), &MultiNodeCollider::get_shape);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "shape", PROPERTY_HINT_RESOURCE_TYPE, "Shape3D"), "set_shape", "get_shape");

	ClassDB::bind_method(D_METHOD("set_body_type", "type"), &MultiNodeCollider::set_body_type);
	ClassDB::bind_method(D_METHOD("get_body_type"), &MultiNodeCollider::get_body_type);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "body_type", PROPERTY_HINT_ENUM, "Static,Kinematic,Rigid"), "set_body_type", "get_body_type");

	ClassDB::bind_method(D_METHOD("set_self_collision", "enable"), &MultiNodeCollider::set_self_collision);
	ClassDB::bind_method(D_METHOD("get_self_collision"), &MultiNodeCollider::get_self_collision);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "self_collision"), "set_self_collision", "get_self_collision");

	ADD_GROUP("Layers", "collision_");

	ClassDB::bind_method(D_METHOD("set_collision_layer", "layer"), &MultiNodeCollider::set_collision_layer);
	ClassDB::bind_method(D_METHOD("get_collision_layer"), &MultiNodeCollider::get_collision_layer);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_layer", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_layer", "get_collision_layer");

	ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"), &MultiNodeCollider::set_collision_mask);
	ClassDB::bind_method(D_METHOD("get_collision_mask"), &MultiNodeCollider::get_collision_mask);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_mask", "get_collision_mask");

	ADD_GROUP("", "");

	ClassDB::bind_method(D_METHOD("get_instance_from_body", "body_rid"), &MultiNodeCollider::get_instance_from_body);
	ClassDB::bind_method(D_METHOD("get_body_rid", "index"), &MultiNodeCollider::get_body_rid);

	BIND_ENUM_CONSTANT(BODY_TYPE_STATIC);
	BIND_ENUM_CONSTANT(BODY_TYPE_KINEMATIC);
	BIND_ENUM_CONSTANT(BODY_TYPE_RIGID);
}

// ---------------------------------------------------------------------------
// Configuration warnings
// ---------------------------------------------------------------------------

PackedStringArray MultiNodeCollider::_get_configuration_warnings() const {
	PackedStringArray warnings = MultiNode3D::_get_configuration_warnings();
	if (!_self_collision && _body_type == BODY_TYPE_RIGID) {
		warnings.push_back("self_collision is OFF for rigid bodies. Bodies are placed on physics layer 20 to prevent self-collision. External objects must include layer 20 in their collision mask to detect these bodies.");
	}
	return warnings;
}

// ---------------------------------------------------------------------------
// Notification
// ---------------------------------------------------------------------------

void MultiNodeCollider::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_TRANSFORM_CHANGED: {
			_global_inv_dirty = true;
		} break;
	}
}

// ---------------------------------------------------------------------------
// MultiNodeSub virtual overrides
// ---------------------------------------------------------------------------

void MultiNodeCollider::_on_sub_entered() {
	set_notify_transform(true);
	_global_inv_dirty = true;
	_rebuild();
	set_physics_process(true);
}

void MultiNodeCollider::_on_sub_exiting() {
	set_physics_process(false);
	_clear_bodies();
}

void MultiNodeCollider::_on_parent_sync() {
	if (_in_readback) {
		return;
	}
	_sync_bodies();
}

void MultiNodeCollider::_on_active_changed() {
	if (is_inside_tree()) {
		_rebuild();
	}
}

// ---------------------------------------------------------------------------
// Property setters / getters
// ---------------------------------------------------------------------------

void MultiNodeCollider::set_shape(const Ref<Shape3D> &p_shape) {
	_shape = p_shape;
	if (is_inside_tree()) {
		_rebuild();
	}
}

Ref<Shape3D> MultiNodeCollider::get_shape() const { return _shape; }

void MultiNodeCollider::set_body_type(int p_type) {
	_body_type = p_type;
	if (is_inside_tree()) {
		_rebuild();
	}
}

int MultiNodeCollider::get_body_type() const { return _body_type; }
void MultiNodeCollider::set_collision_layer(uint32_t p_layer) {
	_collision_layer = p_layer;
	_update_all_layers();
}
uint32_t MultiNodeCollider::get_collision_layer() const { return _collision_layer; }
void MultiNodeCollider::set_collision_mask(uint32_t p_mask) {
	_collision_mask = p_mask;
	_update_all_layers();
}
uint32_t MultiNodeCollider::get_collision_mask() const { return _collision_mask; }
void MultiNodeCollider::set_self_collision(bool p_enable) {
	_self_collision = p_enable;
	_update_all_layers();
	update_configuration_warnings();
}
bool MultiNodeCollider::get_self_collision() const { return _self_collision; }

void MultiNodeCollider::_compute_effective_layers(uint32_t &r_layer, uint32_t &r_mask) const {
	r_layer = _collision_layer;
	r_mask = _collision_mask;
	// Self-collision override only applies to rigid bodies.
	// Static/kinematic bodies never self-collide in physics engines,
	// so overriding their layers would just hide them from external objects.
	if (!_self_collision && _body_type == BODY_TYPE_RIGID) {
		r_layer = 1 << 19;
		r_mask = _collision_mask & ~(1 << 19);
	}
}

void MultiNodeCollider::_update_all_layers() {
	PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
	if (!ps) return;
	uint32_t effective_layer, effective_mask;
	_compute_effective_layers(effective_layer, effective_mask);
	for (int i = 0; i < _bodies.size(); i++) {
		if (_bodies[i].is_valid()) {
			ps->body_set_collision_layer(_bodies[i], effective_layer);
			ps->body_set_collision_mask(_bodies[i], effective_mask);
		}
	}
}

// ---------------------------------------------------------------------------
// Body management
// ---------------------------------------------------------------------------

void MultiNodeCollider::_clear_bodies() {
	PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
	if (!ps) {
		_bodies.clear();
		_body_to_index.clear();
		_last_world_transforms.clear();
		_in_space.resize(0);
		return;
	}
	for (int i = 0; i < _bodies.size(); i++) {
		if (_bodies[i].is_valid()) {
			ps->free_rid(_bodies[i]);
		}
	}
	_bodies.clear();
	_body_to_index.clear();
	_last_world_transforms.clear();
	_in_space.resize(0);
}

void MultiNodeCollider::_rebuild() {
	_clear_bodies();
	if (!_parent || !_shape.is_valid() || !is_inside_tree()) {
		return;
	}
	_sync_bodies();
}

void MultiNodeCollider::_sync_bodies() {
	if (!_parent || !_shape.is_valid() || !is_inside_tree()) {
		return;
	}

	PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
	int count = _parent->get_instance_count();
	int old_count = _bodies.size();

	// Free removed bodies and remove from map.
	for (int i = count; i < old_count; i++) {
		if (_bodies[i].is_valid()) {
			_body_to_index.erase(_bodies[i]);
			ps->free_rid(_bodies[i]);
		}
	}
	_bodies.resize(count);
	_last_world_transforms.resize(count);

	int old_in_space_count = _in_space.size();
	_in_space.resize(count);
	for (int i = old_in_space_count; i < count; i++) {
		_in_space.set(i, 0);
	}

	// After swap-remove, surviving bodies may have moved to different indices.
	// Rebuild the map for all existing entries to fix stale index references.
	if (count < old_count) {
		for (int i = 0; i < count; i++) {
			if (_bodies[i].is_valid()) {
				_body_to_index[_bodies[i]] = i;
			}
		}
	}

	// Early-out: skip the full loop when nothing needs updating.
	// _any_active_changed: must enter loop to handle space add/remove.
	// Otherwise, only enter if we have bodies in space AND dirty transforms to push.
	if (!_any_active_changed) {
		if (count == old_count && get_active_count() == 0) {
			return; // No bodies in space, no active changes — nothing to do.
		}
		int dirty_count = _parent->get_dirty_count();
		if (count == old_count && dirty_count == 0) {
			return; // No transforms changed — nothing to push.
		}
	}

	Ref<World3D> world = get_world_3d();
	RID space = world.is_valid() ? world->get_space() : RID();
	RID shape_rid = _shape->get_rid();
	Transform3D global_xform = get_global_transform();
	const uint8_t *active_ptr = _active.ptr();

	for (int i = 0; i < count; i++) {
		bool should_be_active = active_ptr && i < _active.size() && active_ptr[i];

		if (i >= old_count) {
			// --- Create new body ---
			RID body_rid = ps->body_create();

			switch (_body_type) {
				case BODY_TYPE_STATIC:
					ps->body_set_mode(body_rid, PhysicsServer3D::BODY_MODE_STATIC);
					break;
				case BODY_TYPE_KINEMATIC:
					ps->body_set_mode(body_rid, PhysicsServer3D::BODY_MODE_KINEMATIC);
					break;
				case BODY_TYPE_RIGID:
					ps->body_set_mode(body_rid, PhysicsServer3D::BODY_MODE_RIGID);
					break;
			}

			ps->body_add_shape(body_rid, shape_rid);

			uint32_t effective_layer, effective_mask;
			_compute_effective_layers(effective_layer, effective_mask);
			ps->body_set_collision_layer(body_rid, effective_layer);
			ps->body_set_collision_mask(body_rid, effective_mask);

			_bodies.write[i] = body_rid;
			_body_to_index.insert(body_rid, i); // Incremental map update.

			// Active: add to space and set transform. Inactive: leave out of space.
			if (should_be_active && space.is_valid()) {
				ps->body_set_space(body_rid, space);
				Transform3D xform = global_xform * compute_instance_transform(_parent->get_instance_transform(i));
				ps->body_set_state(body_rid, PhysicsServer3D::BODY_STATE_TRANSFORM, xform);
				_last_world_transforms.write[i] = xform;
				_in_space.set(i, 1);
			} else {
				_last_world_transforms.write[i] = Transform3D();
				_in_space.set(i, 0);
			}
		} else {
			// --- Existing body: handle active↔space mismatches ---
			bool currently_in_space = _in_space[i] != 0;

			if (should_be_active && !currently_in_space && space.is_valid()) {
				// Activated — set velocity from parent bus and add to space.
				ps->body_set_state(_bodies[i], PhysicsServer3D::BODY_STATE_LINEAR_VELOCITY,  _parent->get_instance_linear_velocity(i));
				ps->body_set_state(_bodies[i], PhysicsServer3D::BODY_STATE_ANGULAR_VELOCITY, _parent->get_instance_angular_velocity(i));
				ps->body_set_space(_bodies[i], space);
				Transform3D xform = global_xform * compute_instance_transform(_parent->get_instance_transform(i));
				ps->body_set_state(_bodies[i], PhysicsServer3D::BODY_STATE_TRANSFORM, xform);
				_last_world_transforms.write[i] = xform;
				_in_space.set(i, 1);
			} else if (!should_be_active && currently_in_space) {
				// Deactivated — save velocity to parent bus, then remove from space.
				_parent->set_instance_linear_velocity(i,  ps->body_get_state(_bodies[i], PhysicsServer3D::BODY_STATE_LINEAR_VELOCITY));
				_parent->set_instance_angular_velocity(i, ps->body_get_state(_bodies[i], PhysicsServer3D::BODY_STATE_ANGULAR_VELOCITY));
				ps->body_set_space(_bodies[i], RID());
				_last_world_transforms.write[i] = Transform3D();
				_in_space.set(i, 0);
			} else if (should_be_active && _parent->is_dirty(i)) {
				// Normal transform push — only if actually changed.
				Transform3D xform = global_xform * compute_instance_transform(_parent->get_instance_transform(i));
				if (xform != _last_world_transforms[i]) {
					ps->body_set_state(_bodies[i], PhysicsServer3D::BODY_STATE_TRANSFORM, xform);
					_last_world_transforms.write[i] = xform;
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Physics process — rigid body readback
// ---------------------------------------------------------------------------

void MultiNodeCollider::_physics_process(double p_delta) {
	if (_body_type != BODY_TYPE_RIGID || !_parent || !is_inside_tree()) {
		return;
	}

	int count = _bodies.size();
	if (count == 0) {
		return;
	}

	// Skip readback entirely when no bodies are in the physics space.
	// Avoids begin_batch/end_batch which fires instances_changed on all sub-nodes.
	if (get_active_count() == 0) {
		return;
	}

	PhysicsServer3D *ps = PhysicsServer3D::get_singleton();

	// Cache the inverse global transform — only recompute when our transform moves.
	if (_global_inv_dirty) {
		_cached_global_inv = get_global_transform().affine_inverse();
		_global_inv_dirty = false;
	}

	Transform3D offset_inv = _transform_offset.affine_inverse();
	bool has_offset = (_transform_offset != Transform3D());
	const uint8_t *active_ptr = _active.ptr();
	bool any_updated = false;

	_parent->begin_batch();

	for (int i = 0; i < count; i++) {
		if (active_ptr && i < _active.size() && !active_ptr[i]) {
			continue;
		}
		// Body flagged active but not yet added to physics space (pending _sync_bodies).
		// Skip readback — reading an out-of-space body returns stale transform and would
		// overwrite the parent's instance data with the previous body position.
		if (_in_space[i] == 0) {
			continue;
		}

		// Read transform — compare with cache to detect movement (replaces sleep check).
		// One PhysicsServer call per body instead of two.
		Transform3D world_xform = ps->body_get_state(_bodies[i], PhysicsServer3D::BODY_STATE_TRANSFORM);

		if (world_xform == _last_world_transforms[i]) {
			continue; // Jolt didn't move it — still sleeping or unchanged.
		}
		_last_world_transforms.write[i] = world_xform;

		Transform3D local_xform = _cached_global_inv * world_xform;
		if (has_offset) {
			local_xform = local_xform * offset_inv;
		}
		_parent->set_instance_transform(i, local_xform);
		any_updated = true;
	}

	if (any_updated) {
		_in_readback = true;
		_parent->end_batch();
		_in_readback = false;
	} else {
		_parent->end_batch();
	}
}

RID MultiNodeCollider::get_body_rid(int p_index) const {
	if (p_index >= 0 && p_index < _bodies.size()) {
		return _bodies[p_index];
	}
	return RID();
}

bool MultiNodeCollider::is_instance_in_space(int p_index) const {
	return (p_index >= 0 && p_index < _in_space.size()) ? _in_space[p_index] != 0 : false;
}

int MultiNodeCollider::get_instance_from_body(const RID &p_body_rid) const {
	HashMap<RID, int>::ConstIterator it = _body_to_index.find(p_body_rid);
	if (it != _body_to_index.end()) {
		return it->value;
	}
	return -1;
}
