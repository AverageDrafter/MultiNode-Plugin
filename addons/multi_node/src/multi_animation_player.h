#ifndef MULTI_ANIMATION_PLAYER_H
#define MULTI_ANIMATION_PLAYER_H

#include "multi_animation.h"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

/// Agnostic animation player: plays a MultiAnimation resource on a target node.
///
/// Works with any scene node — not tied to MultiNode. Resolves property paths
/// from keys to the target node and its siblings. Interpolates Variant values
/// between keys during transitions.
///
/// LINEAR mode: auto-advances through keys in order (with optional hold_time).
/// CIRCULAR mode: idles at current key until go_to_key() is called.
class MultiAnimationPlayer : public Node {
	GDCLASS(MultiAnimationPlayer, Node)

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	MultiAnimationPlayer();
	~MultiAnimationPlayer();

	// --- Resource ---
	void set_animation_resource(const Ref<MultiAnimation> &p_res);
	Ref<MultiAnimation> get_animation_resource() const;

	void set_autoplay(bool p_auto);
	bool get_autoplay() const;

	void set_speed_scale(float p_speed);
	float get_speed_scale() const;

	void set_target_path(const NodePath &p_path);
	NodePath get_target_path() const;

	// --- Playback ---
	void play(const StringName &p_from_key = StringName());
	void stop();
	bool is_playing() const;

	void go_to_key(const StringName &p_name);
	StringName get_current_key_name() const;
	StringName get_target_key_name() const;
	float get_progress() const;

private:
	Ref<MultiAnimation> _resource;
	bool _autoplay = true;
	float _speed_scale = 1.0f;
	NodePath _target_path = NodePath("..");

	// Playback state.
	int _current_key = 0;
	int _target_key = -1;  // -1 = at rest (not transitioning).
	float _progress = 0.0f;
	float _hold_timer = 0.0f;
	bool _playing = false;

	// Cached transition for current interpolation.
	float _trans_duration = 0.3f;
	int _trans_easing = MultiAnimation::EASE_IN_OUT;
	int _trans_type = MultiAnimation::TRANS_LINEAR;

	// Cached from/to property values for the active transition.
	struct CachedProperty {
		StringName path;
		Variant from_value;
		Variant to_value;
		uint64_t target_node_id = 0;
		StringName property_name;
	};
	Vector<CachedProperty> _cached_props;

	// --- Internal ---
	Node *_get_target_node() const;
	void _resolve_property(const StringName &p_path, Node *&r_target, StringName &r_prop) const;
	void _begin_transition(int p_from, int p_to);
	void _apply_key(int p_key_index);
	void _advance(double p_delta);
	void _advance_unified(double p_delta);

public:
	static Variant _interpolate_variant(const Variant &p_from, const Variant &p_to, float p_weight);
	static float _apply_easing(float p_t, int p_ease, int p_trans);
};

} // namespace godot

#endif // MULTI_ANIMATION_PLAYER_H
