#include "multi_node_audio.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cmath>

using namespace godot;

MultiNodeAudio::MultiNodeAudio() {}
MultiNodeAudio::~MultiNodeAudio() {}

void MultiNodeAudio::_bind_methods() {
	ADD_GROUP("Playback", "");

	ClassDB::bind_method(D_METHOD("set_stream", "stream"), &MultiNodeAudio::set_stream);
	ClassDB::bind_method(D_METHOD("get_stream"), &MultiNodeAudio::get_stream);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stream", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream"), "set_stream", "get_stream");

	ClassDB::bind_method(D_METHOD("set_volume_db", "db"), &MultiNodeAudio::set_volume_db);
	ClassDB::bind_method(D_METHOD("get_volume_db"), &MultiNodeAudio::get_volume_db);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_db", PROPERTY_HINT_RANGE, "-80.0,24.0,0.1,suffix:dB"), "set_volume_db", "get_volume_db");

	ClassDB::bind_method(D_METHOD("set_amplify_db", "db"), &MultiNodeAudio::set_amplify_db);
	ClassDB::bind_method(D_METHOD("get_amplify_db"), &MultiNodeAudio::get_amplify_db);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "amplify_db", PROPERTY_HINT_RANGE, "-20.0,40.0,0.1,suffix:dB"), "set_amplify_db", "get_amplify_db");

	ClassDB::bind_method(D_METHOD("set_bus", "bus"), &MultiNodeAudio::set_bus);
	ClassDB::bind_method(D_METHOD("get_bus"), &MultiNodeAudio::get_bus);
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "bus"), "set_bus", "get_bus");

	ClassDB::bind_method(D_METHOD("set_loop", "loop"), &MultiNodeAudio::set_loop);
	ClassDB::bind_method(D_METHOD("get_loop"), &MultiNodeAudio::get_loop);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "loop"), "set_loop", "get_loop");

	ClassDB::bind_method(D_METHOD("set_autoplay", "auto"), &MultiNodeAudio::set_autoplay);
	ClassDB::bind_method(D_METHOD("get_autoplay"), &MultiNodeAudio::get_autoplay);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

	ADD_GROUP("Spatial", "");

	ClassDB::bind_method(D_METHOD("set_max_distance", "distance"), &MultiNodeAudio::set_max_distance);
	ClassDB::bind_method(D_METHOD("get_max_distance"), &MultiNodeAudio::get_max_distance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "1.0,1000.0,1.0,suffix:m"), "set_max_distance", "get_max_distance");

	ClassDB::bind_method(D_METHOD("set_reference_distance", "distance"), &MultiNodeAudio::set_reference_distance);
	ClassDB::bind_method(D_METHOD("get_reference_distance"), &MultiNodeAudio::get_reference_distance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reference_distance", PROPERTY_HINT_RANGE, "0.1,100.0,0.1,suffix:m"), "set_reference_distance", "get_reference_distance");

	ClassDB::bind_method(D_METHOD("set_auto_pause", "enable"), &MultiNodeAudio::set_auto_pause);
	ClassDB::bind_method(D_METHOD("get_auto_pause"), &MultiNodeAudio::get_auto_pause);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_pause"), "set_auto_pause", "get_auto_pause");

	ADD_GROUP("Randomization", "");

	ClassDB::bind_method(D_METHOD("set_pitch_randomization", "amount"), &MultiNodeAudio::set_pitch_randomization);
	ClassDB::bind_method(D_METHOD("get_pitch_randomization"), &MultiNodeAudio::get_pitch_randomization);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch_randomization", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_pitch_randomization", "get_pitch_randomization");

	ClassDB::bind_method(D_METHOD("set_volume_randomization", "db"), &MultiNodeAudio::set_volume_randomization);
	ClassDB::bind_method(D_METHOD("get_volume_randomization"), &MultiNodeAudio::get_volume_randomization);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_randomization", PROPERTY_HINT_RANGE, "0.0,20.0,0.1,suffix:dB"), "set_volume_randomization", "get_volume_randomization");

	ADD_GROUP("Distance Filter", "distance_filter_");

	ClassDB::bind_method(D_METHOD("set_distance_filter_enabled", "enable"), &MultiNodeAudio::set_distance_filter_enabled);
	ClassDB::bind_method(D_METHOD("get_distance_filter_enabled"), &MultiNodeAudio::get_distance_filter_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "distance_filter_enabled"), "set_distance_filter_enabled", "get_distance_filter_enabled");

	ClassDB::bind_method(D_METHOD("set_distance_filter_strength", "strength"), &MultiNodeAudio::set_distance_filter_strength);
	ClassDB::bind_method(D_METHOD("get_distance_filter_strength"), &MultiNodeAudio::get_distance_filter_strength);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "distance_filter_strength", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_distance_filter_strength", "get_distance_filter_strength");

	ADD_GROUP("", "");

	ClassDB::bind_method(D_METHOD("play_instance", "index", "from_seconds"), &MultiNodeAudio::play_instance, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("stop_instance", "index"), &MultiNodeAudio::stop_instance);
	ClassDB::bind_method(D_METHOD("play_all"), &MultiNodeAudio::play_all);
	ClassDB::bind_method(D_METHOD("stop_all"), &MultiNodeAudio::stop_all);

	ClassDB::bind_method(D_METHOD("set_instance_playing", "index", "playing"), &MultiNodeAudio::set_instance_playing);
	ClassDB::bind_method(D_METHOD("get_instance_playing", "index"), &MultiNodeAudio::get_instance_playing);

	ClassDB::bind_method(D_METHOD("set_instance_volume", "index", "db"), &MultiNodeAudio::set_instance_volume);
	ClassDB::bind_method(D_METHOD("get_instance_volume", "index"), &MultiNodeAudio::get_instance_volume);

	ClassDB::bind_method(D_METHOD("set_instance_pitch", "index", "scale"), &MultiNodeAudio::set_instance_pitch);
	ClassDB::bind_method(D_METHOD("get_instance_pitch", "index"), &MultiNodeAudio::get_instance_pitch);

	ClassDB::bind_method(D_METHOD("get_instance_playback_position", "index"), &MultiNodeAudio::get_instance_playback_position);
}

