#include "multi_node_raycast.h"
#include "multi_node.h"
#include "multi_node_collider.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/object.hpp>

using namespace godot;

MultiNodeCollider *MultiNodeRaycast::_get_cached_collider() const {
	if (_cached_collider_id == 0) {
		return nullptr;
	}
	Object *obj = ObjectDB::get_instance(ObjectID(_cached_collider_id));
	if (!obj) {
		return nullptr;
	}
	return Object::cast_to<MultiNodeCollider>(obj);
}

MultiNodeRaycast::MultiNodeRaycast() {}
MultiNodeRaycast::~MultiNodeRaycast() {}

void MultiNodeRaycast::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_cast_direction", "direction"), &MultiNodeRaycast::set_cast_direction);
	ClassDB::bind_method(D_METHOD("get_cast_direction"), &MultiNodeRaycast::get_cast_direction);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "cast_direction"), "set_cast_direction", "get_cast_direction");

	ClassDB::bind_method(D_METHOD("set_cast_length", "length"), &MultiNodeRaycast::set_cast_length);
	ClassDB::bind_method(D_METHOD("get_cast_length"), &MultiNodeRaycast::get_cast_length);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cast_length", PROPERTY_HINT_RANGE, "0.1,1000.0,0.1,suffix:m"), "set_cast_length", "get_cast_length");

	ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"), &MultiNodeRaycast::set_collision_mask);
	ClassDB::bind_method(D_METHOD("get_collision_mask"), &MultiNodeRaycast::get_collision_mask);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_mask", "get_collision_mask");

	ClassDB::bind_method(D_METHOD("set_hit_from_inside", "enable"), &MultiNodeRaycast::set_hit_from_inside);
	ClassDB::bind_method(D_METHOD("get_hit_from_inside"), &MultiNodeRaycast::get_hit_from_inside);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hit_from_inside"), "set_hit_from_inside", "get_hit_from_inside");

	ClassDB::bind_method(D_METHOD("is_instance_colliding", "index"), &MultiNodeRaycast::is_instance_colliding);
	ClassDB::bind_method(D_METHOD("get_instance_hit_point", "index"), &MultiNodeRaycast::get_instance_hit_point);
	ClassDB::bind_method(D_METHOD("get_instance_hit_normal", "index"), &MultiNodeRaycast::get_instance_hit_normal);
	ClassDB::bind_method(D_METHOD("get_instance_hit_distance", "index"), &MultiNodeRaycast::get_instance_hit_distance);
	ClassDB::bind_method(D_METHOD("get_instance_hit_rid", "index"), &MultiNodeRaycast::get_instance_hit_rid);
}

void MultiNodeRaycast::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PHYSICS_PROCESS: {
			_cast_all();
		} break;
	}
}

void MultiNodeRaycast::_on_sub_entered() {
	set_physics_process(true);
}

void MultiNodeRaycast::_on_sub_exiting() {
	set_physics_process(false);
	_results.clear();
	_cached_collider_id = 0;
}

void MultiNodeRaycast::_on_parent_sync() {
	if (_parent) {
		_resize_results(_parent->get_instance_count());
	}
}

// Properties
void MultiNodeRaycast::set_cast_direction(const Vector3 &p_dir) { _cast_direction = p_dir.normalized(); }
Vector3 MultiNodeRaycast::get_cast_direction() const { return _cast_direction; }
void MultiNodeRaycast::set_cast_length(float p_length) { _cast_length = p_length; }
float MultiNodeRaycast::get_cast_length() const { return _cast_length; }
void MultiNodeRaycast::set_collision_mask(uint32_t p_mask) { _collision_mask = p_mask; }
uint32_t MultiNodeRaycast::get_collision_mask() const { return _collision_mask; }
void MultiNodeRaycast::set_hit_from_inside(bool p_enable) { _hit_from_inside = p_enable; }
bool MultiNodeRaycast::get_hit_from_inside() const { return _hit_from_inside; }

// Result queries
bool MultiNodeRaycast::is_instance_colliding(int p_index) const {
	return (p_index >= 0 && p_index < _results.size()) ? _results[p_index].hit : false;
}

