#include "multi_node_particle.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/rendering_server.hpp>

using namespace godot;

MultiNodeParticle::MultiNodeParticle() {}
MultiNodeParticle::~MultiNodeParticle() {}

void MultiNodeParticle::_bind_methods() {
	ADD_GROUP("Emission", "");

	ClassDB::bind_method(D_METHOD("set_process_material", "material"), &MultiNodeParticle::set_process_material);
	ClassDB::bind_method(D_METHOD("get_process_material"), &MultiNodeParticle::get_process_material);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "process_material", PROPERTY_HINT_RESOURCE_TYPE, "ParticleProcessMaterial,ShaderMaterial"), "set_process_material", "get_process_material");

	ClassDB::bind_method(D_METHOD("set_draw_mesh", "mesh"), &MultiNodeParticle::set_draw_mesh);
	ClassDB::bind_method(D_METHOD("get_draw_mesh"), &MultiNodeParticle::get_draw_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "draw_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_draw_mesh", "get_draw_mesh");

	ClassDB::bind_method(D_METHOD("set_emitting", "emitting"), &MultiNodeParticle::set_emitting);
	ClassDB::bind_method(D_METHOD("get_emitting"), &MultiNodeParticle::get_emitting);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "emitting"), "set_emitting", "get_emitting");

	ClassDB::bind_method(D_METHOD("set_amount", "amount"), &MultiNodeParticle::set_amount);
	ClassDB::bind_method(D_METHOD("get_amount"), &MultiNodeParticle::get_amount);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "amount", PROPERTY_HINT_RANGE, "1,4096,1"), "set_amount", "get_amount");

	ClassDB::bind_method(D_METHOD("set_one_shot", "one_shot"), &MultiNodeParticle::set_one_shot);
	ClassDB::bind_method(D_METHOD("get_one_shot"), &MultiNodeParticle::get_one_shot);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "one_shot"), "set_one_shot", "get_one_shot");

	ClassDB::bind_method(D_METHOD("set_explosiveness", "ratio"), &MultiNodeParticle::set_explosiveness);
	ClassDB::bind_method(D_METHOD("get_explosiveness"), &MultiNodeParticle::get_explosiveness);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "explosiveness", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_explosiveness", "get_explosiveness");

	ADD_GROUP("Timing", "");

	ClassDB::bind_method(D_METHOD("set_lifetime", "lifetime"), &MultiNodeParticle::set_lifetime);
	ClassDB::bind_method(D_METHOD("get_lifetime"), &MultiNodeParticle::get_lifetime);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lifetime", PROPERTY_HINT_RANGE, "0.01,60.0,0.01,suffix:s"), "set_lifetime", "get_lifetime");

	ClassDB::bind_method(D_METHOD("set_speed_scale", "scale"), &MultiNodeParticle::set_speed_scale);
	ClassDB::bind_method(D_METHOD("get_speed_scale"), &MultiNodeParticle::get_speed_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_scale", PROPERTY_HINT_RANGE, "0.0,64.0,0.01"), "set_speed_scale", "get_speed_scale");

	ClassDB::bind_method(D_METHOD("set_randomness", "ratio"), &MultiNodeParticle::set_randomness);
	ClassDB::bind_method(D_METHOD("get_randomness"), &MultiNodeParticle::get_randomness);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "randomness", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_randomness", "get_randomness");

	ClassDB::bind_method(D_METHOD("set_pre_process_time", "time"), &MultiNodeParticle::set_pre_process_time);
	ClassDB::bind_method(D_METHOD("get_pre_process_time"), &MultiNodeParticle::get_pre_process_time);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pre_process_time", PROPERTY_HINT_RANGE, "0.0,60.0,0.01,suffix:s"), "set_pre_process_time", "get_pre_process_time");

	ClassDB::bind_method(D_METHOD("set_fixed_fps", "fps"), &MultiNodeParticle::set_fixed_fps);
	ClassDB::bind_method(D_METHOD("get_fixed_fps"), &MultiNodeParticle::get_fixed_fps);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "fixed_fps", PROPERTY_HINT_RANGE, "0,1000,1"), "set_fixed_fps", "get_fixed_fps");

	// --- Per-instance ---
	ClassDB::bind_method(D_METHOD("set_instance_emitting", "index", "emitting"), &MultiNodeParticle::set_instance_emitting);
	ClassDB::bind_method(D_METHOD("get_instance_emitting", "index"), &MultiNodeParticle::get_instance_emitting);
	ClassDB::bind_method(D_METHOD("restart_instance", "index"), &MultiNodeParticle::restart_instance);
	ClassDB::bind_method(D_METHOD("restart_all"), &MultiNodeParticle::restart_all);
}

void MultiNodeParticle::_on_sub_entered() { _rebuild(); }
void MultiNodeParticle::_on_sub_exiting() { _clear_emitters(); }
void MultiNodeParticle::_on_parent_sync() { _sync_emitters(); }
void MultiNodeParticle::_on_active_changed() { if (is_inside_tree()) _rebuild(); }

RID MultiNodeParticle::_get_visual_instance_rid() const { return RID(); }

