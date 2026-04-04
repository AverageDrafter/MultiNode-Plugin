#include "multi_node_decal.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>

using namespace godot;

MultiNodeDecal::MultiNodeDecal() {}
MultiNodeDecal::~MultiNodeDecal() { _clear_decals(); }

void MultiNodeDecal::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_texture_albedo", "texture"), &MultiNodeDecal::set_texture_albedo);
	ClassDB::bind_method(D_METHOD("get_texture_albedo"), &MultiNodeDecal::get_texture_albedo);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture_albedo", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture_albedo", "get_texture_albedo");

	ClassDB::bind_method(D_METHOD("set_decal_size", "size"), &MultiNodeDecal::set_decal_size);
	ClassDB::bind_method(D_METHOD("get_decal_size"), &MultiNodeDecal::get_decal_size);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "decal_size"), "set_decal_size", "get_decal_size");

	ClassDB::bind_method(D_METHOD("set_albedo_mix", "mix"), &MultiNodeDecal::set_albedo_mix);
	ClassDB::bind_method(D_METHOD("get_albedo_mix"), &MultiNodeDecal::get_albedo_mix);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "albedo_mix", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_albedo_mix", "get_albedo_mix");

	ClassDB::bind_method(D_METHOD("set_normal_fade", "fade"), &MultiNodeDecal::set_normal_fade);
	ClassDB::bind_method(D_METHOD("get_normal_fade"), &MultiNodeDecal::get_normal_fade);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "normal_fade", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_normal_fade", "get_normal_fade");

	ClassDB::bind_method(D_METHOD("set_upper_fade", "fade"), &MultiNodeDecal::set_upper_fade);
	ClassDB::bind_method(D_METHOD("get_upper_fade"), &MultiNodeDecal::get_upper_fade);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "upper_fade", PROPERTY_HINT_RANGE, "0.0,10.0,0.01"), "set_upper_fade", "get_upper_fade");

	ClassDB::bind_method(D_METHOD("set_lower_fade", "fade"), &MultiNodeDecal::set_lower_fade);
	ClassDB::bind_method(D_METHOD("get_lower_fade"), &MultiNodeDecal::get_lower_fade);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lower_fade", PROPERTY_HINT_RANGE, "0.0,10.0,0.01"), "set_lower_fade", "get_lower_fade");
}

void MultiNodeDecal::_on_sub_entered() { _rebuild(); }
void MultiNodeDecal::_on_sub_exiting() { _clear_decals(); }
void MultiNodeDecal::_on_parent_sync() { _sync_decals(); }
void MultiNodeDecal::_on_active_changed() { if (is_inside_tree()) _rebuild(); }
RID MultiNodeDecal::_get_visual_instance_rid() const { return RID(); }

void MultiNodeDecal::set_texture_albedo(const Ref<Texture2D> &p_texture) {
	_texture_albedo = p_texture;
	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs) {
		for (int i = 0; i < _decals.size(); i++) {
			if (_decals[i].decal.is_valid() && _texture_albedo.is_valid()) {
				rs->decal_set_texture(_decals[i].decal, RenderingServer::DECAL_TEXTURE_ALBEDO, _texture_albedo->get_rid());
			}
		}
	}
}
Ref<Texture2D> MultiNodeDecal::get_texture_albedo() const { return _texture_albedo; }

void MultiNodeDecal::set_decal_size(const Vector3 &p_size) {
	_decal_size = p_size;
	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs) {
		for (int i = 0; i < _decals.size(); i++) {
			if (_decals[i].decal.is_valid()) {
				rs->decal_set_size(_decals[i].decal, _decal_size);
			}
		}
	}
}
Vector3 MultiNodeDecal::get_decal_size() const { return _decal_size; }

void MultiNodeDecal::set_albedo_mix(float p_mix) {
	_albedo_mix = p_mix;
	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs) {
		for (int i = 0; i < _decals.size(); i++) {
			if (_decals[i].decal.is_valid()) {
				rs->decal_set_albedo_mix(_decals[i].decal, _albedo_mix);
			}
		}
	}
}
float MultiNodeDecal::get_albedo_mix() const { return _albedo_mix; }

void MultiNodeDecal::set_normal_fade(float p_fade) {
	_normal_fade = p_fade;
	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs) {
		for (int i = 0; i < _decals.size(); i++) {
			if (_decals[i].decal.is_valid()) rs->decal_set_normal_fade(_decals[i].decal, _normal_fade);
		}
	}
}
float MultiNodeDecal::get_normal_fade() const { return _normal_fade; }