// ---------------------------------------------------------------------------
// Notification
// ---------------------------------------------------------------------------

void MultiNodeAudio::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			_fill_buffer();
		} break;
	}
}

// ---------------------------------------------------------------------------
// MultiNodeSub overrides
// ---------------------------------------------------------------------------

void MultiNodeAudio::_on_sub_entered() {
	_output_mix_rate = (int)AudioServer::get_singleton()->get_mix_rate();
	_decode_stream();
	_setup_output();
	set_process(true);
	if (_autoplay && _parent && _decoded) {
		_resize_state(_parent->get_instance_count());
		play_all();
	}
}

void MultiNodeAudio::_on_sub_exiting() {
	set_process(false);
	_cleanup_output();
}

void MultiNodeAudio::_on_parent_sync() {
	if (_parent) {
		int count = _parent->get_instance_count();
		if (count == _inst_playing.size()) {
			return;
		}
		bool was_empty = (_inst_playing.size() == 0);
		_resize_state(count);
		if (was_empty && count > 0 && _autoplay && _decoded) {
			play_all();
		}
	}
}

void MultiNodeAudio::_on_active_changed() {
	for (int i = 0; i < _inst_playing.size(); i++) {
		if (i >= _active.size()) continue;
		if (!_active[i]) {
			// Stop instances that became inactive.
			_inst_playing.set(i, 0);
		} else if (_autoplay && !_inst_playing[i]) {
			// Start instances that became newly active (respects autoplay flag).
			_inst_playing.set(i, 1);
			_inst_positions.set(i, 0.0);
			_apply_randomization(i);
		}
	}
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------

void MultiNodeAudio::set_stream(const Ref<AudioStream> &p_stream) {
	_stream = p_stream;
	if (is_inside_tree()) {
		_decode_stream();
	}
}

Ref<AudioStream> MultiNodeAudio::get_stream() const { return _stream; }

void MultiNodeAudio::set_volume_db(float p_db) { _volume_db = p_db; }
float MultiNodeAudio::get_volume_db() const { return _volume_db; }

void MultiNodeAudio::set_max_distance(float p_dist) { _max_distance = MAX(0.1f, p_dist); }
float MultiNodeAudio::get_max_distance() const { return _max_distance; }

void MultiNodeAudio::set_reference_distance(float p_dist) { _reference_distance = MAX(0.1f, p_dist); }
float MultiNodeAudio::get_reference_distance() const { return _reference_distance; }

void MultiNodeAudio::set_bus(const StringName &p_bus) {
	_bus = p_bus;
	if (_output_player) {
		_output_player->set_bus(p_bus);
	}
}

StringName MultiNodeAudio::get_bus() const { return _bus; }

void MultiNodeAudio::set_loop(bool p_loop) { _loop = p_loop; }
bool MultiNodeAudio::get_loop() const { return _loop; }

void MultiNodeAudio::set_autoplay(bool p_auto) { _autoplay = p_auto; }
bool MultiNodeAudio::get_autoplay() const { return _autoplay; }

void MultiNodeAudio::set_amplify_db(float p_db) { _amplify_db = p_db; }
float MultiNodeAudio::get_amplify_db() const { return _amplify_db; }

void MultiNodeAudio::set_auto_pause(bool p_enable) { _auto_pause = p_enable; }
bool MultiNodeAudio::get_auto_pause() const { return _auto_pause; }

void MultiNodeAudio::set_pitch_randomization(float p_amount) { _pitch_randomization = Math::clamp(p_amount, 0.0f, 1.0f); }
float MultiNodeAudio::get_pitch_randomization() const { return _pitch_randomization; }

void MultiNodeAudio::set_volume_randomization(float p_db) { _volume_randomization = MAX(0.0f, p_db); }
float MultiNodeAudio::get_volume_randomization() const { return _volume_randomization; }

void MultiNodeAudio::set_distance_filter_enabled(bool p_enable) { _distance_filter_enabled = p_enable; }
bool MultiNodeAudio::get_distance_filter_enabled() const { return _distance_filter_enabled; }

void MultiNodeAudio::set_distance_filter_strength(float p_strength) { _distance_filter_strength = Math::clamp(p_strength, 0.0f, 1.0f); }
float MultiNodeAudio::get_distance_filter_strength() const { return _distance_filter_strength; }

// ---------------------------------------------------------------------------
// Per-instance control
// ---------------------------------------------------------------------------

void MultiNodeAudio::play_instance(int p_index, float p_from) {
	if (p_index < 0 || p_index >= _inst_playing.size() || !_decoded) {
		return;
	}
	// Don't restart if already playing from a non-zero position (natural loop).
	if (_inst_playing[p_index] && _inst_positions[p_index] < (double)_source_sample_count) {
		return;
	}
	_inst_playing.set(p_index, 1);
	double sample_pos = (double)p_from * (double)_source_sample_rate;
	sample_pos = CLAMP(sample_pos, 0.0, (double)(_source_sample_count - 1));
	_inst_positions.set(p_index, sample_pos);
	_apply_randomization(p_index);
}

void MultiNodeAudio::stop_instance(int p_index) {
	if (p_index >= 0 && p_index < _inst_playing.size()) {
		_inst_playing.set(p_index, 0);
	}
}

void MultiNodeAudio::play_all() {
	for (int i = 0; i < _inst_playing.size(); i++) {
		if (_active.size() > 0 && i < _active.size() && !_active[i]) {
			continue;
		}
		_inst_playing.set(i, 1);
		_inst_positions.set(i, 0.0);
		_apply_randomization(i);
	}
}

void MultiNodeAudio::stop_all() {
	_inst_playing.fill(0);
}

void MultiNodeAudio::set_instance_playing(int p_index, bool p_playing) {
	if (p_playing) {
		play_instance(p_index);
	} else {
		stop_instance(p_index);
	}
}

bool MultiNodeAudio::get_instance_playing(int p_index) const {
	if (p_index >= 0 && p_index < _inst_playing.size()) {
		return _inst_playing[p_index] == 1;
	}
	return false;
}

void MultiNodeAudio::set_instance_volume(int p_index, float p_db) {
	if (p_index >= 0 && p_index < _inst_volumes.size()) {
		_inst_volumes.set(p_index, p_db);
		_inst_volumes_linear.set(p_index, _db_to_linear(p_db));
	}
}

float MultiNodeAudio::get_instance_volume(int p_index) const {
	return (p_index >= 0 && p_index < _inst_volumes.size()) ? _inst_volumes[p_index] : 0.0f;
}

void MultiNodeAudio::set_instance_pitch(int p_index, float p_scale) {
	if (p_index >= 0 && p_index < _inst_pitches.size()) {
		_inst_pitches.set(p_index, p_scale);
		_inst_play_pitches.set(p_index, p_scale); // Play pitch matches until next play_instance call.
	}
}

float MultiNodeAudio::get_instance_pitch(int p_index) const {
	// Returns the user-set base pitch, not the randomized runtime pitch.
	return (p_index >= 0 && p_index < _inst_pitches.size()) ? _inst_pitches[p_index] : 1.0f;
}

float MultiNodeAudio::get_instance_playback_position(int p_index) const {
	if (p_index < 0 || p_index >= _inst_positions.size() || _source_sample_rate == 0) {
		return 0.0f;
	}
	return (float)(_inst_positions[p_index] / (double)_source_sample_rate);
}

// ---------------------------------------------------------------------------
// Stream decode — convert AudioStreamWAV to raw float PCM
// ---------------------------------------------------------------------------

void MultiNodeAudio::_decode_stream() {
	_source_samples.clear();
	_source_sample_count = 0;
	_decoded = false;

	if (!_stream.is_valid()) {
		return;
	}

	AudioStreamWAV *wav = Object::cast_to<AudioStreamWAV>(_stream.ptr());
	if (!wav) {
		WARN_PRINT("MultiNodeAudio: Software mixer requires AudioStreamWAV format. Import your audio as WAV.");
		return;
	}

	PackedByteArray data = wav->get_data();
	int format = wav->get_format();
	bool stereo = wav->is_stereo();
	_source_sample_rate = wav->get_mix_rate();

	if (format != AudioStreamWAV::FORMAT_16_BITS && format != AudioStreamWAV::FORMAT_8_BITS) {
		WARN_PRINT(String("MultiNodeAudio: Unsupported WAV format {0}. Need 8-bit (0) or 16-bit (1). "
				"In the Import tab, set Compress Mode to 'Disabled' and reimport.").format(Array::make(format)));
		return;
	}

	int bytes_per_sample = (format == AudioStreamWAV::FORMAT_16_BITS) ? 2 : 1;
	int channels = stereo ? 2 : 1;
	int total_bytes = data.size();
	_source_sample_count = total_bytes / (bytes_per_sample * channels);

	if (_source_sample_count == 0) {
		return;
	}

	_source_samples.resize(_source_sample_count);
	float *out = _source_samples.ptrw();
	const uint8_t *raw = data.ptr();

	for (int i = 0; i < _source_sample_count; i++) {
		int byte_offset = i * bytes_per_sample * channels;

		if (format == AudioStreamWAV::FORMAT_16_BITS) {
			int16_t sample_l = (int16_t)(raw[byte_offset] | (raw[byte_offset + 1] << 8));
			if (stereo) {
				int16_t sample_r = (int16_t)(raw[byte_offset + 2] | (raw[byte_offset + 3] << 8));
				out[i] = ((float)sample_l + (float)sample_r) / 65536.0f;
			} else {
				out[i] = (float)sample_l / 32768.0f;
			}
		} else {
			int sample_l = (int)raw[byte_offset] - 128;
			if (stereo) {
				int sample_r = (int)raw[byte_offset + 1] - 128;
				out[i] = ((float)sample_l + (float)sample_r) / 256.0f;
			} else {
				out[i] = (float)sample_l / 128.0f;
			}
		}
	}

	_decoded = true;
#ifdef DEBUG_ENABLED
	UtilityFunctions::print("MultiNodeAudio: Decoded ", _source_sample_count, " samples at ", _source_sample_rate, "Hz");
#endif
}

// ---------------------------------------------------------------------------
// Output setup — AudioStreamGenerator through a single AudioStreamPlayer
// ---------------------------------------------------------------------------

void MultiNodeAudio::_setup_output() {
	if (_output_player) {
		return;
	}

	_generator.instantiate();
	_generator->set_mix_rate((float)_output_mix_rate);
	_generator->set_buffer_length(0.25f);

	_output_player = memnew(AudioStreamPlayer);
	_output_player->set_stream(_generator);
	_output_player->set_bus(_bus);
	_output_player->set_autoplay(false);
	add_child(_output_player);
	_output_player->play();

	Ref<AudioStreamPlayback> pb = _output_player->get_stream_playback();
	_gen_playback = Ref<AudioStreamGeneratorPlayback>(Object::cast_to<AudioStreamGeneratorPlayback>(pb.ptr()));
	if (!_gen_playback.is_valid()) {
		WARN_PRINT("MultiNodeAudio: No AudioStreamGeneratorPlayback available after play().");
	}
}

void MultiNodeAudio::_cleanup_output() {
	_gen_playback.unref();
	_generator.unref();
	if (_output_player) {
		_output_player->stop();
		_output_player->queue_free();
		_output_player = nullptr;
	}
}

// ---------------------------------------------------------------------------
// THE MIX LOOP — voice-major order for cache-friendly source reads
// ---------------------------------------------------------------------------

void MultiNodeAudio::_fill_buffer() {
	if (!_gen_playback.is_valid() || !_decoded || !_parent) {
		return;
	}

	// Restart generator if buffer starved and playback stopped.
	if (_output_player && !_output_player->is_playing()) {
		_output_player->play();
		Ref<AudioStreamPlayback> pb = _output_player->get_stream_playback();
		_gen_playback = Ref<AudioStreamGeneratorPlayback>(Object::cast_to<AudioStreamGeneratorPlayback>(pb.ptr()));
		if (!_gen_playback.is_valid()) return;
	}

	int frames_available = _gen_playback->get_frames_available();
	if (frames_available == 0) {
		return;
	}

	int inst_count = _parent->get_instance_count();
	if (inst_count == 0 || _inst_playing.size() != inst_count) {
		if (_silence_buffer.size() < frames_available) {
			_silence_buffer.resize(frames_available);
			_silence_buffer.fill(Vector2(0, 0));
		}
		_gen_playback->push_buffer(_silence_buffer);
		return;
	}

	// --- Get listener position and orientation ---
	Vector3 listener_pos;
	Vector3 listener_right;
	Viewport *vp = get_viewport();
	if (vp) {
		Camera3D *cam = vp->get_camera_3d();
		if (cam) {
			Transform3D cam_xform = cam->get_global_transform();
			listener_pos = cam_xform.origin;
			listener_right = cam_xform.basis.get_column(0).normalized();
		}
	}

	Transform3D global_xform = get_global_transform();
	const Vector<Transform3D> &transforms = _parent->get_transforms_internal();
	const uint8_t *active_ptr = _active.size() > 0 ? _active.ptr() : nullptr;
	const uint8_t *playing_ptr = _inst_playing.ptr();
	const float *volumes_linear_ptr = _inst_volumes_linear.ptr();
	const float *play_pitches_ptr = _inst_play_pitches.ptr();
	const float *source_ptr = _source_samples.ptr();
	const double *positions_ptr = _inst_positions.ptr();
	float base_vol_linear = _db_to_linear(_volume_db);
	int active_size = _active.size();
	float rate_ratio = (float)_source_sample_rate / (float)_output_mix_rate;
	double source_count_d = (double)_source_sample_count;

	// --- Pre-compute spatial data for all playing, in-range instances ---
	_voices.clear();

	for (int i = 0; i < inst_count; i++) {
		if (active_ptr && i < active_size && !active_ptr[i]) continue;
		if (!playing_ptr[i]) continue;
		if (positions_ptr[i] >= source_count_d && !_loop) continue;

		Vector3 inst_world_pos = global_xform.xform(
				compute_instance_transform(transforms[i]).origin);
		float distance = listener_pos.distance_to(inst_world_pos);
		if (distance > _max_distance) continue;

		float attenuation = _reference_distance / MAX(distance, _reference_distance);
		float inst_vol = base_vol_linear * attenuation * volumes_linear_ptr[i];

		Vector3 dir = (inst_world_pos - listener_pos);
		float pan = 0.0f;
		if (dir.length_squared() > 0.001f) {
			pan = dir.normalized().dot(listener_right);
		}

		VoiceData vd;
		vd.index = i;
		vd.vol_left = inst_vol * (0.5f - pan * 0.5f);
		vd.vol_right = inst_vol * (0.5f + pan * 0.5f);
		vd.pitch_advance = play_pitches_ptr[i] * rate_ratio;

		// Distance low-pass filter coefficient: 0 at reference, ~0.95*strength at max.
		if (_distance_filter_enabled && _distance_filter_strength > 0.0f) {
			float range = MAX(_max_distance - _reference_distance, 0.001f);
			float dist_ratio = CLAMP((distance - _reference_distance) / range, 0.0f, 1.0f);
			vd.lp_coeff = dist_ratio * _distance_filter_strength * 0.95f;
		} else {
			vd.lp_coeff = 0.0f;
		}

		_voices.push_back(vd);
	}

	// --- Auto-pause: skip mix entirely if no voices in range ---
	if (_auto_pause && _voices.size() == 0) {
		if (_silence_buffer.size() < frames_available) {
			_silence_buffer.resize(frames_available);
			_silence_buffer.fill(Vector2(0, 0));
		}
		_gen_playback->push_buffer(_silence_buffer);
		return;
	}

	// --- Zero output buffer ---
	if (_mix_buffer.size() != frames_available) {
		_mix_buffer.resize(frames_available);
	}
	Vector2 *buf = _mix_buffer.ptrw();
	for (int f = 0; f < frames_available; f++) {
		buf[f] = Vector2(0.0f, 0.0f);
	}

	double *positions_w = _inst_positions.ptrw();
	float *lp_state_w = _inst_lp_state.ptrw();

	int voice_count = _voices.size();
	const VoiceData *voice_ptr = _voices.ptr();
	const int CROSSFADE_LEN = 64;

	// --- Voice-major mix loop — each voice reads sequentially through source ---
	for (int v = 0; v < voice_count; v++) {
		const VoiceData &vd = voice_ptr[v];
		double pos = positions_w[vd.index];
		float vl = vd.vol_left;
		float vr = vd.vol_right;
		float advance = vd.pitch_advance;
		float lp_coeff = vd.lp_coeff;
		float lp_prev = lp_state_w[vd.index];

		for (int f = 0; f < frames_available; f++) {
			if (pos >= source_count_d) {
				if (_loop) {
					pos = Math::fmod(pos, source_count_d);
				} else {
					break; // Voice finished — skip remaining frames.
				}
			}

			int idx0 = (int)pos;
			float frac = (float)(pos - (double)idx0);
			float s0 = source_ptr[idx0 % _source_sample_count];
			float s1 = source_ptr[(idx0 + 1) % _source_sample_count];
			float sample = s0 + (s1 - s0) * frac;

			// Crossfade at loop boundary to avoid click.
			if (_loop && idx0 >= _source_sample_count - CROSSFADE_LEN) {
				int into_fade = idx0 - (_source_sample_count - CROSSFADE_LEN);
				float t = (float)into_fade / (float)CROSSFADE_LEN;
				int wrap_idx = into_fade;
				float ws0 = source_ptr[wrap_idx % _source_sample_count];
				float ws1 = source_ptr[(wrap_idx + 1) % _source_sample_count];
				float wrap_sample = ws0 + (ws1 - ws0) * frac;
				sample = sample * (1.0f - t) + wrap_sample * t;
			}

			// Single-pole low-pass filter — muffles distant sources.
			lp_prev = lp_prev + (1.0f - lp_coeff) * (sample - lp_prev);
			sample = lp_prev;

			buf[f].x += sample * vl;
			buf[f].y += sample * vr;

			pos += (double)advance;
		}

		positions_w[vd.index] = pos;
		lp_state_w[vd.index] = lp_prev;
	}

	// --- Final pass: amplify + tanh soft-clip ---
	float amplify_linear = _db_to_linear(_amplify_db);
	for (int f = 0; f < frames_available; f++) {
		buf[f].x = tanhf(buf[f].x * amplify_linear);
		buf[f].y = tanhf(buf[f].y * amplify_linear);
	}

	_gen_playback->push_buffer(_mix_buffer);

	// --- Mark finished non-looping instances ---
	if (!_loop) {
		for (int v = 0; v < voice_count; v++) {
			if (positions_w[voice_ptr[v].index] >= source_count_d) {
				_inst_playing.set(voice_ptr[v].index, 0);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// State management
// ---------------------------------------------------------------------------

void MultiNodeAudio::_resize_state(int p_count) {
	int old_count = _inst_playing.size();
	_inst_playing.resize(p_count);
	_inst_volumes.resize(p_count);
	_inst_volumes_linear.resize(p_count);
	_inst_pitches.resize(p_count);
	_inst_play_pitches.resize(p_count);
	_inst_positions.resize(p_count);
	_inst_lp_state.resize(p_count);

	for (int i = old_count; i < p_count; i++) {
		_inst_playing.set(i, 0);
		_inst_volumes.set(i, 0.0f);         // 0 dB
		_inst_volumes_linear.set(i, 1.0f);  // linear 1.0 = 0 dB
		_inst_pitches.set(i, 1.0f);
		_inst_play_pitches.set(i, 1.0f);
		_inst_positions.set(i, 0.0);
		_inst_lp_state.set(i, 0.0f);
	}
}

// ---------------------------------------------------------------------------
// Randomization helper — applied at play time
// ---------------------------------------------------------------------------

void MultiNodeAudio::_apply_randomization(int p_index) {
	if (p_index < 0 || p_index >= _inst_pitches.size()) return;

	if (_pitch_randomization > 0.0f) {
		float rand_offset = (UtilityFunctions::randf() * 2.0f - 1.0f) * _pitch_randomization;
		_inst_play_pitches.set(p_index, _inst_pitches[p_index] * (1.0f + rand_offset));
	} else {
		_inst_play_pitches.set(p_index, _inst_pitches[p_index]);
	}

	if (_volume_randomization > 0.0f) {
		float rand_db = (UtilityFunctions::randf() * 2.0f - 1.0f) * _volume_randomization;
		_inst_volumes_linear.set(p_index, _db_to_linear(_inst_volumes[p_index] + rand_db));
	} else {
		_inst_volumes_linear.set(p_index, _db_to_linear(_inst_volumes[p_index]));
	}
}

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------

float MultiNodeAudio::_db_to_linear(float p_db) {
	return Math::pow(10.0f, p_db * 0.05f);
}
