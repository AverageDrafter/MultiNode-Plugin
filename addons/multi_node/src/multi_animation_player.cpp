#include "multi_animation_player.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/object.hpp>

using namespace godot;

MultiAnimationPlayer::MultiAnimationPlayer() {}
MultiAnimationPlayer::~MultiAnimationPlayer() {}

// ---------------------------------------------------------------------------
// Target resolution
// ---------------------------------------------------------------------------

Node *MultiAnimationPlayer::_get_target_node() const {
	if (!is_inside_tree()) {
		return nullptr;
	}
	return get_node_or_null(_target_path);
}

void MultiAnimationPlayer::_resolve_property(const StringName &p_path, Node *&r_target, StringName &r_prop) const {
	r_target = nullptr;
	r_prop = StringName();

	Node *base = _get_target_node();
	if (!base) {
		return;
	}

	String s = String(p_path);
	int colon = s.find(":");
	if (colon >= 0) {
		// "NodeName:property" — look for sibling.
		String node_name = s.substr(0, colon);
		String prop_name = s.substr(colon + 1);
		Node *parent = base->get_parent();
		if (parent) {
			r_target = parent->get_node_or_null(NodePath(node_name));
		}
		r_prop = StringName(prop_name);
	} else {
		// Direct property on the target node.
		r_target = base;
		r_prop = p_path;
	}
}

// ---------------------------------------------------------------------------
// Transition management
// ---------------------------------------------------------------------------

void MultiAnimationPlayer::_begin_transition(int p_from, int p_to) {
	if (!_resource.is_valid()) {
		return;
	}

	_target_key = p_to;
	_progress = 0.0f;

	// Look up explicit transition, or fall back to defaults.
	int tidx = _resource->find_transition(p_from, p_to);
	if (tidx >= 0) {
		_trans_duration = _resource->get_transition_duration(tidx);
		_trans_easing = _resource->get_transition_easing(tidx);
		_trans_type = _resource->get_transition_trans_type(tidx);
	} else {
		_trans_duration = _resource->get_default_duration();
		_trans_easing = _resource->get_default_easing();
		_trans_type = _resource->get_default_trans_type();
	}

	// Cache from/to property values for interpolation.
	_cached_props.clear();

	Dictionary from_props = _resource->get_key_properties(p_from);
	Dictionary to_props = _resource->get_key_properties(p_to);

	// Collect the union of property paths from both keys.
	Array all_paths;
	Array from_keys = from_props.keys();
	for (int i = 0; i < from_keys.size(); i++) {
		all_paths.push_back(from_keys[i]);
	}
	Array to_keys = to_props.keys();
	for (int i = 0; i < to_keys.size(); i++) {
		if (!from_props.has(to_keys[i])) {
			all_paths.push_back(to_keys[i]);
		}
	}

	for (int i = 0; i < all_paths.size(); i++) {
		StringName path = all_paths[i];
		CachedProperty cp;
		cp.path = path;

		// If a key is missing a property, use the other key's value (no interpolation for that prop).
		if (from_props.has(path) && to_props.has(path)) {
			cp.from_value = from_props[path];
			cp.to_value = to_props[path];
		} else if (from_props.has(path)) {
			cp.from_value = from_props[path];
			cp.to_value = from_props[path];
		} else {
			cp.from_value = to_props[path];
			cp.to_value = to_props[path];
		}

		// Resolve target node + property name.
		Node *target = nullptr;
		StringName prop;
		_resolve_property(path, target, prop);
		if (target) {
			cp.target_node_id = target->get_instance_id();
			cp.property_name = prop;
		}

		_cached_props.push_back(cp);
	}

	emit_signal("transition_started", _resource->get_key_name(p_from), _resource->get_key_name(p_to));
}

void MultiAnimationPlayer::_apply_key(int p_key_index) {
	if (!_resource.is_valid()) {
		return;
	}

	Dictionary props = _resource->get_key_properties(p_key_index);
	Array paths = props.keys();
	for (int i = 0; i < paths.size(); i++) {
		StringName path = paths[i];
		Node *target = nullptr;
		StringName prop;
		_resolve_property(path, target, prop);
		if (target) {
			target->set(prop, props[path]);
		}
	}
}

// ---------------------------------------------------------------------------
// Variant interpolation
// ---------------------------------------------------------------------------

