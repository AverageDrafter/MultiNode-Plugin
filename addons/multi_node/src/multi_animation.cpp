#include "multi_animation.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

MultiAnimation::MultiAnimation() {}
MultiAnimation::~MultiAnimation() {}

// ---------------------------------------------------------------------------
// Mode
// ---------------------------------------------------------------------------

void MultiAnimation::set_play_mode(int p_mode) {
	_play_mode = CLAMP(p_mode, 0, 2);
	emit_changed();
}

int MultiAnimation::get_play_mode() const {
	return _play_mode;
}

void MultiAnimation::set_reverse(bool p_reverse) {
	_reverse = p_reverse;
	emit_changed();
}

bool MultiAnimation::get_reverse() const {
	return _reverse;
}

// ---------------------------------------------------------------------------
// Default transition
// ---------------------------------------------------------------------------

void MultiAnimation::set_default_duration(float p_dur) {
	_default_duration = MAX(p_dur, 0.0f);
	emit_changed();
}

float MultiAnimation::get_default_duration() const {
	return _default_duration;
}

void MultiAnimation::set_default_easing(int p_ease) {
	_default_easing = CLAMP(p_ease, 0, 3);
	emit_changed();
}

int MultiAnimation::get_default_easing() const {
	return _default_easing;
}

void MultiAnimation::set_default_trans_type(int p_trans) {
	_default_trans_type = CLAMP(p_trans, 0, 11);
	emit_changed();
}

int MultiAnimation::get_default_trans_type() const {
	return _default_trans_type;
}

// ---------------------------------------------------------------------------
// Key CRUD
// ---------------------------------------------------------------------------

int MultiAnimation::add_key(const StringName &p_name) {
	AnimKey k;
	k.name = p_name;
	_keys.push_back(k);
	emit_changed();
	return _keys.size() - 1;
}

void MultiAnimation::remove_key(int p_index) {
	ERR_FAIL_INDEX(p_index, _keys.size());

	// Remove transitions that reference this key.
	for (int i = _transitions.size() - 1; i >= 0; i--) {
		if (_transitions[i].from_key == p_index || _transitions[i].to_key == p_index) {
			_transitions.remove_at(i);
		}
	}
	// Adjust transition indices above the removed key.
	for (int i = 0; i < _transitions.size(); i++) {
		if (_transitions[i].from_key > p_index) {
			_transitions.write[i].from_key--;
		}
		if (_transitions[i].to_key > p_index) {
			_transitions.write[i].to_key--;
		}
	}

	_keys.remove_at(p_index);
	emit_changed();
}

int MultiAnimation::get_key_count() const {
	return _keys.size();
}

void MultiAnimation::set_key_name(int p_index, const StringName &p_name) {
	ERR_FAIL_INDEX(p_index, _keys.size());
	_keys.write[p_index].name = p_name;
	emit_changed();
}

StringName MultiAnimation::get_key_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _keys.size(), StringName());
	return _keys[p_index].name;
}

int MultiAnimation::find_key(const StringName &p_name) const {
	for (int i = 0; i < _keys.size(); i++) {
		if (_keys[i].name == p_name) {
			return i;
		}
	}
	return -1;
}

// ---------------------------------------------------------------------------
// Key properties
// ---------------------------------------------------------------------------

void MultiAnimation::set_key_property(int p_key_index, const StringName &p_path, const Variant &p_value) {
	ERR_FAIL_INDEX(p_key_index, _keys.size());
	_keys.write[p_key_index].properties[p_path] = p_value;
	emit_changed();
}

Variant MultiAnimation::get_key_property(int p_key_index, const StringName &p_path) const {
	ERR_FAIL_INDEX_V(p_key_index, _keys.size(), Variant());
	const Dictionary &props = _keys[p_key_index].properties;
	if (props.has(p_path)) {
		return props[p_path];
	}
	return Variant();
}

void MultiAnimation::remove_key_property(int p_key_index, const StringName &p_path) {
	ERR_FAIL_INDEX(p_key_index, _keys.size());
	_keys.write[p_key_index].properties.erase(p_path);
	emit_changed();
}

