#include "multi_node_animator.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/object.hpp>

using namespace godot;

Node *MultiNodeAnimator::BoundTrack::get_target_node() const {
	if (target_node_id == 0) {
		return nullptr;
	}
	Object *obj = ObjectDB::get_instance(ObjectID(target_node_id));
	return obj ? Object::cast_to<Node>(obj) : nullptr;
}

MultiNodeAnimator::MultiNodeAnimator() {}
MultiNodeAnimator::~MultiNodeAnimator() {}

void MultiNodeAnimator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_animation", "animation"), &MultiNodeAnimator::set_animation);
	ClassDB::bind_method(D_METHOD("get_animation"), &MultiNodeAnimator::get_animation);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "animation", PROPERTY_HINT_RESOURCE_TYPE, "Animation"), "set_animation", "get_animation");

	ClassDB::bind_method(D_METHOD("set_autoplay", "auto"), &MultiNodeAnimator::set_autoplay);
	ClassDB::bind_method(D_METHOD("get_autoplay"), &MultiNodeAnimator::get_autoplay);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

	ClassDB::bind_method(D_METHOD("set_sync_mode", "mode"), &MultiNodeAnimator::set_sync_mode);
	ClassDB::bind_method(D_METHOD("get_sync_mode"), &MultiNodeAnimator::get_sync_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "sync_mode", PROPERTY_HINT_ENUM, "None,Sync"), "set_sync_mode", "get_sync_mode");

	ClassDB::bind_method(D_METHOD("set_sync_offset", "offset"), &MultiNodeAnimator::set_sync_offset);
	ClassDB::bind_method(D_METHOD("get_sync_offset"), &MultiNodeAnimator::get_sync_offset);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sync_offset", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_sync_offset", "get_sync_offset");

	ClassDB::bind_method(D_METHOD("set_speed_scale", "speed"), &MultiNodeAnimator::set_speed_scale);
	ClassDB::bind_method(D_METHOD("get_speed_scale"), &MultiNodeAnimator::get_speed_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_scale", PROPERTY_HINT_RANGE, "0.0,10.0,0.01"), "set_speed_scale", "get_speed_scale");

	BIND_ENUM_CONSTANT(SYNC_NONE);
	BIND_ENUM_CONSTANT(SYNC_SYNC);

	ClassDB::bind_method(D_METHOD("play_all"), &MultiNodeAnimator::play_all);
	ClassDB::bind_method(D_METHOD("stop_all"), &MultiNodeAnimator::stop_all);
	ClassDB::bind_method(D_METHOD("play_instance", "index", "from_time"), &MultiNodeAnimator::play_instance, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("stop_instance", "index"), &MultiNodeAnimator::stop_instance);
	ClassDB::bind_method(D_METHOD("set_instance_speed", "index", "speed"), &MultiNodeAnimator::set_instance_speed);
	ClassDB::bind_method(D_METHOD("get_instance_time", "index"), &MultiNodeAnimator::get_instance_time);
}

// ---------------------------------------------------------------------------
// Notification — the core evaluation loop
// ---------------------------------------------------------------------------

void MultiNodeAnimator::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			_evaluate_all(get_process_delta_time());
		} break;
	}
}

// ---------------------------------------------------------------------------
// MultiNodeSub overrides
// ---------------------------------------------------------------------------

void MultiNodeAnimator::_on_sub_entered() {
	_bind_tracks();
	if (_autoplay && _parent) {
		play_all();
	}
	set_process(true);
}

void MultiNodeAnimator::_on_sub_exiting() {
	set_process(false);
	_bound_tracks.clear();
}