Variant MultiAnimationPlayer::_interpolate_variant(const Variant &p_from, const Variant &p_to, float p_weight) {
	Variant::Type type = p_from.get_type();

	// Type mismatch — snap.
	if (type != p_to.get_type()) {
		return p_weight < 0.5f ? p_from : p_to;
	}

	switch (type) {
		case Variant::FLOAT:
			return Math::lerp((double)p_from, (double)p_to, (double)p_weight);

		case Variant::INT:
			return (int)Math::lerp((double)(int)p_from, (double)(int)p_to, (double)p_weight);

		case Variant::VECTOR2: {
			Vector2 a = p_from;
			Vector2 b = p_to;
			return a.lerp(b, p_weight);
		}

		case Variant::VECTOR3: {
			Vector3 a = p_from;
			Vector3 b = p_to;
			return a.lerp(b, p_weight);
		}

		case Variant::QUATERNION: {
			Quaternion a = p_from;
			Quaternion b = p_to;
			return a.slerp(b, p_weight);
		}

		case Variant::COLOR: {
			Color a = p_from;
			Color b = p_to;
			return a.lerp(b, p_weight);
		}

		case Variant::BASIS: {
			Basis a = p_from;
			Basis b = p_to;
			Quaternion qa = a.get_quaternion();
			Quaternion qb = b.get_quaternion();
			return Basis(qa.slerp(qb, p_weight));
		}

		case Variant::TRANSFORM3D: {
			Transform3D a = p_from;
			Transform3D b = p_to;
			Transform3D result;
			Quaternion qa = a.basis.get_quaternion();
			Quaternion qb = b.basis.get_quaternion();
			result.basis = Basis(qa.slerp(qb, p_weight));
			result.origin = a.origin.lerp(b.origin, p_weight);
			return result;
		}

		case Variant::BOOL:
			return p_weight < 0.5f ? p_from : p_to;

		default:
			// Non-interpolatable types snap at the end.
			return p_weight < 1.0f ? p_from : p_to;
	}
}

// ---------------------------------------------------------------------------
// Easing — maps to Tween-style curves
// ---------------------------------------------------------------------------

float MultiAnimationPlayer::_apply_easing(float p_t, int p_ease, int p_trans) {
	// Clamp input.
	float t = CLAMP(p_t, 0.0f, 1.0f);

	// Transition function (base curve shape).
	auto trans_func = [](float t, int trans) -> float {
		switch (trans) {
			case MultiAnimation::TRANS_LINEAR: return t;
			case MultiAnimation::TRANS_SINE: return 1.0f - Math::cos(t * Math_PI * 0.5f);
			case MultiAnimation::TRANS_QUINT: return t * t * t * t * t;
			case MultiAnimation::TRANS_QUART: return t * t * t * t;
			case MultiAnimation::TRANS_QUAD: return t * t;
			case MultiAnimation::TRANS_EXPO: return t == 0.0f ? 0.0f : Math::pow(2.0f, 10.0f * (t - 1.0f));
			case MultiAnimation::TRANS_ELASTIC: {
				float p = 0.3f;
				return -Math::pow(2.0f, 10.0f * (t - 1.0f)) * Math::sin((t - 1.0f - p / 4.0f) * (2.0f * Math_PI) / p);
			}
			case MultiAnimation::TRANS_CUBIC: return t * t * t;
			case MultiAnimation::TRANS_CIRC: return 1.0f - Math::sqrt(1.0f - t * t);
			case MultiAnimation::TRANS_BOUNCE: {
				float out_t = 1.0f - t;
				float val;
				if (out_t < 1.0f / 2.75f) {
					val = 7.5625f * out_t * out_t;
				} else if (out_t < 2.0f / 2.75f) {
					out_t -= 1.5f / 2.75f;
					val = 7.5625f * out_t * out_t + 0.75f;
				} else if (out_t < 2.5f / 2.75f) {
					out_t -= 2.25f / 2.75f;
					val = 7.5625f * out_t * out_t + 0.9375f;
				} else {
					out_t -= 2.625f / 2.75f;
					val = 7.5625f * out_t * out_t + 0.984375f;
				}
				return 1.0f - val;
			}
			case MultiAnimation::TRANS_BACK: {
				float s = 1.70158f;
				return t * t * ((s + 1.0f) * t - s);
			}
			case MultiAnimation::TRANS_SPRING: {
				float decay = 0.3f;
				return 1.0f - Math::cos(t * Math_PI * (0.2f + 2.5f * t * t * t)) * Math::pow(1.0f - t, 2.2f + decay);
			}
			default: return t;
		}
	};

	// Ease direction.
	switch (p_ease) {
		case MultiAnimation::EASE_IN:
			return trans_func(t, p_trans);
		case MultiAnimation::EASE_OUT:
			return 1.0f - trans_func(1.0f - t, p_trans);
		case MultiAnimation::EASE_IN_OUT:
			if (t < 0.5f) {
				return trans_func(t * 2.0f, p_trans) * 0.5f;
			} else {
				return (1.0f - trans_func((1.0f - t) * 2.0f, p_trans)) * 0.5f + 0.5f;
			}
		case MultiAnimation::EASE_OUT_IN:
			if (t < 0.5f) {
				return (1.0f - trans_func(1.0f - t * 2.0f, p_trans)) * 0.5f;
			} else {
				return trans_func(t * 2.0f - 1.0f, p_trans) * 0.5f + 0.5f;
			}
		default:
			return t;
	}
}

