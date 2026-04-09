#include "multi_node_animator.h"
#include "multi_node.h"
#include "multi_animation_player.h" // for _interpolate_variant, _apply_easing

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/object.hpp>

using namespace godot;

MultiNodeAnimator::MultiNodeAnimator() {}
MultiNodeAnimator::~MultiNodeAnimator() {}

// ---------------------------------------------------------------------------
// Property binding — resolve key property paths to sibling nodes
// ---------------------------------------------------------------------------

void MultiNodeAnimator::_bind_properties() {
	_bound_props.clear();
	_bound_methods.clear();
	_has_transform_props = false;

	if (!_resource.is_valid() || !_parent) {
		return;
	}

	// Collect all unique property paths across all keys.
	Vector<StringName> all_paths;
	int key_count = _resource->get_key_count();
	for (int k = 0; k < key_count; k++) {
		Dictionary props = _resource->get_key_properties(k);
		Array keys = props.keys();
		for (int i = 0; i < keys.size(); i++) {
			StringName path = keys[i];
			bool found = false;
			for (int j = 0; j < all_paths.size(); j++) {
				if (all_paths[j] == path) {
					found = true;
					break;
				}
			}
			if (!found) {
				all_paths.push_back(path);
			}
		}
	}

	// Resolve each property path.
	for (int i = 0; i < all_paths.size(); i++) {
		BoundProperty bp;
		bp.path = all_paths[i];

		String s = String(bp.path);

		// Check for transform shorthand.
		if (s == "position") {
			bp.is_position = true;
			_has_transform_props = true;
			_bound_props.push_back(bp);
			continue;
		}
		if (s == "rotation") {
			bp.is_rotation = true;
			_has_transform_props = true;
			_bound_props.push_back(bp);
			continue;
		}
		if (s == "scale") {
			bp.is_scale = true;
			_has_transform_props = true;
			_bound_props.push_back(bp);
			continue;
		}

		// "NodeName:property" or direct property.
		int colon = s.find(":");
		Node *target = nullptr;
		if (colon >= 0) {
			String node_name = s.substr(0, colon);
			bp.property_name = StringName(s.substr(colon + 1));
			target = _parent->get_node_or_null(NodePath(node_name));
		} else {
			bp.property_name = bp.path;
			target = _parent;
		}

		if (target) {
			bp.target_node_id = target->get_instance_id();

			// Check for set_instance_X pattern.
			String setter = "set_instance_" + String(bp.property_name);
			if (target->has_method(setter)) {
				bp.setter_method = StringName(setter);
				bp.has_per_instance_setter = true;
			}
		}

		_bound_props.push_back(bp);
	}

	// Collect all unique method names across all keys and resolve targets.
	for (int k = 0; k < key_count; k++) {
		Dictionary methods = _resource->get_key_methods(k);
		Array method_names = methods.keys();
		for (int i = 0; i < method_names.size(); i++) {
			StringName method_name = method_names[i];

			// Check if already bound.
			bool found = false;
			for (int j = 0; j < _bound_methods.size(); j++) {
				if (_bound_methods[j].method_name == method_name) {
					found = true;
					break;
				}
			}
			if (found) continue;

			// Resolve target: check parent and siblings for the method.
			BoundMethod bm;
			bm.method_name = method_name;

			String ms = String(method_name);
			int colon = ms.find(":");
			if (colon >= 0) {
				// "NodeName:method" format.
				String node_name = ms.substr(0, colon);
				bm.method_name = StringName(ms.substr(colon + 1));
				Node *target = _parent->get_node_or_null(NodePath(node_name));
				if (target) {
					bm.target_node_id = target->get_instance_id();
				}
			} else {
				// Default: call on parent.
				bm.target_node_id = _parent->get_instance_id();
			}

			_bound_methods.push_back(bm);
		}
	}
}

// ---------------------------------------------------------------------------
// State management
// ---------------------------------------------------------------------------

