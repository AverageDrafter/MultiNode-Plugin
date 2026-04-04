#ifndef MULTI_NODE_AUDIO_H
#define MULTI_NODE_AUDIO_H

#include "multi_node_3d.h"
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_generator.hpp>
#include <godot_cpp/classes/audio_stream_generator_playback.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

/// Software-mixed spatial audio for MultiNode instances.
/// Decodes the source audio to raw PCM, then mixes ALL playing instances
/// into a single AudioStreamGenerator output with per-instance distance
/// attenuation, stereo panning, and optional distance low-pass filtering.
/// No voice limit — 1000 simultaneous sources through one audio channel.
///
/// Requires AudioStreamWAV format (16-bit or 8-bit) for the source stream.
class MultiNodeAudio : public MultiNode3D {
	GDCLASS(MultiNodeAudio, MultiNode3D)

protected:
	static void _bind_methods();
	void _notification(int p_what);

	void _on_sub_entered() override;
	void _on_sub_exiting() override;
	void _on_parent_sync() override;
	void _on_active_changed() override;

public:
	MultiNodeAudio();
	~MultiNodeAudio();

	void set_stream(const Ref<AudioStream> &p_stream);
	Ref<AudioStream> get_stream() const;

	void set_volume_db(float p_db);
	float get_volume_db() const;

	void set_max_distance(float p_dist);
	float get_max_distance() const;

	void set_reference_distance(float p_dist);
	float get_reference_distance() const;

	void set_bus(const StringName &p_bus);
	StringName get_bus() const;

	void set_loop(bool p_loop);
	bool get_loop() const;

	void set_autoplay(bool p_auto);
	bool get_autoplay() const;

	void set_amplify_db(float p_db);
	float get_amplify_db() const;

	void set_auto_pause(bool p_enable);
	bool get_auto_pause() const;

	void set_pitch_randomization(float p_amount);
	float get_pitch_randomization() const;

	void set_volume_randomization(float p_db);
	float get_volume_randomization() const;

	void set_distance_filter_enabled(bool p_enable);
	bool get_distance_filter_enabled() const;

	void set_distance_filter_strength(float p_strength);
	float get_distance_filter_strength() const;

	// --- Per-instance control ---
	void play_instance(int p_index, float p_from = 0.0f);
	void stop_instance(int p_index);
	void play_all();
	void stop_all();

	void set_instance_playing(int p_index, bool p_playing);
	bool get_instance_playing(int p_index) const;

	void set_instance_volume(int p_index, float p_db);
	float get_instance_volume(int p_index) const;

	void set_instance_pitch(int p_index, float p_scale);
	float get_instance_pitch(int p_index) const;

	float get_instance_playback_position(int p_index) const;

private:
	Ref<AudioStream> _stream;
	float _volume_db = 0.0f;
	float _max_distance = 50.0f;
	float _reference_distance = 1.0f;
	StringName _bus = "Master";
	bool _loop = false;
	bool _autoplay = false;
	float _amplify_db = 0.0f;
	bool _auto_pause = true;
	float _pitch_randomization = 0.0f;
	float _volume_randomization = 0.0f;
	bool _distance_filter_enabled = true;
	float _distance_filter_strength = 0.5f;

	// Decoded source audio (mono, float32, normalized -1 to 1).
	PackedFloat32Array _source_samples;
	int _source_sample_rate = 44100;
	int _source_sample_count = 0;
	bool _decoded = false;

	// Per-instance playback state.
	// _inst_pitches / _inst_volumes: user-facing values (unchanged by randomization).
	// _inst_play_pitches: runtime pitch used by the mix loop (base * randomization).
	// _inst_volumes_linear: pre-converted linear volume (updated on set + play).
	// _inst_lp_state: single-pole low-pass filter state per instance.
	PackedByteArray _inst_playing;
	PackedFloat32Array _inst_volumes;       // user-set dB per instance
	PackedFloat32Array _inst_volumes_linear; // pre-converted linear, used in mix
	PackedFloat32Array _inst_pitches;        // user-set pitch scale per instance
	PackedFloat32Array _inst_play_pitches;   // runtime pitch (base + randomization)
	PackedFloat64Array _inst_positions;      // playback position in source samples
	PackedFloat32Array _inst_lp_state;       // LP filter state per instance

	// Output.
	AudioStreamPlayer *_output_player = nullptr;
	Ref<AudioStreamGenerator> _generator;
	Ref<AudioStreamGeneratorPlayback> _gen_playback;
	int _output_mix_rate = 44100;

	// Persistent buffers to avoid per-frame allocation in _fill_buffer().
	struct VoiceData {
		int index;
		float vol_left;
		float vol_right;
		float pitch_advance;
		float lp_coeff;
	};
	Vector<VoiceData> _voices;
	PackedVector2Array _silence_buffer;
	PackedVector2Array _mix_buffer;

	void _decode_stream();
	void _setup_output();
	void _cleanup_output();
	void _fill_buffer();
	void _resize_state(int p_count);
	void _apply_randomization(int p_index);

	static float _db_to_linear(float p_db);
};

} // namespace godot

#endif // MULTI_NODE_AUDIO_H
