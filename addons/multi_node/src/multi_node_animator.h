#ifndef MULTI_NODE_ANIMATOR_H
#define MULTI_NODE_ANIMATOR_H

#include "multi_node_sub.h"
#include "multi_animation.h"
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

namespace godot {

/// Per-instance key-based animator for MultiNode.
///
/// Plays a MultiAnimation resource across all instances, each with independent
/// state (current key, target key, progress, speed).
///
/// LINEAR mode: auto-advances each instance through keys in order.
/// CIRCULAR mode: each instance idles at its current key until
///                go_to_key_instance() is called.
///
/// Method calls: keys can define methods that fire when an instance arrives.
///               Instance index is prepended as the first argument.
///
/// Sync modes:
///   NONE — each instance independent, user controls timing.
///   SYNC — instances start at staggered key positions.
class MultiNodeAnimator : public MultiNodeSub {
	GDCLASS(MultiNodeAnimator, MultiNodeSub)

public:
	enum SyncMode {
		SYNC_NONE = 0,
		SYNC_SYNC = 1,
	};

protected:
	static void _bind_methods();
	void _notification(int p_what);

	void _on_sub_entered() override;
	void _on_sub_exiting() override;
	void _on_parent_sync() override;

public:
	MultiNodeAnimator();
	~MultiNodeAnimator();

	// --- Resource ---
	void set_animation_resource(const Ref<MultiAnimation> &p_res);
	Ref<MultiAnimation> get_animation_resource() const;

	void set_autoplay(bool p_auto);
	bool get_autoplay() const;

	void set_speed_scale(float p_speed);
	float get_speed_scale() const;

	void set_sync_mode(int p_mode);
	int get_sync_mode() const;

	void set_sync_offset(float p_offset);
	float get_sync_offset() const;

	// --- Bulk playback ---
	void play_all(const StringName &p_from_key = StringName());
	void stop_all();

	// --- Per-instance control ---
	void play_instance(int p_index, const StringName &p_from_key = StringName());
	void stop_instance(int p_index);
	void go_to_key_instance(int p_index, const StringName &p_name);
	void set_instance_speed(int p_index, float p_speed);

	// --- Per-instance queries ---
	StringName get_instance_current_key(int p_index) const;
	StringName get_instance_target_key(int p_index) const;
	float get_instance_progress(int p_index) const;

	// --- Arrived instances (batched signal) ---
	PackedInt32Array get_arrived_instances() const;

private:
	Ref<MultiAnimation> _resource;
	bool _autoplay = true;
	float _speed_scale = 1.0f;
	int _sync_mode = SYNC_NONE;
	float _sync_offset = 0.0f;

	// Per-instance state (parallel arrays).
	PackedInt32Array _current_keys;
	PackedInt32Array _target_keys;   // -1 = at rest.
	PackedFloat32Array _progresses;
	PackedFloat32Array _hold_timers;
	PackedFloat32Array _speeds;
	PackedByteArray _playing;

	// Base transforms for additive animation.
	Vector<Transform3D> _base_transforms;

	// Property binding — resolved at bind time.
	struct BoundProperty {
		StringName path;
		uint64_t target_node_id = 0;
		StringName property_name;
		StringName setter_method;      // "set_instance_X" if exists.
		bool has_per_instance_setter = false;
		bool is_position = false;
		bool is_rotation = false;
		bool is_scale = false;
	};

	// Method call binding — resolved at bind time.
	struct BoundMethod {
		StringName method_name;
		uint64_t target_node_id = 0;
	};

	Vector<BoundProperty> _bound_props;
	Vector<BoundMethod> _bound_methods;
	bool _has_transform_props = false;

	// Arrived-this-frame buffer.
	PackedInt32Array _arrived_this_frame;

	// --- Internal ---
	void _bind_properties();
	void _resize_state(int p_count);
	void _evaluate_all(double p_delta);
	void _fire_key_methods(int p_instance, int p_key_index);

	struct TransitionCache {
		float duration = 0.3f;
		int easing = MultiAnimation::EASE_IN_OUT;
		int trans_type = MultiAnimation::TRANS_LINEAR;
	};
	TransitionCache _get_transition_cache(int p_from, int p_to) const;
};

} // namespace godot

VARIANT_ENUM_CAST(MultiNodeAnimator::SyncMode);

#endif // MULTI_NODE_ANIMATOR_H