void MultiNodeAnimator::_resize_state(int p_count) {
	int old_count = _current_keys.size();
	_current_keys.resize(p_count);
	_target_keys.resize(p_count);
	_progresses.resize(p_count);
	_hold_timers.resize(p_count);
	_speeds.resize(p_count);
	_playing.resize(p_count);
	_base_transforms.resize(p_count);

	for (int i = old_count; i < p_count; i++) {
		_current_keys.set(i, 0);
		_target_keys.set(i, -1);
		_progresses.set(i, 0.0f);
		_hold_timers.set(i, 0.0f);
		_speeds.set(i, 1.0f);
		_playing.set(i, 0);
		_base_transforms.write[i] = Transform3D();
	}
}

// ---------------------------------------------------------------------------
// Transition cache
// ---------------------------------------------------------------------------

MultiNodeAnimator::TransitionCache MultiNodeAnimator::_get_transition_cache(int p_from, int p_to) const {
	TransitionCache tc;
	if (_resource.is_valid()) {
		int tidx = _resource->find_transition(p_from, p_to);
		if (tidx >= 0) {
			tc.duration = _resource->get_transition_duration(tidx);
			tc.easing = _resource->get_transition_easing(tidx);
			tc.trans_type = _resource->get_transition_trans_type(tidx);
		} else {
			tc.duration = _resource->get_default_duration();
			tc.easing = _resource->get_default_easing();
			tc.trans_type = _resource->get_default_trans_type();
		}
	}
	return tc;
}

// ---------------------------------------------------------------------------
// Method call firing
// ---------------------------------------------------------------------------