// ---------------------------------------------------------------------------
// Advance — per-frame update
// ---------------------------------------------------------------------------

void MultiAnimationPlayer::_advance(double p_delta) {
	if (!_playing || !_resource.is_valid()) {
		return;
	}

	_advance_unified(p_delta);
}

void MultiAnimationPlayer::_advance_unified(double p_delta) {
	int key_count = _resource->get_key_count();
	if (key_count == 0) {
		return;
	}

	if (_target_key >= 0) {
		// --- Transitioning between keys ---
		if (_trans_duration > 0.0f) {
			_progress += (float)(p_delta * (double)_speed_scale) / _trans_duration;
		} else {
			_progress = 1.0f;
		}

		if (_progress >= 1.0f) {
			_progress = 1.0f;
			StringName from_name = _resource->get_key_name(_current_key);
			StringName to_name = _resource->get_key_name(_target_key);
			_current_key = _target_key;
			_target_key = -1;
			_apply_key(_current_key);
			_hold_timer = _resource->get_key_hold_time(_current_key);
			emit_signal("transition_completed", from_name, to_name);
			emit_signal("key_reached", _resource->get_key_name(_current_key));
		} else {
			float eased = _apply_easing(_progress, _trans_easing, _trans_type);
			for (int i = 0; i < _cached_props.size(); i++) {
				const CachedProperty &cp = _cached_props[i];
				if (cp.target_node_id == 0) continue;
				Object *obj = ObjectDB::get_instance(ObjectID(cp.target_node_id));
				Node *target = obj ? Object::cast_to<Node>(obj) : nullptr;
				if (target) {
					Variant val = _interpolate_variant(cp.from_value, cp.to_value, eased);
					target->set(cp.property_name, val);
				}
			}
		}
	} else {
		// --- At rest on a key — look for AUTO transitions ---
		int auto_tidx = _resource->find_auto_transition_from(_current_key);
		if (auto_tidx >= 0) {
			_hold_timer -= (float)(p_delta * (double)_speed_scale);
			if (_hold_timer <= 0.0f) {
				int next = _resource->get_transition_to(auto_tidx);
				_begin_transition(_current_key, next);
			}
		} else {
			// No AUTO transition — check play mode for loop wrap.
			int play_mode = _resource->get_play_mode();
			if (play_mode == MultiAnimation::PLAY_LOOP) {
				_hold_timer -= (float)(p_delta * (double)_speed_scale);
				if (_hold_timer <= 0.0f) {
					bool reverse = _resource->get_reverse();
					int next = reverse ? key_count - 1 : 0;
					if (next != _current_key) {
						_begin_transition(_current_key, next);
					} else {
						_playing = false;
					}
				}
			}
			// ONE_SHOT / PING_PONG with no AUTO: idle until go_to_key().
		}
	}
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------

void MultiAnimationPlayer::set_animation_resource(const Ref<MultiAnimation> &p_res) {
	_resource = p_res;
}

Ref<MultiAnimation> MultiAnimationPlayer::get_animation_resource() const {
	return _resource;
}

void MultiAnimationPlayer::set_autoplay(bool p_auto) { _autoplay = p_auto; }
bool MultiAnimationPlayer::get_autoplay() const { return _autoplay; }

void MultiAnimationPlayer::set_speed_scale(float p_speed) { _speed_scale = p_speed; }
float MultiAnimationPlayer::get_speed_scale() const { return _speed_scale; }

void MultiAnimationPlayer::set_target_path(const NodePath &p_path) { _target_path = p_path; }
NodePath MultiAnimationPlayer::get_target_path() const { return _target_path; }

// ---------------------------------------------------------------------------
// Playback control
// ---------------------------------------------------------------------------

void MultiAnimationPlayer::play(const StringName &p_from_key) {
	if (!_resource.is_valid() || _resource->get_key_count() == 0) {
		return;
	}

	if (p_from_key != StringName()) {
		int idx = _resource->find_key(p_from_key);
		if (idx >= 0) {
			_current_key = idx;
		}
	} else {
		_current_key = 0;
	}

	_target_key = -1;
	_progress = 0.0f;
	_playing = true;
	_apply_key(_current_key);
	_hold_timer = _resource->get_key_hold_time(_current_key);
	emit_signal("key_reached", _resource->get_key_name(_current_key));
}

void MultiAnimationPlayer::stop() {
	_playing = false;
	_target_key = -1;
}

bool MultiAnimationPlayer::is_playing() const {
	return _playing;
}

void MultiAnimationPlayer::go_to_key(const StringName &p_name) {
	if (!_resource.is_valid()) {
		return;
	}

	int target_idx = _resource->find_key(p_name);
	ERR_FAIL_COND_MSG(target_idx < 0, "MultiAnimationPlayer: key not found: " + String(p_name));

	if (target_idx == _current_key) {
		return;
	}

	// Only transition if an edge exists.
	if (!_resource->can_transition(_current_key, target_idx)) {
		WARN_PRINT("MultiAnimationPlayer: no transition from '" +
				String(_resource->get_key_name(_current_key)) + "' to '" + String(p_name) + "'");
		return;
	}

	_playing = true;
	_begin_transition(_current_key, target_idx);
}

StringName MultiAnimationPlayer::get_current_key_name() const {
	if (_resource.is_valid() && _current_key >= 0 && _current_key < _resource->get_key_count()) {
		return _resource->get_key_name(_current_key);
	}
	return StringName();
}

StringName MultiAnimationPlayer::get_target_key_name() const {
	if (_resource.is_valid() && _target_key >= 0 && _target_key < _resource->get_key_count()) {
		return _resource->get_key_name(_target_key);
	}
	return StringName();
}

float MultiAnimationPlayer::get_progress() const {
	return _progress;
}

// ---------------------------------------------------------------------------
// Notification
// ---------------------------------------------------------------------------

void MultiAnimationPlayer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (_autoplay && _resource.is_valid()) {
				play();
			}
			set_process(true);
		} break;

		case NOTIFICATION_PROCESS: {
			_advance(get_process_delta_time());
		} break;
	}
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void MultiAnimationPlayer::_bind_methods() {
	// --- Resource ---
	ClassDB::bind_method(D_METHOD("set_animation_resource", "resource"), &MultiAnimationPlayer::set_animation_resource);
	ClassDB::bind_method(D_METHOD("get_animation_resource"), &MultiAnimationPlayer::get_animation_resource);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "animation_resource", PROPERTY_HINT_RESOURCE_TYPE, "MultiAnimation"),
			"set_animation_resource", "get_animation_resource");

	ClassDB::bind_method(D_METHOD("set_autoplay", "auto"), &MultiAnimationPlayer::set_autoplay);
	ClassDB::bind_method(D_METHOD("get_autoplay"), &MultiAnimationPlayer::get_autoplay);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

	ClassDB::bind_method(D_METHOD("set_speed_scale", "speed"), &MultiAnimationPlayer::set_speed_scale);
	ClassDB::bind_method(D_METHOD("get_speed_scale"), &MultiAnimationPlayer::get_speed_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_scale", PROPERTY_HINT_RANGE, "0.0,10.0,0.01"),
			"set_speed_scale", "get_speed_scale");

	ClassDB::bind_method(D_METHOD("set_target_path", "path"), &MultiAnimationPlayer::set_target_path);
	ClassDB::bind_method(D_METHOD("get_target_path"), &MultiAnimationPlayer::get_target_path);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_path"),
			"set_target_path", "get_target_path");

	// --- Playback ---
	ClassDB::bind_method(D_METHOD("play", "from_key"), &MultiAnimationPlayer::play, DEFVAL(StringName()));
	ClassDB::bind_method(D_METHOD("stop"), &MultiAnimationPlayer::stop);
	ClassDB::bind_method(D_METHOD("is_playing"), &MultiAnimationPlayer::is_playing);
	ClassDB::bind_method(D_METHOD("go_to_key", "name"), &MultiAnimationPlayer::go_to_key);
	ClassDB::bind_method(D_METHOD("get_current_key_name"), &MultiAnimationPlayer::get_current_key_name);
	ClassDB::bind_method(D_METHOD("get_target_key_name"), &MultiAnimationPlayer::get_target_key_name);
	ClassDB::bind_method(D_METHOD("get_progress"), &MultiAnimationPlayer::get_progress);

	// --- Signals ---
	ADD_SIGNAL(MethodInfo("key_reached", PropertyInfo(Variant::STRING_NAME, "key_name")));
	ADD_SIGNAL(MethodInfo("transition_started",
			PropertyInfo(Variant::STRING_NAME, "from_key"),
			PropertyInfo(Variant::STRING_NAME, "to_key")));
	ADD_SIGNAL(MethodInfo("transition_completed",
			PropertyInfo(Variant::STRING_NAME, "from_key"),
			PropertyInfo(Variant::STRING_NAME, "to_key")));
}