Dictionary MultiAnimation::get_key_properties(int p_key_index) const {
	ERR_FAIL_INDEX_V(p_key_index, _keys.size(), Dictionary());
	return _keys[p_key_index].properties;
}

// ---------------------------------------------------------------------------
// Key hold time
// ---------------------------------------------------------------------------

void MultiAnimation::set_key_hold_time(int p_index, float p_time) {
	ERR_FAIL_INDEX(p_index, _keys.size());
	_keys.write[p_index].hold_time = MAX(p_time, 0.0f);
	emit_changed();
}

float MultiAnimation::get_key_hold_time(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _keys.size(), 0.0f);
	return _keys[p_index].hold_time;
}

// ---------------------------------------------------------------------------
// Key method calls
// ---------------------------------------------------------------------------

void MultiAnimation::set_key_method(int p_key_index, const StringName &p_method, const Array &p_args) {
	ERR_FAIL_INDEX(p_key_index, _keys.size());
	_keys.write[p_key_index].methods[p_method] = p_args;
	emit_changed();
}

void MultiAnimation::remove_key_method(int p_key_index, const StringName &p_method) {
	ERR_FAIL_INDEX(p_key_index, _keys.size());
	_keys.write[p_key_index].methods.erase(p_method);
	emit_changed();
}

Dictionary MultiAnimation::get_key_methods(int p_key_index) const {
	ERR_FAIL_INDEX_V(p_key_index, _keys.size(), Dictionary());
	return _keys[p_key_index].methods;
}

// ---------------------------------------------------------------------------
// Transition CRUD
// ---------------------------------------------------------------------------

int MultiAnimation::add_transition(int p_from, int p_to) {
	ERR_FAIL_INDEX_V(p_from, _keys.size(), -1);
	ERR_FAIL_INDEX_V(p_to, _keys.size(), -1);
	ERR_FAIL_COND_V(p_from == p_to, -1);

	// Don't add duplicate edges.
	if (find_transition(p_from, p_to) >= 0) {
		return find_transition(p_from, p_to);
	}

	AnimTransition t;
	t.from_key = p_from;
	t.to_key = p_to;
	t.duration = _default_duration;
	t.easing = _default_easing;
	t.trans_type = _default_trans_type;
	_transitions.push_back(t);
	emit_changed();
	return _transitions.size() - 1;
}

void MultiAnimation::remove_transition(int p_index) {
	ERR_FAIL_INDEX(p_index, _transitions.size());
	_transitions.remove_at(p_index);
	emit_changed();
}

int MultiAnimation::get_transition_count() const {
	return _transitions.size();
}

int MultiAnimation::find_transition(int p_from, int p_to) const {
	for (int i = 0; i < _transitions.size(); i++) {
		if (_transitions[i].from_key == p_from && _transitions[i].to_key == p_to) {
			return i;
		}
	}
	return -1;
}

// ---------------------------------------------------------------------------
// Transition properties
// ---------------------------------------------------------------------------

void MultiAnimation::set_transition_duration(int p_index, float p_dur) {
	ERR_FAIL_INDEX(p_index, _transitions.size());
	_transitions.write[p_index].duration = MAX(p_dur, 0.0f);
	emit_changed();
}

float MultiAnimation::get_transition_duration(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _transitions.size(), 0.0f);
	return _transitions[p_index].duration;
}

void MultiAnimation::set_transition_easing(int p_index, int p_ease) {
	ERR_FAIL_INDEX(p_index, _transitions.size());
	_transitions.write[p_index].easing = CLAMP(p_ease, 0, 3);
	emit_changed();
}

int MultiAnimation::get_transition_easing(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _transitions.size(), 0);
	return _transitions[p_index].easing;
}

void MultiAnimation::set_transition_trans_type(int p_index, int p_trans) {
	ERR_FAIL_INDEX(p_index, _transitions.size());
	_transitions.write[p_index].trans_type = CLAMP(p_trans, 0, 11);
	emit_changed();
}

