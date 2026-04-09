#ifndef MULTI_ANIMATION_H
#define MULTI_ANIMATION_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

/// Agnostic animation resource: named keys (state snapshots) with transitions.
///
/// Two topology modes:
///   LINEAR   — keys in sequential order; auto-advances through them.
///              Covers standard animation and tweening.
///   CIRCULAR — any key can transition to any other key via explicit edges.
///              Covers state machines.
///
/// Keys store arbitrary property values as Dictionary (path -> Variant).
/// Transitions store per-edge duration, easing, and transition type.
/// Serializes cleanly to .tres via Array-of-Dictionary properties.
class MultiAnimation : public Resource {
	GDCLASS(MultiAnimation, Resource)

public:
	enum TriggerType {
		TRIGGER_AUTO = 0,    // Auto-advance on arrival at from_key.
		TRIGGER_MANUAL = 1,  // Only via go_to_key() call.
	};

	enum PlayMode {
		PLAY_ONE_SHOT = 0,
		PLAY_LOOP = 1,
		PLAY_PING_PONG = 2,
	};

	// Easing types (mirrors Tween::EaseType for simplicity)
	enum EaseType {
		EASE_IN = 0,
		EASE_OUT = 1,
		EASE_IN_OUT = 2,
		EASE_OUT_IN = 3,
	};

	// Transition curve types (mirrors Tween::TransitionType)
	enum TransType {
		TRANS_LINEAR = 0,
		TRANS_SINE = 1,
		TRANS_QUINT = 2,
		TRANS_QUART = 3,
		TRANS_QUAD = 4,
		TRANS_EXPO = 5,
		TRANS_ELASTIC = 6,
		TRANS_CUBIC = 7,
		TRANS_CIRC = 8,
		TRANS_BOUNCE = 9,
		TRANS_BACK = 10,
		TRANS_SPRING = 11,
	};

protected:
	static void _bind_methods();

public:
	MultiAnimation();
	~MultiAnimation();

	// --- Play settings ---
	void set_play_mode(int p_mode);
	int get_play_mode() const;

	void set_reverse(bool p_reverse);
	bool get_reverse() const;

	// --- Default transition (LINEAR fallback) ---
	void set_default_duration(float p_dur);
	float get_default_duration() const;

	void set_default_easing(int p_ease);
	int get_default_easing() const;

	void set_default_trans_type(int p_trans);
	int get_default_trans_type() const;

	// --- Key CRUD ---
	int add_key(const StringName &p_name);
	void remove_key(int p_index);
	int get_key_count() const;
	void set_key_name(int p_index, const StringName &p_name);
	StringName get_key_name(int p_index) const;
	int find_key(const StringName &p_name) const;

	// --- Key properties ---
	void set_key_property(int p_key_index, const StringName &p_path, const Variant &p_value);
	Variant get_key_property(int p_key_index, const StringName &p_path) const;
	void remove_key_property(int p_key_index, const StringName &p_path);
	Dictionary get_key_properties(int p_key_index) const;

	// --- Key hold time (LINEAR mode dwell) ---
	void set_key_hold_time(int p_index, float p_time);
	float get_key_hold_time(int p_index) const;

	// --- Key method calls (fired on arrival) ---
	void set_key_method(int p_key_index, const StringName &p_method, const Array &p_args);
	void remove_key_method(int p_key_index, const StringName &p_method);
	Dictionary get_key_methods(int p_key_index) const;

	// --- Transition CRUD ---
	int add_transition(int p_from, int p_to);
	void remove_transition(int p_index);
	int get_transition_count() const;
	int find_transition(int p_from, int p_to) const;

	// --- Transition properties ---
	void set_transition_duration(int p_index, float p_dur);
	float get_transition_duration(int p_index) const;

	void set_transition_easing(int p_index, int p_ease);
	int get_transition_easing(int p_index) const;

	void set_transition_trans_type(int p_index, int p_trans);
	int get_transition_trans_type(int p_index) const;

	int get_transition_from(int p_index) const;
	int get_transition_to(int p_index) const;

	void set_transition_trigger(int p_index, int p_trigger);
	int get_transition_trigger(int p_index) const;

	// --- Queries ---
	PackedInt32Array get_transitions_from(int p_key_index) const;
	bool can_transition(int p_from, int p_to) const;
	int find_auto_transition_from(int p_key_index) const;

	// --- Serialization (Array-of-Dictionary for .tres) ---
	void set_keys_data(const Array &p_data);
	Array get_keys_data() const;

	void set_transitions_data(const Array &p_data);
	Array get_transitions_data() const;

private:
	struct AnimKey {
		StringName name;
		Dictionary properties;
		Dictionary methods; // StringName method_name -> Array args
		float hold_time = 0.0f;
	};

	struct AnimTransition {
		int from_key = 0;
		int to_key = 0;
		float duration = 0.3f;
		int easing = EASE_IN_OUT;
		int trans_type = TRANS_LINEAR;
		int trigger = TRIGGER_AUTO;
	};

	int _play_mode = PLAY_LOOP;
	bool _reverse = false;
	Vector<AnimKey> _keys;
	Vector<AnimTransition> _transitions;

	// Default transition settings for LINEAR auto-advance.
	float _default_duration = 0.3f;
	int _default_easing = EASE_IN_OUT;
	int _default_trans_type = TRANS_LINEAR;
};

} // namespace godot

VARIANT_ENUM_CAST(MultiAnimation::TriggerType);
VARIANT_ENUM_CAST(MultiAnimation::PlayMode);
VARIANT_ENUM_CAST(MultiAnimation::EaseType);
VARIANT_ENUM_CAST(MultiAnimation::TransType);

#endif // MULTI_ANIMATION_H
