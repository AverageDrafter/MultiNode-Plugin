#include "multi_node_area.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

MultiNodeArea::MultiNodeArea() {}
MultiNodeArea::~MultiNodeArea() { _clear_areas(); }

void MultiNodeArea::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_shape", "shape"), &MultiNodeArea::set_shape);
	ClassDB::bind_method(D_METHOD("get_shape"), &MultiNodeArea::get_shape);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "shape", PROPERTY_HINT_RESOURCE_TYPE, "Shape3D"), "set_shape", "get_shape");

	ClassDB::bind_method(D_METHOD("set_collision_layer", "layer"), &MultiNodeArea::set_collision_layer);
	ClassDB::bind_method(D_METHOD("get_collision_layer"), &MultiNodeArea::get_collision_layer);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_layer", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_layer", "get_collision_layer");

	ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"), &MultiNodeArea::set_collision_mask);
	ClassDB::bind_method(D_METHOD("get_collision_mask"), &MultiNodeArea::get_collision_mask);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_mask", "get_collision_mask");

	ClassDB::bind_method(D_METHOD("set_monitorable", "monitorable"), &MultiNodeArea::set_monitorable);
	ClassDB::bind_method(D_METHOD("get_monitorable"), &MultiNodeArea::get_monitorable);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "monitorable"), "set_monitorable", "get_monitorable");

	ClassDB::bind_method(D_METHOD("get_instance_area_rid", "index"), &MultiNodeArea::get_instance_area_rid);
	ClassDB::bind_method(D_METHOD("get_instance_from_area", "area_rid"), &MultiNodeArea::get_instance_from_area);
}

void MultiNodeArea::_on_sub_entered() { _rebuild(); }
void MultiNodeArea::_on_sub_exiting() { _clear_areas(); }
void MultiNodeArea::_on_parent_sync() { _sync_areas(); }
void MultiNodeArea::_on_active_changed() {
	// Do NOT _rebuild — Jolt leaks freed area RIDs in its broad-phase.
	// Just request a sync so _on_parent_sync → _sync_areas handles
	// active↔space transitions on existing RIDs at end of frame.
	if (_parent) {
		_parent->request_sync();
	}
}

// Properties
void MultiNodeArea::set_shape(const Ref<Shape3D> &p_shape) { _shape = p_shape; if (is_inside_tree()) _rebuild(); }
Ref<Shape3D> MultiNodeArea::get_shape() const { return _shape; }
void MultiNodeArea::set_collision_layer(uint32_t p_layer) {
	_collision_layer = p_layer;
	PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
	if (ps) {
		for (int i = 0; i < _areas.size(); i++) {
			if (_areas[i].is_valid()) {
				ps->area_set_collision_layer(_areas[i], _collision_layer);
			}
		}
	}
}
uint32_t MultiNodeArea::get_collision_layer() const { return _collision_layer; }
void MultiNodeArea::set_collision_mask(uint32_t p_mask) {
	_collision_mask = p_mask;
	PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
	if (ps) {
		for (int i = 0; i < _areas.size(); i++) {
			if (_areas[i].is_valid()) {
				ps->area_set_collision_mask(_areas[i], _collision_mask);
			}
		}
	}
}
uint32_t MultiNodeArea::get_collision_mask() const { return _collision_mask; }
void MultiNodeArea::set_monitorable(bool p_monitorable) {
	_monitorable = p_monitorable;
	PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
	if (ps) {
		for (int i = 0; i < _areas.size(); i++) {
			if (_areas[i].is_valid()) {
				ps->area_set_monitorable(_areas[i], _monitorable);
			}
		}
	}
}
bool MultiNodeArea::get_monitorable() const { return _monitorable; }

RID MultiNodeArea::get_instance_area_rid(int p_index) const {
	if (p_index >= 0 && p_index < _areas.size() && _areas[p_index].is_valid()) {
		return _areas[p_index];
	}
	return RID();
}

int MultiNodeArea::get_instance_from_area(const RID &p_area_rid) const {
	HashMap<RID, int>::ConstIterator it = _area_to_index.find(p_area_rid);
	return (it != _area_to_index.end()) ? it->value : -1;
}

void MultiNodeArea::_clear_areas() {
	PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
	if (!ps) {
		_areas.clear();
		_in_space.resize(0);
		_area_to_index.clear();
		return;
	}
	int freed_count = 0;
	int was_in_space = 0;
	for (int i = 0; i < _areas.size(); i++) {
		if (_areas[i].is_valid()) {
			if (i < _in_space.size() && _in_space[i] != 0) was_in_space++;
			ps->area_set_space(_areas[i], RID());
			ps->free_rid(_areas[i]);
			freed_count++;
		}
	}
	UtilityFunctions::print("[MultiNodeArea] _clear_areas: freed=", freed_count, " was_in_space=", was_in_space);
	_areas.clear();
	_in_space.resize(0);
	_last_transforms.clear();
	_area_to_index.clear();
}