void MultiNodeDecal::set_upper_fade(float p_fade) {
	_upper_fade = p_fade;
	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs) {
		for (int i = 0; i < _decals.size(); i++) {
			if (_decals[i].decal.is_valid()) rs->decal_set_fade(_decals[i].decal, _upper_fade, _lower_fade);
		}
	}
}
float MultiNodeDecal::get_upper_fade() const { return _upper_fade; }

void MultiNodeDecal::set_lower_fade(float p_fade) {
	_lower_fade = p_fade;
	RenderingServer *rs = RenderingServer::get_singleton();
	if (rs) {
		for (int i = 0; i < _decals.size(); i++) {
			if (_decals[i].decal.is_valid()) rs->decal_set_fade(_decals[i].decal, _upper_fade, _lower_fade);
		}
	}
}
float MultiNodeDecal::get_lower_fade() const { return _lower_fade; }

// --- Tint via decal modulate ---

void MultiNodeDecal::_on_instance_color_changed(int p_index, const Color &p_color) {
	if (p_index >= 0 && p_index < _decals.size() && _decals[p_index].decal.is_valid()) {
		RenderingServer::get_singleton()->decal_set_modulate(_decals[p_index].decal, p_color);
	}
}

void MultiNodeDecal::_on_colors_changed() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs) return;
	for (int i = 0; i < _decals.size(); i++) {
		if (_decals[i].decal.is_valid()) {
			rs->decal_set_modulate(_decals[i].decal, get_tinted_color(i));
		}
	}
}

void MultiNodeDecal::_clear_decals() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs) { _decals.clear(); return; }
	for (int i = 0; i < _decals.size(); i++) {
		if (_decals[i].instance.is_valid()) rs->free_rid(_decals[i].instance);
		if (_decals[i].decal.is_valid()) rs->free_rid(_decals[i].decal);
	}
	_decals.clear();
}

void MultiNodeDecal::_rebuild() {
	_clear_decals();
	if (!_parent || !is_inside_tree()) return;
	_sync_decals();
}

void MultiNodeDecal::_sync_decals() {
	if (!_parent || !is_inside_tree()) return;

	RenderingServer *rs = RenderingServer::get_singleton();
	int count = _parent->get_instance_count();
	int old_count = _decals.size();

	for (int i = count; i < old_count; i++) {
		if (_decals[i].instance.is_valid()) rs->free_rid(_decals[i].instance);
		if (_decals[i].decal.is_valid()) rs->free_rid(_decals[i].decal);
	}
	_decals.resize(count);

	Ref<World3D> world = get_world_3d();
	RID scenario = world.is_valid() ? world->get_scenario() : RID();
	Transform3D global_xform = get_global_transform();
	const Vector<Transform3D> &transforms = _parent->get_transforms_internal();
	const PackedByteArray &dirty = _parent->get_dirty_flags_internal();
	const uint8_t *active_ptr = _active.size() > 0 ? _active.ptr() : nullptr;

	for (int i = 0; i < count; i++) {
		bool is_active = !active_ptr || (i < _active.size() && active_ptr[i]);

		if (i >= old_count) {
			DecalInstance di;
			di.decal = rs->decal_create();
			rs->decal_set_size(di.decal, _decal_size);
			rs->decal_set_albedo_mix(di.decal, _albedo_mix);
			rs->decal_set_normal_fade(di.decal, _normal_fade);
			rs->decal_set_fade(di.decal, _upper_fade, _lower_fade);
			if (_texture_albedo.is_valid()) {
				rs->decal_set_texture(di.decal, RenderingServer::DECAL_TEXTURE_ALBEDO, _texture_albedo->get_rid());
			}
			rs->decal_set_modulate(di.decal, get_tinted_color(i));

			di.instance = rs->instance_create();
			rs->instance_set_base(di.instance, di.decal);
			if (scenario.is_valid()) rs->instance_set_scenario(di.instance, scenario);

			rs->instance_set_layer_mask(di.instance, get_render_layers());

			Transform3D xform = global_xform * compute_instance_transform(transforms[i]);
			rs->instance_set_transform(di.instance, xform);
			rs->instance_set_visible(di.instance, is_active);

			_decals.write[i] = di;
		} else if (dirty[i] && _decals[i].instance.is_valid()) {
			Transform3D xform = global_xform * compute_instance_transform(transforms[i]);
			rs->instance_set_transform(_decals[i].instance, xform);
			rs->instance_set_visible(_decals[i].instance, is_active);
		} else if (!dirty[i] && _any_active_changed && did_instance_active_change(i) && _decals[i].instance.is_valid()) {
			// Active flag changed on non-dirty instance — update visibility and transform.
			rs->instance_set_visible(_decals[i].instance, is_active);
			if (is_active) {
				Transform3D xform = global_xform * compute_instance_transform(transforms[i]);
				rs->instance_set_transform(_decals[i].instance, xform);
			}
		}
	}
}