int MultiAnimation::get_transition_trans_type(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _transitions.size(), 0);
	return _transitions[p_index].trans_type;
}

int MultiAnimation::get_transition_from(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _transitions.size(), -1);
	return _transitions[p_index].from_key;
}

int MultiAnimation::get_transition_to(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _transitions.size(), -1);
	return _transitions[p_index].to_key;
}

void MultiAnimation::set_transition_trigger(int p_index, int p_trigger) {
	ERR_FAIL_INDEX(p_index, _transitions.size());
	_transitions.write[p_index].trigger = CLAMP(p_trigger, 0, 1);
	emit_changed();
}

int MultiAnimation::get_transition_trigger(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _transitions.size(), 0);
	return _transitions[p_index].trigger;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

PackedInt32Array MultiAnimation::get_transitions_from(int p_key_index) const {
	PackedInt32Array result;
	for (int i = 0; i < _transitions.size(); i++) {
		if (_transitions[i].from_key == p_key_index) {
			result.push_back(i);
		}
	}
	return result;
}

bool MultiAnimation::can_transition(int p_from, int p_to) const {
	return find_transition(p_from, p_to) >= 0;
}

int MultiAnimation::find_auto_transition_from(int p_key_index) const {
	for (int i = 0; i < _transitions.size(); i++) {
		if (_transitions[i].from_key == p_key_index && _transitions[i].trigger == TRIGGER_AUTO) {
			return i;
		}
	}
	return -1;
}

// ---------------------------------------------------------------------------
// Serialization — Array-of-Dictionary for clean .tres storage
// ---------------------------------------------------------------------------

void MultiAnimation::set_keys_data(const Array &p_data) {
	_keys.clear();
	_keys.resize(p_data.size());
	for (int i = 0; i < p_data.size(); i++) {
		const Dictionary &d = p_data[i];
		AnimKey &k = _keys.write[i];
		k.name = d.get("name", StringName());
		k.properties = d.get("properties", Dictionary());
		k.methods = d.get("methods", Dictionary());
		k.hold_time = d.get("hold_time", 0.0f);
	}
	emit_changed();
}

Array MultiAnimation::get_keys_data() const {
	Array out;
	out.resize(_keys.size());
	for (int i = 0; i < _keys.size(); i++) {
		Dictionary d;
		d["name"] = _keys[i].name;
		d["properties"] = _keys[i].properties;
		d["methods"] = _keys[i].methods;
		d["hold_time"] = _keys[i].hold_time;
		out[i] = d;
	}
	return out;
}

void MultiAnimation::set_transitions_data(const Array &p_data) {
	_transitions.clear();
	_transitions.resize(p_data.size());
	for (int i = 0; i < p_data.size(); i++) {
		const Dictionary &d = p_data[i];
		AnimTransition &t = _transitions.write[i];
		t.from_key = d.get("from", 0);
		t.to_key = d.get("to", 0);
		t.duration = d.get("duration", 0.3f);
		t.easing = d.get("easing", (int)EASE_IN_OUT);
		t.trans_type = d.get("trans_type", (int)TRANS_LINEAR);
		t.trigger = d.get("trigger", (int)TRIGGER_AUTO);
	}
	emit_changed();
}

