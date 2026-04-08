#include "multi_node_sub.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/rd_shader_source.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

MultiNodeSub::MultiNodeSub() {}
MultiNodeSub::~MultiNodeSub() { _teardown_compute(); }

void MultiNodeSub::_bind_methods() {
	// --- Active count (read-only inspector display) ---
	ClassDB::bind_method(D_METHOD("get_active_count"), &MultiNodeSub::get_active_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "active_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_active_count");

	// --- Layer 1: instance_every ---
	ClassDB::bind_method(D_METHOD("set_instance_every", "every"), &MultiNodeSub::set_instance_every);
	ClassDB::bind_method(D_METHOD("get_instance_every"), &MultiNodeSub::get_instance_every);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "instance_every", PROPERTY_HINT_RANGE, "-1000000,1000000,1"), "set_instance_every", "get_instance_every");

	// --- Layer 2: range string ---
	ClassDB::bind_method(D_METHOD("set_instance_range", "range"), &MultiNodeSub::set_instance_range);
	ClassDB::bind_method(D_METHOD("get_instance_range"), &MultiNodeSub::get_instance_range);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "instance_range", PROPERTY_HINT_PLACEHOLDER_TEXT, "e.g. 0:99,-50:59,150"), "set_instance_range", "get_instance_range");

	// --- Direct active array access ---
	ClassDB::bind_method(D_METHOD("set_instance_active", "index", "active"), &MultiNodeSub::set_instance_active);
	ClassDB::bind_method(D_METHOD("is_instance_active", "index"), &MultiNodeSub::is_instance_active);
	ClassDB::bind_method(D_METHOD("set_all_active", "active"), &MultiNodeSub::set_all_active);
	ClassDB::bind_method(D_METHOD("get_multi_node_parent"), &MultiNodeSub::get_multi_node_parent);

	// --- Per-instance data (stride controlled by parent MultiNode's sub_data_stride) ---
	ClassDB::bind_method(D_METHOD("get_data_stride"), &MultiNodeSub::get_data_stride);

	ClassDB::bind_method(D_METHOD("set_instance_data", "index", "channel", "value"), &MultiNodeSub::set_instance_data);
	ClassDB::bind_method(D_METHOD("get_instance_data", "index", "channel"), &MultiNodeSub::get_instance_data);
	ClassDB::bind_method(D_METHOD("set_instance_data_vec", "index", "values"), &MultiNodeSub::set_instance_data_vec);
	ClassDB::bind_method(D_METHOD("get_instance_data_vec", "index"), &MultiNodeSub::get_instance_data_vec);

	// --- Compute shader ---
	ClassDB::bind_method(D_METHOD("set_compute_code", "code"), &MultiNodeSub::set_compute_code);
	ClassDB::bind_method(D_METHOD("get_compute_code"), &MultiNodeSub::get_compute_code);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "compute_code", PROPERTY_HINT_MULTILINE_TEXT), "set_compute_code", "get_compute_code");

	ClassDB::bind_method(D_METHOD("set_compute_enabled", "enabled"), &MultiNodeSub::set_compute_enabled);
	ClassDB::bind_method(D_METHOD("get_compute_enabled"), &MultiNodeSub::get_compute_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "compute_enabled"), "set_compute_enabled", "get_compute_enabled");

	// Internal signal handler.
	ClassDB::bind_method(D_METHOD("_on_instances_changed"), &MultiNodeSub::_on_instances_changed);
}

// ---------------------------------------------------------------------------
// Notification
// ---------------------------------------------------------------------------

void MultiNodeSub::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_setup_parent();
			_on_sub_entered();
			if (_compute_enabled && !_compute_code.is_empty()) {
				_setup_compute();
				set_process(true);
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			_on_sub_exiting();
			_teardown_compute();
			_cleanup();
		} break;

		case NOTIFICATION_PARENTED: {
			_setup_parent();
		} break;

		case NOTIFICATION_PROCESS: {
			if (_compute_ready && _compute_enabled && _parent) {
				_dispatch_compute(get_process_delta_time());
			}
		} break;
	}
}

// ---------------------------------------------------------------------------
// Virtual callbacks — defaults
// ---------------------------------------------------------------------------

