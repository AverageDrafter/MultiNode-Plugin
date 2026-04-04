#ifndef MULTI_NODE_ANIMATOR_H
#define MULTI_NODE_ANIMATOR_H

#include "multi_node_sub.h"
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

class MultiNodeMesh;

/// General-purpose per-instance animator for MultiNode.
/// Shares ONE Animation resource across all instances, each with independent
/// playback state (time, speed, playing/paused).
///
/// Sync modes:
///   NONE — each instance animates from whenever it was played. Use this for
///          manual per-instance control via scripts.
///   SYNC — all instances share a base clock. sync_offset (0-1) controls
///          the chain offset between consecutive instances:
///          0.0 = perfect lockstep, 0.5 = each instance offset half the
///          animation length from the previous, 1.0 = wraps fully (= lockstep).
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

	void set_animation(const Ref<Animation> &p_anim);
	Ref<Animation> get_animation() const;

	void set_autoplay(bool p_auto);
	bool get_autoplay() const;

	void set_sync_mode(int p_mode);
	int get_sync_mode() const;

	void set_sync_offset(float p_offset);
	float get_sync_offset() const;

	void set_speed_scale(float p_speed);
	float get_speed_scale() const;

	void play_all();
	void stop_all();
	void play_instance(int p_index, float p_from_time = 0.0f);
	void stop_instance(int p_index);
	void set_instance_speed(int p_index, float p_speed);
	float get_instance_time(int p_index) const;

private:
	Ref<Animation> _animation;
	bool _autoplay = true;
	int _sync_mode = SYNC_NONE;
	float _sync_offset = 0.0f;
	float _speed_scale = 1.0f;

	// Per-instance playback state.
	PackedFloat64Array _times;
	PackedFloat32Array _speeds;
	PackedByteArray _playing; // 0 = paused, 1 = playing.

	// Base transforms captured at play_all()/play_instance() time.
	// Transform tracks are applied additively on top of these.
	Vector<Transform3D> _base_transforms;

	float _anim_length = 0.0f;

	// --- Track binding ---
	enum TrackTarget {
		TARGET_NONE,
		TARGET_PARENT_TRANSFORM,
		TARGET_SUB_PROPERTY,
		TARGET_METHOD_CALL,
	};

	struct BoundTrack {
		int track_index = -1;
		Animation::TrackType type = Animation::TYPE_VALUE;
		TrackTarget target = TARGET_NONE;

		bool is_position = false;
		bool is_rotation = false;
		bool is_scale = false;

		uint64_t target_node_id = 0;
		StringName property_name;
		StringName method_name;

		Node *get_target_node() const;
	};

	Vector<BoundTrack> _bound_tracks;

	// Cached flags from _bind_tracks() to avoid scanning _bound_tracks every frame.
	bool _has_position_track = false;
	bool _has_rotation_track = false;
	bool _has_scale_track = false;
	bool _has_transform_tracks = false;

	void _bind_tracks();
	void _resize_state(int p_count);
	void _evaluate_all(double p_delta);
};

} // namespace godot

VARIANT_ENUM_CAST(MultiNodeAnimator::SyncMode);

#endif // MULTI_NODE_ANIMATOR_H