Array MultiAnimation::get_transitions_data() const {
	Array out;
	out.resize(_transitions.size());
	for (int i = 0; i < _transitions.size(); i++) {
		Dictionary d;
		d["from"] = _transitions[i].from_key;
		d["to"] = _transitions[i].to_key;
		d["duration"] = _transitions[i].duration;
		d["easing"] = _transitions[i].easing;
		d["trans_type"] = _transitions[i].trans_type;
		d["trigger"] = _transitions[i].trigger;
		out[i] = d;
	}
	return out;
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void MultiAnimation::_bind_methods() {
	// --- Play settings ---

	ClassDB::bind_method(D_METHOD("set_play_mode", "play_mode"), &MultiAnimation::set_play_mode);
	ClassDB::bind_method(D_METHOD("get_play_mode"), &MultiAnimation::get_play_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "play_mode", PROPERTY_HINT_ENUM, "One Shot,Loop,Ping Pong"),
			"set_play_mode", "get_play_mode");

	ClassDB::bind_method(D_METHOD("set_reverse", "reverse"), &MultiAnimation::set_reverse);
	ClassDB::bind_method(D_METHOD("get_reverse"), &MultiAnimation::get_reverse);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reverse"), "set_reverse", "get_reverse");

	// --- Default transition ---
	ADD_GROUP("Default Transition", "default_");

	ClassDB::bind_method(D_METHOD("set_default_duration", "duration"), &MultiAnimation::set_default_duration);
	ClassDB::bind_method(D_METHOD("get_default_duration"), &MultiAnimation::get_default_duration);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_duration", PROPERTY_HINT_RANGE, "0.0,10.0,0.01,suffix:s"),
			"set_default_duration", "get_default_duration");

	ClassDB::bind_method(D_METHOD("set_default_easing", "easing"), &MultiAnimation::set_default_easing);
	ClassDB::bind_method(D_METHOD("get_default_easing"), &MultiAnimation::get_default_easing);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "default_easing", PROPERTY_HINT_ENUM, "Ease In,Ease Out,Ease In-Out,Ease Out-In"),
			"set_default_easing", "get_default_easing");

	ClassDB::bind_method(D_METHOD("set_default_trans_type", "trans_type"), &MultiAnimation::set_default_trans_type);
	ClassDB::bind_method(D_METHOD("get_default_trans_type"), &MultiAnimation::get_default_trans_type);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "default_trans_type", PROPERTY_HINT_ENUM,
						 "Linear,Sine,Quint,Quart,Quad,Expo,Elastic,Cubic,Circ,Bounce,Back,Spring"),
			"set_default_trans_type", "get_default_trans_type");

	// --- Key API ---
	ClassDB::bind_method(D_METHOD("add_key", "name"), &MultiAnimation::add_key);
	ClassDB::bind_method(D_METHOD("remove_key", "index"), &MultiAnimation::remove_key);
	ClassDB::bind_method(D_METHOD("get_key_count"), &MultiAnimation::get_key_count);
	ClassDB::bind_method(D_METHOD("set_key_name", "index", "name"), &MultiAnimation::set_key_name);
	ClassDB::bind_method(D_METHOD("get_key_name", "index"), &MultiAnimation::get_key_name);
	ClassDB::bind_method(D_METHOD("find_key", "name"), &MultiAnimation::find_key);

	ClassDB::bind_method(D_METHOD("set_key_property", "key_index", "path", "value"), &MultiAnimation::set_key_property);
	ClassDB::bind_method(D_METHOD("get_key_property", "key_index", "path"), &MultiAnimation::get_key_property);
	ClassDB::bind_method(D_METHOD("remove_key_property", "key_index", "path"), &MultiAnimation::remove_key_property);
	ClassDB::bind_method(D_METHOD("get_key_properties", "key_index"), &MultiAnimation::get_key_properties);

	ClassDB::bind_method(D_METHOD("set_key_hold_time", "index", "time"), &MultiAnimation::set_key_hold_time);
	ClassDB::bind_method(D_METHOD("get_key_hold_time", "index"), &MultiAnimation::get_key_hold_time);

	ClassDB::bind_method(D_METHOD("set_key_method", "key_index", "method", "args"), &MultiAnimation::set_key_method);
	ClassDB::bind_method(D_METHOD("remove_key_method", "key_index", "method"), &MultiAnimation::remove_key_method);
	ClassDB::bind_method(D_METHOD("get_key_methods", "key_index"), &MultiAnimation::get_key_methods);

	// --- Transition API ---
	ClassDB::bind_method(D_METHOD("add_transition", "from", "to"), &MultiAnimation::add_transition);
	ClassDB::bind_method(D_METHOD("remove_transition", "index"), &MultiAnimation::remove_transition);
	ClassDB::bind_method(D_METHOD("get_transition_count"), &MultiAnimation::get_transition_count);
	ClassDB::bind_method(D_METHOD("find_transition", "from", "to"), &MultiAnimation::find_transition);

	ClassDB::bind_method(D_METHOD("set_transition_duration", "index", "duration"), &MultiAnimation::set_transition_duration);
	ClassDB::bind_method(D_METHOD("get_transition_duration", "index"), &MultiAnimation::get_transition_duration);
	ClassDB::bind_method(D_METHOD("set_transition_easing", "index", "easing"), &MultiAnimation::set_transition_easing);
	ClassDB::bind_method(D_METHOD("get_transition_easing", "index"), &MultiAnimation::get_transition_easing);
	ClassDB::bind_method(D_METHOD("set_transition_trans_type", "index", "trans_type"), &MultiAnimation::set_transition_trans_type);
	ClassDB::bind_method(D_METHOD("get_transition_trans_type", "index"), &MultiAnimation::get_transition_trans_type);
	ClassDB::bind_method(D_METHOD("get_transition_from", "index"), &MultiAnimation::get_transition_from);
	ClassDB::bind_method(D_METHOD("get_transition_to", "index"), &MultiAnimation::get_transition_to);
	ClassDB::bind_method(D_METHOD("set_transition_trigger", "index", "trigger"), &MultiAnimation::set_transition_trigger);
	ClassDB::bind_method(D_METHOD("get_transition_trigger", "index"), &MultiAnimation::get_transition_trigger);

	// --- Queries ---
	ClassDB::bind_method(D_METHOD("get_transitions_from", "key_index"), &MultiAnimation::get_transitions_from);
	ClassDB::bind_method(D_METHOD("can_transition", "from", "to"), &MultiAnimation::can_transition);
	ClassDB::bind_method(D_METHOD("find_auto_transition_from", "key_index"), &MultiAnimation::find_auto_transition_from);

	// --- Serialization (storage-only, not shown in inspector) ---
	ClassDB::bind_method(D_METHOD("set_keys_data", "data"), &MultiAnimation::set_keys_data);
	ClassDB::bind_method(D_METHOD("get_keys_data"), &MultiAnimation::get_keys_data);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "keys_data", PROPERTY_HINT_NONE, "",
						 PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_NO_EDITOR),
			"set_keys_data", "get_keys_data");

	ClassDB::bind_method(D_METHOD("set_transitions_data", "data"), &MultiAnimation::set_transitions_data);
	ClassDB::bind_method(D_METHOD("get_transitions_data"), &MultiAnimation::get_transitions_data);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "transitions_data", PROPERTY_HINT_NONE, "",
						 PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_NO_EDITOR),
			"set_transitions_data", "get_transitions_data");

	// --- Enum constants ---
	BIND_ENUM_CONSTANT(TRIGGER_AUTO);
	BIND_ENUM_CONSTANT(TRIGGER_MANUAL);

	BIND_ENUM_CONSTANT(PLAY_ONE_SHOT);
	BIND_ENUM_CONSTANT(PLAY_LOOP);
	BIND_ENUM_CONSTANT(PLAY_PING_PONG);

	BIND_ENUM_CONSTANT(EASE_IN);
	BIND_ENUM_CONSTANT(EASE_OUT);
	BIND_ENUM_CONSTANT(EASE_IN_OUT);
	BIND_ENUM_CONSTANT(EASE_OUT_IN);

	BIND_ENUM_CONSTANT(TRANS_LINEAR);
	BIND_ENUM_CONSTANT(TRANS_SINE);
	BIND_ENUM_CONSTANT(TRANS_QUINT);
	BIND_ENUM_CONSTANT(TRANS_QUART);
	BIND_ENUM_CONSTANT(TRANS_QUAD);
	BIND_ENUM_CONSTANT(TRANS_EXPO);
	BIND_ENUM_CONSTANT(TRANS_ELASTIC);
	BIND_ENUM_CONSTANT(TRANS_CUBIC);
	BIND_ENUM_CONSTANT(TRANS_CIRC);
	BIND_ENUM_CONSTANT(TRANS_BOUNCE);
	BIND_ENUM_CONSTANT(TRANS_BACK);
	BIND_ENUM_CONSTANT(TRANS_SPRING);
}