void MultiNodeSub::_on_sub_entered() {}
void MultiNodeSub::_on_sub_exiting() {}
void MultiNodeSub::_on_parent_sync() {}
void MultiNodeSub::_on_active_changed() {}

PackedStringArray MultiNodeSub::_get_configuration_warnings() const {
	PackedStringArray warnings;
	Node *p = get_parent();
	if (p && !Object::cast_to<MultiNode>(p)) {
		warnings.push_back("MultiNodeSub must be a direct child of a MultiNode. It will not function under a different parent.");
	}
	if (!p) {
		warnings.push_back("MultiNodeSub requires a MultiNode parent.");
	}
	return warnings;
}

// ---------------------------------------------------------------------------
// Parent discovery and signal wiring
// ---------------------------------------------------------------------------

void MultiNodeSub::_setup_parent() {
	if (_parent) {
		if (_parent->is_connected("instances_changed", Callable(this, "_on_instances_changed"))) {
			_parent->disconnect("instances_changed", Callable(this, "_on_instances_changed"));
		}
		_parent = nullptr;
	}
	_parent = Object::cast_to<MultiNode>(get_parent());
	update_configuration_warnings();
	if (!_parent) {
		return;
	}
	_cache_stride();
	if (!_parent->is_connected("instances_changed", Callable(this, "_on_instances_changed"))) {
		_parent->connect("instances_changed", Callable(this, "_on_instances_changed"));
	}
}

void MultiNodeSub::_cleanup() {
	if (_parent) {
		if (_parent->is_connected("instances_changed", Callable(this, "_on_instances_changed"))) {
			_parent->disconnect("instances_changed", Callable(this, "_on_instances_changed"));
		}
		_parent = nullptr;
	}
	_active.resize(0);
	_active_count = 0;
	_range_flags.resize(0);
	_instance_data.resize(0);
}

// ---------------------------------------------------------------------------
// Signal handler
// ---------------------------------------------------------------------------

void MultiNodeSub::_on_instances_changed() {
	if (!_parent || !is_inside_tree()) {
		return;
	}

	int count = _parent->get_instance_count();
	bool count_changed = (count != _active.size());

	if (count_changed) {
		_active.resize(count);
		_parse_range_string();
		_recompute_active();
		_cache_stride();
		_resize_instance_data(count);
	}

	_on_parent_sync();
	commit_active_state();
}

// ---------------------------------------------------------------------------
// Layer 1: instance_every
// ---------------------------------------------------------------------------

void MultiNodeSub::set_instance_every(int32_t p_every) {
	_instance_every = p_every;
	_recompute_active();
	_on_active_changed();
}

int32_t MultiNodeSub::get_instance_every() const {
	return _instance_every;
}

// ---------------------------------------------------------------------------
// Layer 2: range string — Excel-style parser
// ---------------------------------------------------------------------------

void MultiNodeSub::set_instance_range(const String &p_range) {
	_range_string = p_range;
	_parse_range_string();
	_recompute_active();
	_on_active_changed();
}

String MultiNodeSub::get_instance_range() const {
	return _range_string;
}