void MultiNodeAnimator::_fire_key_methods(int p_instance, int p_key_index) {
	if (!_resource.is_valid()) return;

	Dictionary methods = _resource->get_key_methods(p_key_index);
	if (methods.is_empty()) return;

	Array method_names = methods.keys();
	for (int m = 0; m < method_names.size(); m++) {
		StringName raw_name = method_names[m];
		Array args = methods[raw_name];

		// Find the bound method target.
		for (int b = 0; b < _bound_methods.size(); b++) {
			// Match on the raw name (which may include "NodeName:" prefix).
			// The BoundMethod stores the resolved method_name and target_node_id.
			String raw_str = String(raw_name);
			String bound_check;
			int colon = raw_str.find(":");
			if (colon >= 0) {
				bound_check = raw_str.substr(colon + 1);
			} else {
				bound_check = raw_str;
			}

			if (_bound_methods[b].method_name == StringName(bound_check) || _bound_methods[b].method_name == raw_name) {
				Object *obj = ObjectDB::get_instance(ObjectID(_bound_methods[b].target_node_id));
				Node *target = obj ? Object::cast_to<Node>(obj) : nullptr;
				if (target) {
					// Prepend instance index as first argument.
					Array call_args;
					call_args.push_back(p_instance);
					call_args.append_array(args);
					target->callv(_bound_methods[b].method_name, call_args);
				}
				break;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Core evaluation — the hot loop
// ---------------------------------------------------------------------------

void MultiNodeAnimator::_evaluate_all(double p_delta) {
	if (!_parent || !_resource.is_valid()) {
		return;
	}

	int count = _parent->get_instance_count();
	if (count == 0 || _current_keys.size() != count) {
		return;
	}

	int key_count = _resource->get_key_count();
	if (key_count == 0) {
		return;
	}

	int active_size = _active.size();
	const uint8_t *active_ptr = active_size > 0 ? _active.ptr() : nullptr;
	const uint8_t *playing_ptr = _playing.ptr();
	int play_mode = _resource->get_play_mode();
	bool is_reverse = _resource->get_reverse();

	_arrived_this_frame.clear();

	if (_has_transform_props) {
		_parent->begin_batch();
	}

	for (int i = 0; i < count; i++) {
		if (active_ptr && i < active_size && !active_ptr[i]) {
			continue;
		}
		if (!playing_ptr[i]) {
			continue;
		}

		int cur = _current_keys[i];
		int tgt = _target_keys[i];
		float spd = _speeds[i] * _speed_scale;

		if (tgt >= 0) {
			// --- Currently transitioning ---
			TransitionCache tc = _get_transition_cache(cur, tgt);

			float prog = _progresses[i];
			if (tc.duration > 0.0f) {
				prog += (float)(p_delta * (double)spd) / tc.duration;
			} else {
				prog = 1.0f;
			}

			if (prog >= 1.0f) {
				// Arrived at target key.
				_current_keys.set(i, tgt);
				_target_keys.set(i, -1);
				_progresses.set(i, 0.0f);

				if (tgt < key_count) {
					_hold_timers.set(i, _resource->get_key_hold_time(tgt));
				}

				_arrived_this_frame.push_back(i);

				// Apply final key values.
				Dictionary props = _resource->get_key_properties(tgt);
				Vector3 pos;
				Quaternion rot;
				Vector3 scl(1, 1, 1);
				bool wrote_transform = false;

				for (int b = 0; b < _bound_props.size(); b++) {
					const BoundProperty &bp = _bound_props[b];
					if (!props.has(bp.path)) continue;
					Variant val = props[bp.path];

					if (bp.is_position) {
						pos = val;
						wrote_transform = true;
					} else if (bp.is_rotation) {
						if (val.get_type() == Variant::QUATERNION) {
							rot = val;
						} else {
							Vector3 euler = val;
							rot = Quaternion::from_euler(euler);
						}
						wrote_transform = true;
					} else if (bp.is_scale) {
						scl = val;
						wrote_transform = true;
					} else if (bp.has_per_instance_setter) {
						Object *obj = ObjectDB::get_instance(ObjectID(bp.target_node_id));
						Node *target = obj ? Object::cast_to<Node>(obj) : nullptr;
						if (target) target->call(bp.setter_method, i, val);
					} else if (bp.target_node_id != 0) {
						Object *obj = ObjectDB::get_instance(ObjectID(bp.target_node_id));
						Node *target = obj ? Object::cast_to<Node>(obj) : nullptr;
						if (target) target->set(bp.property_name, val);
					}
				}

				if (wrote_transform && i < (int)_base_transforms.size()) {
					const Transform3D &base = _base_transforms[i];
					Basis anim_basis = Basis(rot).scaled(scl);
					Transform3D xform;
					xform.basis = base.basis * anim_basis;
					xform.origin = base.origin + pos;
					_parent->set_instance_transform(i, xform);
				}

				// Fire method calls for this key.
				_fire_key_methods(i, tgt);

			} else {
				_progresses.set(i, prog);

				// Interpolate properties.
				float eased = MultiAnimationPlayer::_apply_easing(prog, tc.easing, tc.trans_type);
				Dictionary from_props = _resource->get_key_properties(cur);
				Dictionary to_props = _resource->get_key_properties(tgt);

				Vector3 pos;
				Quaternion rot;
				Vector3 scl(1, 1, 1);
				bool wrote_transform = false;

				for (int b = 0; b < _bound_props.size(); b++) {
					const BoundProperty &bp = _bound_props[b];

					Variant from_val = from_props.has(bp.path) ? from_props[bp.path] : Variant();
					Variant to_val = to_props.has(bp.path) ? to_props[bp.path] : Variant();

					if (from_val.get_type() == Variant::NIL && to_val.get_type() == Variant::NIL) continue;
					if (from_val.get_type() == Variant::NIL) from_val = to_val;
					if (to_val.get_type() == Variant::NIL) to_val = from_val;

					Variant val = MultiAnimationPlayer::_interpolate_variant(from_val, to_val, eased);

					if (bp.is_position) {
						pos = val;
						wrote_transform = true;
					} else if (bp.is_rotation) {
						if (val.get_type() == Variant::QUATERNION) {
							rot = val;
						} else {
							Vector3 euler = val;
							rot = Quaternion::from_euler(euler);
						}
						wrote_transform = true;
					} else if (bp.is_scale) {
						scl = val;
						wrote_transform = true;
					} else if (bp.has_per_instance_setter) {
						Object *obj = ObjectDB::get_instance(ObjectID(bp.target_node_id));
						Node *target = obj ? Object::cast_to<Node>(obj) : nullptr;
						if (target) target->call(bp.setter_method, i, val);
					} else if (bp.target_node_id != 0) {
						Object *obj = ObjectDB::get_instance(ObjectID(bp.target_node_id));
						Node *target = obj ? Object::cast_to<Node>(obj) : nullptr;
						if (target) target->set(bp.property_name, val);
					}
				}

				if (wrote_transform && i < (int)_base_transforms.size()) {
					const Transform3D &base = _base_transforms[i];
					Basis anim_basis = Basis(rot).scaled(scl);
					Transform3D xform;
					xform.basis = base.basis * anim_basis;
					xform.origin = base.origin + pos;
					_parent->set_instance_transform(i, xform);
				}
			}
		} else {
			// --- At rest on a key — look for AUTO transitions ---
			int auto_tidx = _resource->find_auto_transition_from(cur);
			if (auto_tidx >= 0) {
				// Has an AUTO transition — advance after hold time.
				float hold = _hold_timers[i];
				hold -= (float)(p_delta * (double)spd);
				if (hold <= 0.0f) {
					int next = _resource->get_transition_to(auto_tidx);
					_target_keys.set(i, next);
					_progresses.set(i, 0.0f);
				} else {
					_hold_timers.set(i, hold);
				}
			} else {
				// No AUTO transition — check play mode for wrap/bounce.
				if (play_mode == MultiAnimation::PLAY_LOOP || play_mode == MultiAnimation::PLAY_PING_PONG) {
					float hold = _hold_timers[i];
					hold -= (float)(p_delta * (double)spd);
					if (hold <= 0.0f) {
						int next = -1;
						if (play_mode == MultiAnimation::PLAY_LOOP) {
							next = is_reverse ? key_count - 1 : 0;
							// Only loop if there's a transition back.
							if (_resource->find_transition(cur, next) < 0 && _resource->find_auto_transition_from(next) < 0) {
								_playing.set(i, 0);
								continue;
							}
						}
						if (next >= 0 && next != cur) {
							_target_keys.set(i, next);
							_progresses.set(i, 0.0f);
						} else {
							_playing.set(i, 0);
						}
					} else {
						_hold_timers.set(i, hold);
					}
				}
				// ONE_SHOT with no AUTO: idle until go_to_key_instance().
			}
		}
	}

	if (_has_transform_props) {
		_parent->end_batch();
	}

	if (_arrived_this_frame.size() > 0) {
		emit_signal("instances_arrived");
	}
}

// ---------------------------------------------------------------------------
// MultiNodeSub overrides
// ---------------------------------------------------------------------------

void MultiNodeAnimator::_on_sub_entered() {
	_bind_properties();
	if (_autoplay && _parent) {
		play_all();
	}
	set_process(true);
}

void MultiNodeAnimator::_on_sub_exiting() {
	set_process(false);
	_bound_props.clear();
	_bound_methods.clear();
}

void MultiNodeAnimator::_on_parent_sync() {
	if (!_parent) {
		return;
	}
	int count = _parent->get_instance_count();
	if (count == _current_keys.size()) {
		return;
	}
	bool was_empty = (_current_keys.size() == 0);
	_resize_state(count);
	if (was_empty && count > 0 && _autoplay && _resource.is_valid()) {
		play_all();
	}
}

// ---------------------------------------------------------------------------
// Notification
// ---------------------------------------------------------------------------

void MultiNodeAnimator::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			_evaluate_all(get_process_delta_time());
		} break;
	}
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------

void MultiNodeAnimator::set_animation_resource(const Ref<MultiAnimation> &p_res) {
	_resource = p_res;
	if (is_inside_tree()) {
		_bind_properties();
		if (_autoplay && _parent && _resource.is_valid()) {
			play_all();
		}
	}
}

Ref<MultiAnimation> MultiNodeAnimator::get_animation_resource() const {
	return _resource;
}

void MultiNodeAnimator::set_autoplay(bool p_auto) { _autoplay = p_auto; }
bool MultiNodeAnimator::get_autoplay() const { return _autoplay; }

void MultiNodeAnimator::set_speed_scale(float p_speed) { _speed_scale = p_speed; }
float MultiNodeAnimator::get_speed_scale() const { return _speed_scale; }

void MultiNodeAnimator::set_sync_mode(int p_mode) { _sync_mode = p_mode; }
int MultiNodeAnimator::get_sync_mode() const { return _sync_mode; }

void MultiNodeAnimator::set_sync_offset(float p_offset) { _sync_offset = Math::clamp(p_offset, 0.0f, 1.0f); }
float MultiNodeAnimator::get_sync_offset() const { return _sync_offset; }

// ---------------------------------------------------------------------------
// Playback control
// ---------------------------------------------------------------------------

void MultiNodeAnimator::play_all(const StringName &p_from_key) {
	if (!_parent || !_resource.is_valid() || _resource->get_key_count() == 0) {
		return;
	}

	int count = _parent->get_instance_count();
	_resize_state(count);

	int start_key = 0;
	if (p_from_key != StringName()) {
		int idx = _resource->find_key(p_from_key);
		if (idx >= 0) start_key = idx;
	}

	int key_count = _resource->get_key_count();

	// Snapshot base transforms for additive transform properties.
	if (_has_transform_props) {
		for (int i = 0; i < count; i++) {
			_base_transforms.write[i] = _parent->get_instance_transform(i);
		}
	}

	for (int i = 0; i < count; i++) {
		_playing.set(i, 1);
		_target_keys.set(i, -1);
		_progresses.set(i, 0.0f);

		if (_sync_mode == SYNC_SYNC && key_count > 1) {
			int offset_key = ((int)(i * _sync_offset * key_count)) % key_count;
			int key = (start_key + offset_key) % key_count;
			_current_keys.set(i, key);
		} else {
			_current_keys.set(i, start_key);
		}

		_hold_timers.set(i, _resource->get_key_hold_time(_current_keys[i]));
	}
}

void MultiNodeAnimator::stop_all() {
	_playing.fill(0);
}

void MultiNodeAnimator::play_instance(int p_index, const StringName &p_from_key) {
	if (p_index < 0 || p_index >= _playing.size() || !_resource.is_valid()) {
		return;
	}

	int start_key = 0;
	if (p_from_key != StringName()) {
		int idx = _resource->find_key(p_from_key);
		if (idx >= 0) start_key = idx;
	}

	_playing.set(p_index, 1);
	_current_keys.set(p_index, start_key);
	_target_keys.set(p_index, -1);
	_progresses.set(p_index, 0.0f);
	_hold_timers.set(p_index, _resource->get_key_hold_time(start_key));

	if (_has_transform_props && _parent && p_index < (int)_base_transforms.size()) {
		_base_transforms.write[p_index] = _parent->get_instance_transform(p_index);
	}
}

void MultiNodeAnimator::stop_instance(int p_index) {
	if (p_index >= 0 && p_index < _playing.size()) {
		_playing.set(p_index, 0);
	}
}

void MultiNodeAnimator::go_to_key_instance(int p_index, const StringName &p_name) {
	if (!_resource.is_valid() || p_index < 0 || p_index >= _current_keys.size()) {
		return;
	}

	int target_idx = _resource->find_key(p_name);
	ERR_FAIL_COND_MSG(target_idx < 0, "MultiNodeAnimator: key not found: " + String(p_name));

	int cur = _current_keys[p_index];
	if (target_idx == cur) {
		return;
	}

	if (!_resource->can_transition(cur, target_idx)) {
		return;
	}

	_playing.set(p_index, 1);
	_target_keys.set(p_index, target_idx);
	_progresses.set(p_index, 0.0f);
}

void MultiNodeAnimator::set_instance_speed(int p_index, float p_speed) {
	if (p_index >= 0 && p_index < _speeds.size()) {
		_speeds.set(p_index, p_speed);
	}
}

// ---------------------------------------------------------------------------
// Per-instance queries
// ---------------------------------------------------------------------------

StringName MultiNodeAnimator::get_instance_current_key(int p_index) const {
	if (_resource.is_valid() && p_index >= 0 && p_index < _current_keys.size()) {
		int k = _current_keys[p_index];
		if (k >= 0 && k < _resource->get_key_count()) {
			return _resource->get_key_name(k);
		}
	}
	return StringName();
}

StringName MultiNodeAnimator::get_instance_target_key(int p_index) const {
	if (_resource.is_valid() && p_index >= 0 && p_index < _target_keys.size()) {
		int k = _target_keys[p_index];
		if (k >= 0 && k < _resource->get_key_count()) {
			return _resource->get_key_name(k);
		}
	}
	return StringName();
}

float MultiNodeAnimator::get_instance_progress(int p_index) const {
	if (p_index >= 0 && p_index < _progresses.size()) {
		return _progresses[p_index];
	}
	return 0.0f;
}

PackedInt32Array MultiNodeAnimator::get_arrived_instances() const {
	return _arrived_this_frame;
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void MultiNodeAnimator::_bind_methods() {
	// --- Resource ---
	ClassDB::bind_method(D_METHOD("set_animation_resource", "resource"), &MultiNodeAnimator::set_animation_resource);
	ClassDB::bind_method(D_METHOD("get_animation_resource"), &MultiNodeAnimator::get_animation_resource);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "animation_resource", PROPERTY_HINT_RESOURCE_TYPE, "MultiAnimation"),
			"set_animation_resource", "get_animation_resource");

	ClassDB::bind_method(D_METHOD("set_autoplay", "auto"), &MultiNodeAnimator::set_autoplay);
	ClassDB::bind_method(D_METHOD("get_autoplay"), &MultiNodeAnimator::get_autoplay);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

	ClassDB::bind_method(D_METHOD("set_speed_scale", "speed"), &MultiNodeAnimator::set_speed_scale);
	ClassDB::bind_method(D_METHOD("get_speed_scale"), &MultiNodeAnimator::get_speed_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_scale", PROPERTY_HINT_RANGE, "0.0,10.0,0.01"),
			"set_speed_scale", "get_speed_scale");

	ClassDB::bind_method(D_METHOD("set_sync_mode", "mode"), &MultiNodeAnimator::set_sync_mode);
	ClassDB::bind_method(D_METHOD("get_sync_mode"), &MultiNodeAnimator::get_sync_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sync_mode", PROPERTY_HINT_ENUM, "None,Sync"),
			"set_sync_mode", "get_sync_mode");

	ClassDB::bind_method(D_METHOD("set_sync_offset", "offset"), &MultiNodeAnimator::set_sync_offset);
	ClassDB::bind_method(D_METHOD("get_sync_offset"), &MultiNodeAnimator::get_sync_offset);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sync_offset", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
			"set_sync_offset", "get_sync_offset");

	BIND_ENUM_CONSTANT(SYNC_NONE);
	BIND_ENUM_CONSTANT(SYNC_SYNC);

	// --- Playback ---
	ClassDB::bind_method(D_METHOD("play_all", "from_key"), &MultiNodeAnimator::play_all, DEFVAL(StringName()));
	ClassDB::bind_method(D_METHOD("stop_all"), &MultiNodeAnimator::stop_all);
	ClassDB::bind_method(D_METHOD("play_instance", "index", "from_key"), &MultiNodeAnimator::play_instance, DEFVAL(StringName()));
	ClassDB::bind_method(D_METHOD("stop_instance", "index"), &MultiNodeAnimator::stop_instance);
	ClassDB::bind_method(D_METHOD("go_to_key_instance", "index", "name"), &MultiNodeAnimator::go_to_key_instance);
	ClassDB::bind_method(D_METHOD("set_instance_speed", "index", "speed"), &MultiNodeAnimator::set_instance_speed);

	// --- Queries ---
	ClassDB::bind_method(D_METHOD("get_instance_current_key", "index"), &MultiNodeAnimator::get_instance_current_key);
	ClassDB::bind_method(D_METHOD("get_instance_target_key", "index"), &MultiNodeAnimator::get_instance_target_key);
	ClassDB::bind_method(D_METHOD("get_instance_progress", "index"), &MultiNodeAnimator::get_instance_progress);
	ClassDB::bind_method(D_METHOD("get_arrived_instances"), &MultiNodeAnimator::get_arrived_instances);

	// --- Signals ---
	ADD_SIGNAL(MethodInfo("instances_arrived"));
}
