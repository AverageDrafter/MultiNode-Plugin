#include "multi_node_light.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>

using namespace godot;

MultiNodeLight::MultiNodeLight() {}
MultiNodeLight::~MultiNodeLight() { _clear_lights(); }

void MultiNodeLight::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_light_type", "type"), &MultiNodeLight::set_light_type);
	ClassDB::bind_method(D_METHOD("get_light_type"), &MultiNodeLight::get_light_type);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "light_type", PROPERTY_HINT_ENUM, "Omni,Spot"), "set_light_type", "get_light_type");

	ClassDB::bind_method(D_METHOD("set_light_color", "color"), &MultiNodeLight::set_light_color);
	ClassDB::bind_method(D_METHOD("get_light_color"), &MultiNodeLight::get_light_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "light_color"), "set_light_color", "get_light_color");

	ClassDB::bind_method(D_METHOD("set_light_energy", "energy"), &MultiNodeLight::set_light_energy);
	ClassDB::bind_method(D_METHOD("get_light_energy"), &MultiNodeLight::get_light_energy);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "light_energy", PROPERTY_HINT_RANGE, "0.0,64.0,0.01"), "set_light_energy", "get_light_energy");

	ClassDB::bind_method(D_METHOD("set_light_range", "range"), &MultiNodeLight::set_light_range);
	ClassDB::bind_method(D_METHOD("get_light_range"), &MultiNodeLight::get_light_range);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "light_range", PROPERTY_HINT_RANGE, "0.1,1000.0,0.1,suffix:m"), "set_light_range", "get_light_range");

	ClassDB::bind_method(D_METHOD("set_spot_angle", "angle"), &MultiNodeLight::set_spot_angle);
	ClassDB::bind_method(D_METHOD("get_spot_angle"), &MultiNodeLight::get_spot_angle);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spot_angle", PROPERTY_HINT_RANGE, "1.0,179.0,0.1,degrees"), "set_spot_angle", "get_spot_angle");

	ClassDB::bind_method(D_METHOD("set_shadow_enabled", "enable"), &MultiNodeLight::set_shadow_enabled);
	ClassDB::bind_method(D_METHOD("get_shadow_enabled"), &MultiNodeLight::get_shadow_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "shadow_enabled"), "set_shadow_enabled", "get_shadow_enabled");

	ClassDB::bind_method(D_METHOD("set_instance_energy", "index", "energy"), &MultiNodeLight::set_instance_energy);

	BIND_ENUM_CONSTANT(LIGHT_OMNI);
	BIND_ENUM_CONSTANT(LIGHT_SPOT);
}

void MultiNodeLight::_on_sub_entered() { _rebuild(); }
void MultiNodeLight::_on_sub_exiting() { _clear_lights(); }
void MultiNodeLight::_on_parent_sync() { _sync_lights(); }
void MultiNodeLight::_on_active_changed() { if (is_inside_tree()) _rebuild(); }
RID MultiNodeLight::_get_visual_instance_rid() const { return RID(); }

// Properties
void MultiNodeLight::set_light_type(int p_type) { _light_type = p_type; if (is_inside_tree()) _rebuild(); }
int MultiNodeLight::get_light_type() const { return _light_type; }

void MultiNodeLight::set_light_color(const Color &p_color) {
	_light_color = p_color;
	RenderingServer *rs = RenderingServer::get_singleton();
	for (int i = 0; i < _lights.size(); i++) {
		if (_lights[i].light.is_valid()) rs->light_set_color(_lights[i].light, _light_color);
	}
}
Color MultiNodeLight::get_light_color() const { return _light_color; }

void MultiNodeLight::set_light_energy(float p_energy) {
	_light_energy = p_energy;
	RenderingServer *rs = RenderingServer::get_singleton();
	for (int i = 0; i < _lights.size(); i++) {
		if (_lights[i].light.is_valid()) rs->light_set_param(_lights[i].light, RenderingServer::LIGHT_PARAM_ENERGY, _light_energy);
	}
}
float MultiNodeLight::get_light_energy() const { return _light_energy; }

void MultiNodeLight::set_light_range(float p_range) {
	_light_range = p_range;
	RenderingServer *rs = RenderingServer::get_singleton();
	for (int i = 0; i < _lights.size(); i++) {
		if (_lights[i].light.is_valid()) rs->light_set_param(_lights[i].light, RenderingServer::LIGHT_PARAM_RANGE, _light_range);
	}
}
float MultiNodeLight::get_light_range() const { return _light_range; }

void MultiNodeLight::set_spot_angle(float p_angle) {
	_spot_angle = p_angle;
	RenderingServer *rs = RenderingServer::get_singleton();
	for (int i = 0; i < _lights.size(); i++) {
		if (_lights[i].light.is_valid()) rs->light_set_param(_lights[i].light, RenderingServer::LIGHT_PARAM_SPOT_ANGLE, _spot_angle);
	}
}
float MultiNodeLight::get_spot_angle() const { return _spot_angle; }