// ---------------------------------------------------------------------------
// Core properties
// ---------------------------------------------------------------------------

void MultiNodeParticle::set_amount(int p_amount) {
	_amount = MAX(1, p_amount);
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) _emitters[i]->set_amount(_amount);
	}
}

int MultiNodeParticle::get_amount() const { return _amount; }

void MultiNodeParticle::set_lifetime(float p_lifetime) {
	_lifetime = MAX(0.01f, p_lifetime);
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) _emitters[i]->set_lifetime(_lifetime);
	}
}

float MultiNodeParticle::get_lifetime() const { return _lifetime; }

void MultiNodeParticle::set_emitting(bool p_emitting) {
	_emitting = p_emitting;
	const uint8_t *active_ptr = _active.size() > 0 ? _active.ptr() : nullptr;
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) {
			bool should = _emitting && (!active_ptr || (i < _active.size() && active_ptr[i]))
					&& (i < _inst_emitting.size() && _inst_emitting[i]);
			_emitters[i]->set_emitting(should);
		}
	}
}

bool MultiNodeParticle::get_emitting() const { return _emitting; }

// ---------------------------------------------------------------------------
// Simulation properties — pushed to all emitters on change
// ---------------------------------------------------------------------------

void MultiNodeParticle::set_speed_scale(float p_scale) {
	_speed_scale = p_scale;
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) _emitters[i]->set_speed_scale(_speed_scale);
	}
}
float MultiNodeParticle::get_speed_scale() const { return _speed_scale; }

void MultiNodeParticle::set_explosiveness(float p_ratio) {
	_explosiveness = p_ratio;
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) _emitters[i]->set_explosiveness_ratio(_explosiveness);
	}
}
float MultiNodeParticle::get_explosiveness() const { return _explosiveness; }

void MultiNodeParticle::set_randomness(float p_ratio) {
	_randomness = p_ratio;
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) _emitters[i]->set_randomness_ratio(_randomness);
	}
}
float MultiNodeParticle::get_randomness() const { return _randomness; }

void MultiNodeParticle::set_one_shot(bool p_one_shot) {
	_one_shot = p_one_shot;
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) _emitters[i]->set_one_shot(_one_shot);
	}
}
bool MultiNodeParticle::get_one_shot() const { return _one_shot; }

void MultiNodeParticle::set_pre_process_time(float p_time) {
	_pre_process_time = MAX(0.0f, p_time);
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) _emitters[i]->set_pre_process_time(_pre_process_time);
	}
}
float MultiNodeParticle::get_pre_process_time() const { return _pre_process_time; }

void MultiNodeParticle::set_fixed_fps(int p_fps) {
	_fixed_fps = MAX(0, p_fps);
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) _emitters[i]->set_fixed_fps(_fixed_fps);
	}
}
int MultiNodeParticle::get_fixed_fps() const { return _fixed_fps; }

// ---------------------------------------------------------------------------
// Per-instance
// ---------------------------------------------------------------------------

void MultiNodeParticle::set_instance_emitting(int p_index, bool p_emitting) {
	if (p_index >= 0 && p_index < _inst_emitting.size()) {
		_inst_emitting.set(p_index, p_emitting ? 1 : 0);
		if (p_index < _emitters.size() && _emitters[p_index]) {
			const uint8_t *active_ptr = _active.size() > 0 ? _active.ptr() : nullptr;
			bool should = _emitting && p_emitting
					&& (!active_ptr || (p_index < _active.size() && active_ptr[p_index]));
			_emitters[p_index]->set_emitting(should);
		}
	}
}

bool MultiNodeParticle::get_instance_emitting(int p_index) const {
	return (p_index >= 0 && p_index < _inst_emitting.size()) ? _inst_emitting[p_index] == 1 : false;
}

void MultiNodeParticle::restart_instance(int p_index) {
	if (p_index >= 0 && p_index < _emitters.size() && _emitters[p_index]) {
		_emitters[p_index]->restart();
	}
}

void MultiNodeParticle::restart_all() {
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) _emitters[i]->restart();
	}
}

// ---------------------------------------------------------------------------
// Visual property forwarding — push cast_shadow, render_layers, color
// ---------------------------------------------------------------------------

void MultiNodeParticle::_apply_visual_to_emitter(GPUParticles3D *p_emitter) {
	if (!p_emitter) return;
	RenderingServer *rs = RenderingServer::get_singleton();
	RID instance_rid = p_emitter->get_instance();
	if (instance_rid.is_valid()) {
		rs->instance_geometry_set_cast_shadows_setting(instance_rid,
				(RenderingServer::ShadowCastingSetting)(int)get_cast_shadow());
		rs->instance_set_layer_mask(instance_rid, get_render_layers());
	}
}

void MultiNodeParticle::_on_colors_changed() {
	// Tint changes don't directly apply to GPUParticles3D — would need
	// per-instance material overrides or shader parameter injection.
	// For now, color support is via the process material's color property.
}

void MultiNodeParticle::_on_instance_color_changed(int p_index, const Color &p_color) {
	// Same limitation — GPUParticles3D doesn't have per-instance color tint
	// without custom shader material. Reserved for future implementation.
}