void MultiNodeSub::_parse_range_string() {
	int count = _active.size();
	_range_flags.resize(count);

	if (_range_string.is_empty()) {
		// Empty string = all in range, no restriction.
		_range_flags.fill(1);
		return;
	}

	// Start with nothing included. Tokens build up the set left-to-right.
	// Positive tokens add, negative tokens (-) subtract.
	// But if the very first token is negative, start with all included
	// so you can write "-5" to mean "everything except 5".
	bool first_is_exclude = _range_string.strip_edges().begins_with("-");
	_range_flags.fill(first_is_exclude ? 1 : 0);

	PackedStringArray tokens = _range_string.split(",", false);
	for (int t = 0; t < tokens.size(); t++) {
		String token = tokens[t].strip_edges();
		if (token.is_empty()) continue;

		bool exclude = token.begins_with("-");
		if (exclude) {
			token = token.substr(1);
		}

		int colon = token.find(":");
		if (colon >= 0) {
			// Range: "N:M"
			String left = token.substr(0, colon).strip_edges();
			String right = token.substr(colon + 1).strip_edges();
			int start = left.to_int();
			int end = right.to_int();
			if (start > end) {
				int tmp = start; start = end; end = tmp;
			}
			start = MAX(0, start);
			end = MIN(end, count - 1);
			for (int i = start; i <= end; i++) {
				_range_flags.set(i, exclude ? 0 : 1);
			}
		} else {
			// Single index: "N"
			int idx = token.to_int();
			if (idx >= 0 && idx < count) {
				_range_flags.set(idx, exclude ? 0 : 1);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Recompute active — ANDs both layers
// ---------------------------------------------------------------------------

void MultiNodeSub::_recompute_active() {
	int count = _active.size();
	if (count == 0) {
		_active_count = 0;
		return;
	}

	int every = _instance_every;
	int new_active_count = 0;

	for (int i = 0; i < count; i++) {
		// Layer 1: instance_every
		bool every_ok;
		if (every == 0 || every == -1) {
			every_ok = false;
		} else if (every == 1) {
			every_ok = true;
		} else if (every > 0) {
			every_ok = (i % every == 0);
		} else {
			int step = -every;
			every_ok = (step > count) ? true : (i % step != 0);
		}

		// Layer 2: range flags
		bool range_ok = (i < _range_flags.size()) ? (_range_flags[i] == 1) : true;

		bool is_active = every_ok && range_ok;
		_active.set(i, is_active ? 1 : 0);
		new_active_count += is_active ? 1 : 0;
	}

	_active_count = new_active_count;
	_any_active_changed = true;
}

// ---------------------------------------------------------------------------
// Active array — direct access for runtime code
// ---------------------------------------------------------------------------

void MultiNodeSub::set_instance_active(int p_index, bool p_active) {
	if (p_index < 0 || p_index >= _active.size()) return;
	uint8_t val = p_active ? 1 : 0;
	if (_active[p_index] != val) {
		_active.set(p_index, val);
		_active_count += p_active ? 1 : -1;
		_any_active_changed = true;
		// Tell parent to emit instances_changed at end of frame so all
		// sub-nodes can process the active↔space transitions.
		if (_parent) {
			_parent->request_sync();
		}
	}
}

bool MultiNodeSub::is_instance_active(int p_index) const {
	if (p_index < 0 || p_index >= _active.size()) return false;
	return _active[p_index] == 1;
}

void MultiNodeSub::set_all_active(bool p_active) {
	_active.fill(p_active ? 1 : 0);
	_active_count = p_active ? _active.size() : 0;
	_any_active_changed = true;
}

bool MultiNodeSub::has_active_state_changes() const {
	return _any_active_changed;
}

bool MultiNodeSub::did_instance_active_change(int p_index) const {
	if (p_index < 0 || p_index >= _active.size()) return false;
	if (p_index >= _prev_active.size()) return true;
	return _active[p_index] != _prev_active[p_index];
}

void MultiNodeSub::commit_active_state() {
	_prev_active = _active;
	_any_active_changed = false;
}

int MultiNodeSub::get_active_count() const {
	return _active_count;
}

MultiNode *MultiNodeSub::get_multi_node_parent() const {
	return _parent;
}

// ---------------------------------------------------------------------------
// Per-instance data array
// ---------------------------------------------------------------------------

int MultiNodeSub::get_data_stride() const {
	return _cached_stride;
}

void MultiNodeSub::_cache_stride() {
	_cached_stride = _parent ? _parent->get_sub_data_stride() : 4;
}

void MultiNodeSub::_resize_instance_data(int p_count) {
	int new_size = p_count * _cached_stride;
	int old_size = _instance_data.size();
	if (new_size == old_size) return;
	_instance_data.resize(new_size);
	for (int i = old_size; i < new_size; i++) {
		_instance_data.set(i, 0.0f);
	}
}

void MultiNodeSub::set_instance_data(int p_index, int p_channel, float p_value) {
	int offset = p_index * _cached_stride + p_channel;
	if (offset >= 0 && offset < _instance_data.size() && p_channel >= 0 && p_channel < _cached_stride) {
		_instance_data.set(offset, p_value);
	}
}

float MultiNodeSub::get_instance_data(int p_index, int p_channel) const {
	int offset = p_index * _cached_stride + p_channel;
	if (offset >= 0 && offset < _instance_data.size() && p_channel >= 0 && p_channel < _cached_stride) {
		return _instance_data[offset];
	}
	return 0.0f;
}

void MultiNodeSub::set_instance_data_vec(int p_index, const PackedFloat32Array &p_values) {
	int base = p_index * _cached_stride;
	int to_copy = MIN(p_values.size(), _cached_stride);
	if (base < 0 || base + to_copy > _instance_data.size()) return;
	for (int i = 0; i < to_copy; i++) {
		_instance_data.set(base + i, p_values[i]);
	}
}

PackedFloat32Array MultiNodeSub::get_instance_data_vec(int p_index) const {
	PackedFloat32Array result;
	int base = p_index * _cached_stride;
	if (base < 0 || base + _cached_stride > _instance_data.size()) return result;
	result.resize(_cached_stride);
	for (int i = 0; i < _cached_stride; i++) {
		result.set(i, _instance_data[base + i]);
	}
	return result;
}

// ---------------------------------------------------------------------------
// Compute shader properties
// ---------------------------------------------------------------------------

void MultiNodeSub::set_compute_code(const String &p_code) {
	_compute_code = p_code;
	if (_compute_ready) {
		_teardown_compute();
	}
	if (_compute_enabled && !_compute_code.is_empty() && is_inside_tree()) {
		_setup_compute();
	}
}

String MultiNodeSub::get_compute_code() const {
	return _compute_code;
}

void MultiNodeSub::set_compute_enabled(bool p_enabled) {
	_compute_enabled = p_enabled;
	if (_compute_enabled && !_compute_code.is_empty() && is_inside_tree()) {
		if (!_compute_ready) {
			_setup_compute();
		}
		set_process(true);
	} else if (!_compute_enabled) {
		_teardown_compute();
	}
}

bool MultiNodeSub::get_compute_enabled() const {
	return _compute_enabled;
}

// ---------------------------------------------------------------------------
// Compute shader — setup
// ---------------------------------------------------------------------------

void MultiNodeSub::_setup_compute() {
	if (_rd) {
		_teardown_compute();
	}
	if (_compute_code.is_empty()) return;

	_rd = RenderingServer::get_singleton()->create_local_rendering_device();
	if (!_rd) {
		WARN_PRINT("MultiNodeSub: Failed to create local RenderingDevice for compute.");
		return;
	}

	Ref<RDShaderSource> source;
	source.instantiate();
	source->set_stage_source(RenderingDevice::SHADER_STAGE_COMPUTE, _compute_code);

	Ref<RDShaderSPIRV> spirv = _rd->shader_compile_spirv_from_source(source);
	if (!spirv.is_valid()) {
		WARN_PRINT("MultiNodeSub: Compute shader compilation returned null.");
		memdelete(_rd);
		_rd = nullptr;
		return;
	}

	String err = spirv->get_stage_compile_error(RenderingDevice::SHADER_STAGE_COMPUTE);
	if (!err.is_empty()) {
		WARN_PRINT(String("MultiNodeSub: Compute shader compile error: ") + err);
		memdelete(_rd);
		_rd = nullptr;
		return;
	}

	_compute_shader = _rd->shader_create_from_spirv(spirv);
	if (!_compute_shader.is_valid()) {
		WARN_PRINT("MultiNodeSub: Failed to create compute shader from SPIR-V.");
		memdelete(_rd);
		_rd = nullptr;
		return;
	}

	_compute_pipeline = _rd->compute_pipeline_create(_compute_shader);
	if (!_compute_pipeline.is_valid()) {
		WARN_PRINT("MultiNodeSub: Failed to create compute pipeline.");
		_rd->free_rid(_compute_shader);
		memdelete(_rd);
		_rd = nullptr;
		return;
	}

	_compute_count = 0;
	_compute_ready = true;
#ifdef DEBUG_ENABLED
	UtilityFunctions::print("MultiNodeSub: Compute shader compiled and ready.");
#endif
}

// ---------------------------------------------------------------------------
// Compute shader — teardown
// ---------------------------------------------------------------------------

void MultiNodeSub::_teardown_compute() {
	if (!_rd) return;

	if (_uniform_set.is_valid()) { _rd->free_rid(_uniform_set); _uniform_set = RID(); }
	if (_transform_buffer.is_valid()) { _rd->free_rid(_transform_buffer); _transform_buffer = RID(); }
	if (_active_buffer.is_valid()) { _rd->free_rid(_active_buffer); _active_buffer = RID(); }
	if (_data_buffer.is_valid()) { _rd->free_rid(_data_buffer); _data_buffer = RID(); }
	if (_compute_pipeline.is_valid()) { _rd->free_rid(_compute_pipeline); _compute_pipeline = RID(); }
	if (_compute_shader.is_valid()) { _rd->free_rid(_compute_shader); _compute_shader = RID(); }

	memdelete(_rd);
	_rd = nullptr;
	_compute_ready = false;
	_compute_count = 0;
}

// ---------------------------------------------------------------------------
// Compute shader — buffer resize
// ---------------------------------------------------------------------------

void MultiNodeSub::_resize_compute_buffers(int p_count) {
	if (!_rd || p_count == _compute_count) return;

	if (_uniform_set.is_valid()) { _rd->free_rid(_uniform_set); _uniform_set = RID(); }
	if (_transform_buffer.is_valid()) { _rd->free_rid(_transform_buffer); _transform_buffer = RID(); }
	if (_active_buffer.is_valid()) { _rd->free_rid(_active_buffer); _active_buffer = RID(); }
	if (_data_buffer.is_valid()) { _rd->free_rid(_data_buffer); _data_buffer = RID(); }

	if (p_count == 0) {
		_compute_count = 0;
		return;
	}

	uint32_t transform_size = p_count * 12 * sizeof(float);
	PackedByteArray transform_init;
	transform_init.resize(transform_size);
	transform_init.fill(0);
	_transform_buffer = _rd->storage_buffer_create(transform_size, transform_init);

	uint32_t active_size = p_count * sizeof(uint32_t);
	PackedByteArray active_init;
	active_init.resize(active_size);
	active_init.fill(0);
	_active_buffer = _rd->storage_buffer_create(active_size, active_init);

	int gpu_stride = MAX(get_data_stride(), 4);
	gpu_stride = ((gpu_stride + 3) / 4) * 4;
	uint32_t data_size = p_count * gpu_stride * sizeof(float);
	PackedByteArray data_init;
	data_init.resize(data_size);
	data_init.fill(0);
	_data_buffer = _rd->storage_buffer_create(data_size, data_init);

	Ref<RDUniform> u_transforms;
	u_transforms.instantiate();
	u_transforms->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	u_transforms->set_binding(0);
	u_transforms->add_id(_transform_buffer);

	Ref<RDUniform> u_active;
	u_active.instantiate();
	u_active->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	u_active->set_binding(1);
	u_active->add_id(_active_buffer);

	Ref<RDUniform> u_data;
	u_data.instantiate();
	u_data->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	u_data->set_binding(2);
	u_data->add_id(_data_buffer);

	TypedArray<Ref<RDUniform>> uniforms;
	uniforms.push_back(u_transforms);
	uniforms.push_back(u_active);
	uniforms.push_back(u_data);

	_uniform_set = _rd->uniform_set_create(uniforms, _compute_shader, 0);
	_compute_count = p_count;
}

// ---------------------------------------------------------------------------
// Compute shader — dispatch
// ---------------------------------------------------------------------------

void MultiNodeSub::_dispatch_compute(double p_delta) {
	if (!_compute_ready || !_rd || !_parent) return;

	int count = _parent->get_instance_count();
	if (count == 0) return;

	if (get_active_count() == 0) return;

	if (count != _compute_count) {
		_resize_compute_buffers(count);
		if (!_uniform_set.is_valid()) return;
	}

	count = _compute_count;
	if (count == 0) return;

	const Vector<Transform3D> &transforms = _parent->get_transforms_internal();
	if (transforms.size() < count) return;

	int gpu_stride = MAX(_cached_stride, 4);
	gpu_stride = ((gpu_stride + 3) / 4) * 4;

	uint32_t transform_byte_size = count * 12 * sizeof(float);
	uint32_t active_byte_size = count * sizeof(uint32_t);
	uint32_t data_byte_size = count * gpu_stride * sizeof(float);

	if ((uint32_t)_stage_transforms.size() != transform_byte_size) _stage_transforms.resize(transform_byte_size);
	if ((uint32_t)_stage_active.size() != active_byte_size) _stage_active.resize(active_byte_size);
	if ((uint32_t)_stage_data.size() != data_byte_size) _stage_data.resize(data_byte_size);

	float *tbuf = (float *)_stage_transforms.ptrw();
	for (int i = 0; i < count; i++) {
		const Transform3D &t = transforms[i];
		int off = i * 12;
		tbuf[off + 0] = t.basis[0][0]; tbuf[off + 1] = t.basis[0][1]; tbuf[off + 2] = t.basis[0][2]; tbuf[off + 3] = t.origin.x;
		tbuf[off + 4] = t.basis[1][0]; tbuf[off + 5] = t.basis[1][1]; tbuf[off + 6] = t.basis[1][2]; tbuf[off + 7] = t.origin.y;
		tbuf[off + 8] = t.basis[2][0]; tbuf[off + 9] = t.basis[2][1]; tbuf[off + 10] = t.basis[2][2]; tbuf[off + 11] = t.origin.z;
	}
	_rd->buffer_update(_transform_buffer, 0, transform_byte_size, _stage_transforms);

	uint32_t *abuf = (uint32_t *)_stage_active.ptrw();
	const uint8_t *active_ptr = _active.size() > 0 ? _active.ptr() : nullptr;
	for (int i = 0; i < count; i++) {
		abuf[i] = (active_ptr && i < _active.size() && active_ptr[i]) ? 1 : 0;
	}
	_rd->buffer_update(_active_buffer, 0, active_byte_size, _stage_active);

	float *dbuf = (float *)_stage_data.ptrw();
	for (int i = 0; i < count; i++) {
		int src_base = i * _cached_stride;
		int dst_base = i * gpu_stride;
		for (int c = 0; c < _cached_stride && src_base + c < _instance_data.size(); c++) {
			dbuf[dst_base + c] = _instance_data[src_base + c];
		}
	}
	_rd->buffer_update(_data_buffer, 0, data_byte_size, _stage_data);

	PackedByteArray push_data;
	push_data.resize(16);
	float *push = (float *)push_data.ptrw();
	push[0] = (float)p_delta;
	push[1] = (float)UtilityFunctions::snappedf(
			Engine::get_singleton()->get_process_frames() * p_delta, 0.001);
	uint32_t *push_uint = (uint32_t *)push_data.ptrw();
	push_uint[2] = (uint32_t)count;
	push_uint[3] = (uint32_t)gpu_stride;

	int64_t compute_list = _rd->compute_list_begin();
	_rd->compute_list_bind_compute_pipeline(compute_list, _compute_pipeline);
	_rd->compute_list_bind_uniform_set(compute_list, _uniform_set, 0);
	_rd->compute_list_set_push_constant(compute_list, push_data, 16);
	_rd->compute_list_dispatch(compute_list, (count + 63) / 64, 1, 1);
	_rd->compute_list_end();

	_rd->submit();
	_rd->sync();

	if (_parent->get_use_transforms()) {
		PackedByteArray result = _rd->buffer_get_data(_transform_buffer);
		const float *rbuf = (const float *)result.ptr();

		_parent->begin_batch();
		for (int i = 0; i < count; i++) {
			if (active_ptr && i < _active.size() && !active_ptr[i]) {
				continue;
			}
			int off = i * 12;
			Transform3D t;
			t.basis[0][0] = rbuf[off + 0]; t.basis[0][1] = rbuf[off + 1]; t.basis[0][2] = rbuf[off + 2]; t.origin.x = rbuf[off + 3];
			t.basis[1][0] = rbuf[off + 4]; t.basis[1][1] = rbuf[off + 5]; t.basis[1][2] = rbuf[off + 6]; t.origin.y = rbuf[off + 7];
			t.basis[2][0] = rbuf[off + 8]; t.basis[2][1] = rbuf[off + 9]; t.basis[2][2] = rbuf[off + 10]; t.origin.z = rbuf[off + 11];

			if (t != transforms[i]) {
				_parent->set_instance_transform(i, t);
			}
		}
		_parent->end_batch();
	}

	PackedByteArray data_result = _rd->buffer_get_data(_data_buffer);
	const float *data_rbuf = (const float *)data_result.ptr();
	for (int i = 0; i < count; i++) {
		int src_base = i * gpu_stride;
		int dst_base = i * _cached_stride;
		for (int c = 0; c < _cached_stride && dst_base + c < _instance_data.size(); c++) {
			_instance_data.set(dst_base + c, data_rbuf[src_base + c]);
		}
	}
}