void MultiNodeLight::set_shadow_enabled(bool p_enable) {
	_shadow_enabled = p_enable;
	RenderingServer *rs = RenderingServer::get_singleton();
	for (int i = 0; i < _lights.size(); i++) {
		if (_lights[i].light.is_valid()) rs->light_set_shadow(_lights[i].light, _shadow_enabled);
	}
}
bool MultiNodeLight::get_shadow_enabled() const { return _shadow_enabled; }

// Per-instance (color override comes from MultiNodeVisual3D base)
void MultiNodeLight::_on_instance_color_changed(int p_index, const Color &p_color) {
	if (p_index >= 0 && p_index < _lights.size() && _lights[p_index].light.is_valid()) {
		RenderingServer::get_singleton()->light_set_color(_lights[p_index].light, p_color);
	}
}

void MultiNodeLight::set_instance_energy(int p_index, float p_energy) {
	if (p_index >= 0 && p_index < _lights.size() && _lights[p_index].light.is_valid()) {
		RenderingServer::get_singleton()->light_set_param(_lights[p_index].light, RenderingServer::LIGHT_PARAM_ENERGY, p_energy);
	}
}

// ---------------------------------------------------------------------------
// Light management
// ---------------------------------------------------------------------------

void MultiNodeLight::_configure_light(RID p_light) {
	RenderingServer *rs = RenderingServer::get_singleton();
	rs->light_set_color(p_light, _light_color);
	rs->light_set_param(p_light, RenderingServer::LIGHT_PARAM_ENERGY, _light_energy);
	rs->light_set_param(p_light, RenderingServer::LIGHT_PARAM_RANGE, _light_range);
	rs->light_set_shadow(p_light, _shadow_enabled);
	if (_light_type == LIGHT_SPOT) {
		rs->light_set_param(p_light, RenderingServer::LIGHT_PARAM_SPOT_ANGLE, _spot_angle);
	}
}

void MultiNodeLight::_clear_lights() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs) { _lights.clear(); return; }
	for (int i = 0; i < _lights.size(); i++) {
		if (_lights[i].instance.is_valid()) rs->free_rid(_lights[i].instance);
		if (_lights[i].light.is_valid()) rs->free_rid(_lights[i].light);
	}
	_lights.clear();
}

void MultiNodeLight::_rebuild() {
	_clear_lights();
	if (!_parent || !is_inside_tree()) return;
	_sync_lights();
}

void MultiNodeLight::_sync_lights() {
	if (!_parent || !is_inside_tree()) return;

	RenderingServer *rs = RenderingServer::get_singleton();
	int count = _parent->get_instance_count();
	int old_count = _lights.size();

	for (int i = count; i < old_count; i++) {
		if (_lights[i].instance.is_valid()) rs->free_rid(_lights[i].instance);
		if (_lights[i].light.is_valid()) rs->free_rid(_lights[i].light);
	}
	_lights.resize(count);

	Ref<World3D> world = get_world_3d();
	RID scenario = world.is_valid() ? world->get_scenario() : RID();
	Transform3D global_xform = _parent->get_cached_global_transform();
	const Vector<Transform3D> &transforms = _parent->get_transforms_internal();
	const PackedByteArray &dirty = _parent->get_dirty_flags_internal();
	const uint8_t *active_ptr = _active.size() > 0 ? _active.ptr() : nullptr;

	for (int i = 0; i < count; i++) {
		bool is_active = !active_ptr || (i < _active.size() && active_ptr[i]);

		if (i >= old_count) {
			LightInstance li;
			if (_light_type == LIGHT_SPOT) {
				li.light = rs->spot_light_create();
			} else {
				li.light = rs->omni_light_create();
			}
			_configure_light(li.light);
			// Apply per-instance tint (base color * tint).
			rs->light_set_color(li.light, get_tinted_color(i));

			li.instance = rs->instance_create();
			rs->instance_set_base(li.instance, li.light);
			if (scenario.is_valid()) rs->instance_set_scenario(li.instance, scenario);
			rs->instance_set_layer_mask(li.instance, get_render_layers());

			Transform3D xform = global_xform * compute_instance_transform(transforms[i]);
			rs->instance_set_transform(li.instance, xform);
			rs->instance_set_visible(li.instance, is_active);

			_lights.write[i] = li;
		} else if (dirty[i] && _lights[i].instance.is_valid()) {
			Transform3D xform = global_xform * compute_instance_transform(transforms[i]);
			rs->instance_set_transform(_lights[i].instance, xform);
			rs->instance_set_visible(_lights[i].instance, is_active);
		} else if (!dirty[i] && _any_active_changed && did_instance_active_change(i) && _lights[i].instance.is_valid()) {
			// Active flag changed on non-dirty instance — update visibility and transform.
			rs->instance_set_visible(_lights[i].instance, is_active);
			if (is_active) {
				Transform3D xform = global_xform * compute_instance_transform(transforms[i]);
				rs->instance_set_transform(_lights[i].instance, xform);
			}
		}
	}
}