// ---------------------------------------------------------------------------
// Emitter management
// ---------------------------------------------------------------------------

void MultiNodeParticle::_configure_emitter(GPUParticles3D *p_emitter) {
	p_emitter->set_amount(_amount);
	p_emitter->set_lifetime(_lifetime);
	p_emitter->set_one_shot(_one_shot);
	p_emitter->set_use_local_coordinates(false);
	p_emitter->set_speed_scale(_speed_scale);
	p_emitter->set_explosiveness_ratio(_explosiveness);
	p_emitter->set_randomness_ratio(_randomness);
	p_emitter->set_pre_process_time(_pre_process_time);
	if (_fixed_fps > 0) {
		p_emitter->set_fixed_fps(_fixed_fps);
	}
	p_emitter->set_draw_passes(1);
	if (_process_material.is_valid()) {
		p_emitter->set_process_material(_process_material);
	}
	if (_draw_mesh.is_valid()) {
		p_emitter->set_draw_pass_mesh(0, _draw_mesh);
	}
}

void MultiNodeParticle::set_process_material(const Ref<Material> &p_material) {
	_process_material = p_material;
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) _emitters[i]->set_process_material(p_material);
	}
}

Ref<Material> MultiNodeParticle::get_process_material() const { return _process_material; }

void MultiNodeParticle::set_draw_mesh(const Ref<Mesh> &p_mesh) {
	_draw_mesh = p_mesh;
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) {
			_emitters[i]->set_draw_passes(1);
			_emitters[i]->set_draw_pass_mesh(0, p_mesh);
		}
	}
}

Ref<Mesh> MultiNodeParticle::get_draw_mesh() const { return _draw_mesh; }

void MultiNodeParticle::_clear_emitters() {
	for (int i = 0; i < _emitters.size(); i++) {
		if (_emitters[i]) {
			_emitters[i]->set_emitting(false);
			_emitters[i]->queue_free();
		}
	}
	_emitters.clear();
	_inst_emitting.resize(0);
}

void MultiNodeParticle::_rebuild() {
	_clear_emitters();
	if (!_parent || !is_inside_tree()) return;
	_sync_emitters();
}

void MultiNodeParticle::_resize_state(int p_count) {
	int old_count = _inst_emitting.size();
	_inst_emitting.resize(p_count);
	for (int i = old_count; i < p_count; i++) {
		_inst_emitting.set(i, 1);
	}
}

void MultiNodeParticle::_sync_emitters() {
	if (!_parent || !is_inside_tree()) return;

	int count = _parent->get_instance_count();
	int old_count = _emitters.size();

	bool resized = (count != old_count);

	_resize_state(count);

	// Shrink.
	for (int i = count; i < old_count; i++) {
		if (_emitters[i]) {
			_emitters[i]->set_emitting(false);
			_emitters[i]->queue_free();
		}
	}
	_emitters.resize(count);

	// Early-out when nothing changed.
	int dirty_count = _parent->get_dirty_count();
	if (dirty_count == 0 && !resized && !_any_active_changed) {
		return;
	}

	Transform3D global_xform = _parent->get_cached_global_transform();
	const Vector<Transform3D> &transforms = _parent->get_transforms_internal();
	const PackedByteArray &dirty = _parent->get_dirty_flags_internal();
	const uint8_t *active_ptr = _active.size() > 0 ? _active.ptr() : nullptr;

	for (int i = 0; i < count; i++) {
		bool is_active = !active_ptr || (i < _active.size() && active_ptr[i]);

		if (i >= old_count) {
			// Create new emitter.
			GPUParticles3D *emitter = memnew(GPUParticles3D);
			_configure_emitter(emitter);
			add_child(emitter);
			_apply_visual_to_emitter(emitter);

			bool should_emit = _emitting && is_active && _inst_emitting[i];
			emitter->set_emitting(should_emit);

			Transform3D xform = compute_instance_transform(transforms[i]);
			emitter->set_global_transform(global_xform * Transform3D(Basis(), xform.origin));
			emitter->set_visible(is_active);

			_emitters.write[i] = emitter;
		} else if (_emitters[i] && dirty[i]) {
			// Update position for dirty instances.
			Transform3D xform = compute_instance_transform(transforms[i]);
			_emitters[i]->set_global_transform(global_xform * Transform3D(Basis(), xform.origin));
			_emitters[i]->set_visible(is_active);
			bool should_emit = _emitting && is_active && _inst_emitting[i];
			_emitters[i]->set_emitting(should_emit);
		} else if (_emitters[i] && !dirty[i] && _any_active_changed && did_instance_active_change(i)) {
			// Active flag changed on non-dirty instance.
			_emitters[i]->set_visible(is_active);
			bool should_emit = _emitting && is_active && _inst_emitting[i];
			_emitters[i]->set_emitting(should_emit);
			if (is_active) {
				Transform3D xform = compute_instance_transform(transforms[i]);
				_emitters[i]->set_global_transform(global_xform * Transform3D(Basis(), xform.origin));
			}
		}
	}
}