void MultiNodeArea::_rebuild() {
	_clear_areas();
	if (!_parent || !_shape.is_valid() || !is_inside_tree()) return;
	_sync_areas();
}

void MultiNodeArea::_sync_areas() {
	if (!_parent || !_shape.is_valid() || !is_inside_tree()) return;

	PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
	int count = _parent->get_instance_count();
	int old_count = _areas.size();

	// Shrink — remove from space and free (only on instance removal, not toggle).
	for (int i = count; i < old_count; i++) {
		if (_areas[i].is_valid()) {
			if (i < _in_space.size() && _in_space[i] != 0) {
				ps->area_set_space(_areas[i], RID());
			}
			_area_to_index.erase(_areas[i]);
			ps->free_rid(_areas[i]);
		}
	}
	_areas.resize(count);
	_last_transforms.resize(count);

	int old_in_space_count = _in_space.size();
	_in_space.resize(count);
	for (int i = old_in_space_count; i < count; i++) {
		_in_space.set(i, 0);
	}

	// Rebuild map after shrink.
	if (count < old_count) {
		_area_to_index.clear();
		for (int i = 0; i < count; i++) {
			if (_areas[i].is_valid()) {
				_area_to_index[_areas[i]] = i;
			}
		}
	}

	// Early-out when nothing needs updating.
	if (!_any_active_changed) {
		if (count == old_count && get_active_count() == 0) {
			return;
		}
		int dirty_count = _parent->get_dirty_count();
		if (count == old_count && dirty_count == 0) {
			return;
		}
	}

	Ref<World3D> world = get_world_3d();
	RID space = world.is_valid() ? world->get_space() : RID();
	RID shape_rid = _shape->get_rid();
	Transform3D global_xform = _parent->get_cached_global_transform();
	const uint8_t *active_ptr = _active.ptr();

	// Trace counters.
	int _dbg_created = 0, _dbg_added = 0, _dbg_removed = 0;

	// Create areas upfront for new slots (like collider creates bodies).
	// NEVER free_rid during normal operation — Jolt leaks freed areas in
	// its broad-phase. Toggle space membership instead.
	for (int i = 0; i < count; i++) {
		bool should_be_active = active_ptr && i < _active.size() && active_ptr[i];

		if (i >= old_count) {
			// --- New slot: create area RID ---
			RID area_rid = ps->area_create();
			ps->area_add_shape(area_rid, shape_rid);
			ps->area_set_collision_layer(area_rid, _collision_layer);
			ps->area_set_collision_mask(area_rid, _collision_mask);
			ps->area_set_monitorable(area_rid, _monitorable);

			_areas.write[i] = area_rid;
			_area_to_index.insert(area_rid, i);

			_dbg_created++;
			if (should_be_active && space.is_valid()) {
				ps->area_set_space(area_rid, space);
				Transform3D xform = global_xform * compute_instance_transform(_parent->get_instance_transform(i));
				ps->area_set_transform(area_rid, xform);
				_last_transforms.write[i] = xform;
				_in_space.set(i, 1);
				_dbg_added++;
			} else {
				_last_transforms.write[i] = Transform3D();
				_in_space.set(i, 0);
			}
		} else if (_areas[i].is_valid()) {
			// --- Existing slot: toggle space membership ---
			bool currently_in_space = _in_space[i] != 0;

			if (should_be_active && !currently_in_space && space.is_valid()) {
				ps->area_set_space(_areas[i], space);
				Transform3D xform = global_xform * compute_instance_transform(_parent->get_instance_transform(i));
				ps->area_set_transform(_areas[i], xform);
				_last_transforms.write[i] = xform;
				_in_space.set(i, 1);
				_dbg_added++;
			} else if (!should_be_active && currently_in_space) {
				// Remove from space — do NOT free_rid.
				ps->area_set_space(_areas[i], RID());
				_last_transforms.write[i] = Transform3D();
				_in_space.set(i, 0);
				_dbg_removed++;
			} else if (should_be_active && currently_in_space && _parent->is_dirty(i)) {
				Transform3D xform = global_xform * compute_instance_transform(_parent->get_instance_transform(i));
				if (xform != _last_transforms[i]) {
					ps->area_set_transform(_areas[i], xform);
					_last_transforms.write[i] = xform;
				}
			}
		}
	}

	if (_dbg_created > 0 || _dbg_added > 0 || _dbg_removed > 0) {
		int in_space_count = 0;
		for (int i = 0; i < _in_space.size(); i++) {
			if (_in_space[i] != 0) in_space_count++;
		}
		UtilityFunctions::print("[MultiNodeArea] _sync: created=", _dbg_created,
			" added_to_space=", _dbg_added, " removed_from_space=", _dbg_removed,
			" total_rids=", _areas.size(), " currently_in_space=", in_space_count);
	}
}