Vector3 MultiNodeRaycast::get_instance_hit_point(int p_index) const {
	return (p_index >= 0 && p_index < _results.size()) ? _results[p_index].point : Vector3();
}

Vector3 MultiNodeRaycast::get_instance_hit_normal(int p_index) const {
	return (p_index >= 0 && p_index < _results.size()) ? _results[p_index].normal : Vector3();
}

float MultiNodeRaycast::get_instance_hit_distance(int p_index) const {
	return (p_index >= 0 && p_index < _results.size()) ? _results[p_index].distance : 0.0f;
}

RID MultiNodeRaycast::get_instance_hit_rid(int p_index) const {
	return (p_index >= 0 && p_index < _results.size()) ? _results[p_index].collider_rid : RID();
}

void MultiNodeRaycast::_resize_results(int p_count) {
	int old_count = _results.size();
	_results.resize(p_count);
	for (int i = old_count; i < p_count; i++) {
		_results.write[i] = RayResult();
	}
}

// ---------------------------------------------------------------------------
// The cast loop — runs every physics frame
// ---------------------------------------------------------------------------

void MultiNodeRaycast::_cast_all() {
	if (!_parent || !is_inside_tree()) return;

	int count = _parent->get_instance_count();
	if (count == 0 || _results.size() != count) return;

	Ref<World3D> world = get_world_3d();
	if (!world.is_valid()) return;

	PhysicsDirectSpaceState3D *space_state = world->get_direct_space_state();
	if (!space_state) return;

	Transform3D global_xform = get_global_transform();
	const Vector<Transform3D> &transforms = _parent->get_transforms_internal();
	const PackedByteArray &dirty = _parent->get_dirty_flags_internal();
	const uint8_t *active_ptr = _active.size() > 0 ? _active.ptr() : nullptr;

	// Build query parameters once — reuse for all casts.
	Ref<PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(
			Vector3(), Vector3(), _collision_mask);
	query->set_hit_from_inside(_hit_from_inside);

	// Cache sibling collider lookup (only search once, not every frame).
	// Uses ObjectID to avoid dangling pointer if sibling is freed.
	MultiNodeCollider *collider = _get_cached_collider();
	if (!collider) {
		_cached_collider_id = 0;
		for (int c = 0; c < _parent->get_child_count(); c++) {
			collider = Object::cast_to<MultiNodeCollider>(_parent->get_child(c));
			if (collider) {
				_cached_collider_id = collider->get_instance_id();
				break;
			}
		}
	}

	// Pre-allocate exclude array once.
	TypedArray<RID> exclude;
	if (collider) {
		exclude.resize(1);
	}

	int dirty_count = _parent->get_dirty_count();

	for (int i = 0; i < count; i++) {
		RayResult &result = _results.write[i];

		if (active_ptr && i < _active.size() && !active_ptr[i]) {
			result.hit = false;
			continue;
		}

		// Skip instances that haven't moved since last cast (if nothing is dirty, skip all).
		if (dirty_count > 0 && !dirty[i] && result.distance > 0.0f) {
			continue; // Result from last frame is still valid.
		}

		// Set exclude to this instance's own body.
		if (collider) {
			exclude[0] = collider->get_body_rid(i);
			query->set_exclude(exclude);
		}

		// Compute ray origin from instance transform.
		Transform3D inst_xform = compute_instance_transform(transforms[i]);
		Vector3 origin = global_xform.xform(inst_xform.origin);

		// Direction: apply instance rotation if use_instance_basis is on,
		// otherwise use the raw cast_direction in world space.
		Vector3 direction = _cast_direction;
		if (get_use_instance_basis()) {
			direction = (global_xform.basis * inst_xform.basis).xform(_cast_direction).normalized();
		}

		Vector3 end = origin + direction * _cast_length;

		query->set_from(origin);
		query->set_to(end);

		Dictionary hit = space_state->intersect_ray(query);

		if (hit.size() > 0) {
			result.hit = true;
			result.point = hit["position"];
			result.normal = hit["normal"];
			result.collider_rid = hit["rid"];
			result.distance = origin.distance_to(result.point);
		} else {
			result.hit = false;
			result.point = end;
			result.normal = Vector3();
			result.collider_rid = RID();
			result.distance = _cast_length;
		}
	}
}