void MultiNodeAnimator::_on_parent_sync() {
	if (!_parent) {
		return;
	}
	int count = _parent->get_instance_count();
	if (count == _times.size()) {
		return; // Count unchanged — nothing to resize.
	}
	bool was_empty = (_times.size() == 0);
	_resize_state(count);
	// Re-trigger autoplay when instances first arrive (handles child-before-parent _ready ordering).
	if (was_empty && count > 0 && _autoplay && _animation.is_valid()) {
		play_all();
	}
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------

void MultiNodeAnimator::set_animation(const Ref<Animation> &p_anim) {
	_animation = p_anim;
	_anim_length = _animation.is_valid() ? _animation->get_length() : 0.0f;
	if (is_inside_tree()) {
		_bind_tracks();
		// Autoplay when animation is set after entering tree (script _ready timing).
		if (_autoplay && _parent && _animation.is_valid()) {
			play_all();
		}
	}
}

Ref<Animation> MultiNodeAnimator::get_animation() const { return _animation; }

void MultiNodeAnimator::set_autoplay(bool p_auto) { _autoplay = p_auto; }
bool MultiNodeAnimator::get_autoplay() const { return _autoplay; }

void MultiNodeAnimator::set_sync_mode(int p_mode) { _sync_mode = p_mode; }
int MultiNodeAnimator::get_sync_mode() const { return _sync_mode; }

void MultiNodeAnimator::set_sync_offset(float p_offset) { _sync_offset = Math::clamp(p_offset, 0.0f, 1.0f); }
float MultiNodeAnimator::get_sync_offset() const { return _sync_offset; }

void MultiNodeAnimator::set_speed_scale(float p_speed) { _speed_scale = p_speed; }
float MultiNodeAnimator::get_speed_scale() const { return _speed_scale; }

// ---------------------------------------------------------------------------
// Playback control
// ---------------------------------------------------------------------------

void MultiNodeAnimator::play_all() {
	if (!_parent) {
		return;
	}
	_resize_state(_parent->get_instance_count());

	int count = _times.size();

	// Snapshot base transforms for additive transform tracks.
	if (_has_transform_tracks) {
		_base_transforms.resize(count);
		for (int i = 0; i < count; i++) {
			_base_transforms.write[i] = _parent->get_instance_transform(i);
		}
	}

	for (int i = 0; i < count; i++) {
		_playing.set(i, 1);
		if (_sync_mode == SYNC_SYNC) {
			// Chain offset: each instance is offset from the previous by
			// sync_offset * anim_length. Wraps via fmod.
			double offset = Math::fmod((double)i * (double)_sync_offset * (double)_anim_length, (double)_anim_length);
			_times.set(i, offset);
		} else {
			// None: start from 0 — user controls timing via play_instance / scripts.
			_times.set(i, 0.0);
		}
	}
}

void MultiNodeAnimator::stop_all() {
	_playing.fill(0);
}

void MultiNodeAnimator::play_instance(int p_index, float p_from_time) {
	if (p_index >= 0 && p_index < _playing.size()) {
		_playing.set(p_index, 1);
		_times.set(p_index, (double)p_from_time);
		// Snapshot base transform for additive playback.
		if (_has_transform_tracks && _parent && p_index < (int)_base_transforms.size()) {
			_base_transforms.write[p_index] = _parent->get_instance_transform(p_index);
		}
	}
}

void MultiNodeAnimator::stop_instance(int p_index) {
	if (p_index >= 0 && p_index < _playing.size()) {
		_playing.set(p_index, 0);
	}
}

void MultiNodeAnimator::set_instance_speed(int p_index, float p_speed) {
	if (p_index >= 0 && p_index < _speeds.size()) {
		_speeds.set(p_index, p_speed);
	}
}

float MultiNodeAnimator::get_instance_time(int p_index) const {
	if (p_index >= 0 && p_index < _times.size()) {
		return (float)_times[p_index];
	}
	return 0.0f;
}

// ---------------------------------------------------------------------------
// Track binding — resolve animation track paths to targets
// ---------------------------------------------------------------------------

void MultiNodeAnimator::_bind_tracks() {
	_bound_tracks.clear();
	_has_position_track = false;
	_has_rotation_track = false;
	_has_scale_track = false;
	_has_transform_tracks = false;

	if (!_animation.is_valid() || !_parent) {
		_anim_length = 0.0f;
		return;
	}

	_anim_length = _animation->get_length();
	int track_count = _animation->get_track_count();

	for (int t = 0; t < track_count; t++) {
		BoundTrack bt;
		bt.track_index = t;
		bt.type = _animation->track_get_type(t);
		NodePath path = _animation->track_get_path(t);
		String path_str = path;

		// --- Transform tracks: write to parent MultiNode ---
		if (bt.type == Animation::TYPE_POSITION_3D) {
			bt.target = TARGET_PARENT_TRANSFORM;
			bt.is_position = true;
			_has_position_track = true;
			_bound_tracks.push_back(bt);
			continue;
		}
		if (bt.type == Animation::TYPE_ROTATION_3D) {
			bt.target = TARGET_PARENT_TRANSFORM;
			bt.is_rotation = true;
			_has_rotation_track = true;
			_bound_tracks.push_back(bt);
			continue;
		}
		if (bt.type == Animation::TYPE_SCALE_3D) {
			bt.target = TARGET_PARENT_TRANSFORM;
			bt.is_scale = true;
			_has_scale_track = true;
			_bound_tracks.push_back(bt);
			continue;
		}

		// --- Value/bezier tracks: resolve path to sibling sub-node property ---
		// Path format: "NodeName:property" or just ":property" (targets parent).
		// We look for the node among the parent's children (our siblings).
		if (bt.type == Animation::TYPE_VALUE || bt.type == Animation::TYPE_BEZIER) {
			// Split path into node name and property.
			StringName prop = path.get_concatenated_subnames();
			NodePath node_path = NodePath(path.get_concatenated_names());

			// Try to find the target node as a sibling under the parent.
			Node *target = nullptr;
			if (node_path == NodePath() || node_path == NodePath(".")) {
				// Targets the parent MultiNode itself.
				target = _parent;
			} else {
				target = _parent->get_node_or_null(node_path);
			}

			if (target && prop != StringName()) {
				bt.target = TARGET_SUB_PROPERTY;
				bt.target_node_id = target->get_instance_id();
				bt.property_name = prop;

				// Check for set_instance_X pattern (preferred for per-instance data).
				String setter = "set_instance_" + String(prop);
				if (target->has_method(setter)) {
					bt.method_name = setter;
				}

				_bound_tracks.push_back(bt);
			}
			continue;
		}

		// --- Method tracks: call a method on a sibling node per instance ---
		if (bt.type == Animation::TYPE_METHOD) {
			NodePath node_path = NodePath(path.get_concatenated_names());
			Node *target = nullptr;
			if (node_path == NodePath() || node_path == NodePath(".")) {
				target = _parent;
			} else {
				target = _parent->get_node_or_null(node_path);
			}
			if (target) {
				bt.target = TARGET_METHOD_CALL;
				bt.target_node_id = target->get_instance_id();
				_bound_tracks.push_back(bt);
			}
			continue;
		}

		// Other track types — stored but not evaluated.
		bt.target = TARGET_NONE;
		_bound_tracks.push_back(bt);
	}

	_has_transform_tracks = _has_position_track || _has_rotation_track || _has_scale_track;
}

// ---------------------------------------------------------------------------
// State management
// ---------------------------------------------------------------------------

void MultiNodeAnimator::_resize_state(int p_count) {
	int old_count = _times.size();
	_times.resize(p_count);
	_speeds.resize(p_count);
	_playing.resize(p_count);
	_base_transforms.resize(p_count);

	for (int i = old_count; i < p_count; i++) {
		_times.set(i, 0.0);
		_speeds.set(i, 1.0f);
		_playing.set(i, 0);
		_base_transforms.write[i] = Transform3D();
	}
}

// ---------------------------------------------------------------------------
// Core evaluation — the hot loop
// ---------------------------------------------------------------------------

void MultiNodeAnimator::_evaluate_all(double p_delta) {
	if (!_parent || !_animation.is_valid() || _anim_length <= 0.0f) {
		return;
	}

	int count = _parent->get_instance_count();
	if (count == 0 || _times.size() != count) {
		return;
	}

	int active_size = _active.size();
	const uint8_t *active_ptr = active_size > 0 ? _active.ptr() : nullptr;
	const uint8_t *playing_ptr = _playing.ptr();
	bool is_looping = _animation->get_loop_mode() == Animation::LOOP_LINEAR;

	// --- Phase 1: Advance all instance times ---
	for (int i = 0; i < count; i++) {
		if (active_ptr && i < active_size && !active_ptr[i]) {
			continue;
		}
		if (!playing_ptr[i]) {
			continue;
		}

		double new_time = _times[i] + p_delta * (double)_speeds[i] * (double)_speed_scale;

		if (is_looping) {
			new_time = Math::fmod(new_time, (double)_anim_length);
		} else if (new_time > (double)_anim_length) {
			new_time = (double)_anim_length;
			_playing.set(i, 0);
		}

		_times.set(i, new_time);
	}

	// --- Phase 2: Evaluate all tracks for all instances ---
	// Use cached flags from _bind_tracks() instead of scanning every frame.

	if (_has_transform_tracks) {
		_parent->begin_batch();
	}

	int bound_count = _bound_tracks.size();
	const BoundTrack *bound_ptr = _bound_tracks.ptr();

	for (int i = 0; i < count; i++) {
		if (active_ptr && i < active_size && !active_ptr[i]) {
			continue;
		}
		if (!playing_ptr[i]) {
			continue;
		}

		double t = _times[i];

		// Evaluate transform tracks.
		Vector3 pos;
		Quaternion rot;
		Vector3 scl = Vector3(1, 1, 1);

		for (int b = 0; b < bound_count; b++) {
			const BoundTrack &bt = bound_ptr[b];

			if (bt.target == TARGET_PARENT_TRANSFORM) {
				if (bt.is_position) {
					pos = _animation->position_track_interpolate(bt.track_index, t);
				} else if (bt.is_rotation) {
					rot = _animation->rotation_track_interpolate(bt.track_index, t);
				} else if (bt.is_scale) {
					scl = _animation->scale_track_interpolate(bt.track_index, t);
				}
			} else if (bt.target == TARGET_SUB_PROPERTY) {
				Node *target = bt.get_target_node();
				if (!target) continue;
				// Evaluate value/bezier track and write to the target.
				Variant value;
				if (bt.type == Animation::TYPE_VALUE) {
					value = _animation->value_track_interpolate(bt.track_index, t);
				} else if (bt.type == Animation::TYPE_BEZIER) {
					value = _animation->bezier_track_interpolate(bt.track_index, t);
				}

				if (bt.method_name != StringName()) {
					// Per-instance setter: set_instance_color(index, value), etc.
					target->call(bt.method_name, i, value);
				} else {
					// Direct property set (applies to all instances, not per-instance).
					target->set(bt.property_name, value);
				}
			} else if (bt.target == TARGET_METHOD_CALL) {
				Node *target = bt.get_target_node();
				if (!target) continue;
				// Fire method calls whose keyframe time falls within this frame's window.
				double advance = p_delta * (double)_speeds[i] * (double)_speed_scale;
				double prev_t = t - advance;
				int key_count = _animation->track_get_key_count(bt.track_index);
				for (int k = 0; k < key_count; k++) {
					double key_time = _animation->track_get_key_time(bt.track_index, k);
					// Check if this key fired during the current frame window.
					bool in_window = (key_time > prev_t && key_time <= t);
					// Handle loop wraparound.
					if (is_looping && prev_t < 0.0) {
						in_window = in_window || (key_time > prev_t + (double)_anim_length && key_time <= (double)_anim_length);
					}
					if (in_window) {
						StringName method = _animation->method_track_get_name(bt.track_index, k);
						Array params = _animation->method_track_get_params(bt.track_index, k);
						// Prepend instance index as first argument.
						Array call_params;
						call_params.push_back(i);
						call_params.append_array(params);
						target->callv(method, call_params);
					}
				}
			}
		}

		if (_has_transform_tracks && i < (int)_base_transforms.size()) {
			// Additive: compose animated transform on top of the base transform
			// captured at play time. Each instance keeps its own base position.
			const Transform3D &base = _base_transforms[i];
			Basis anim_basis = Basis(rot).scaled(scl);
			Transform3D xform;
			xform.basis = base.basis * anim_basis;
			xform.origin = base.origin + pos;
			_parent->set_instance_transform(i, xform);
		}
	}

	if (_has_transform_tracks) {
		_parent->end_batch();
	}
}
