#include "multi_node_visual_3d.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

MultiNodeVisual3D::MultiNodeVisual3D() {}
MultiNodeVisual3D::~MultiNodeVisual3D() {}

void MultiNodeVisual3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_cast_shadow", "mode"), &MultiNodeVisual3D::set_cast_shadow);
	ClassDB::bind_method(D_METHOD("get_cast_shadow"), &MultiNodeVisual3D::get_cast_shadow);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cast_shadow", PROPERTY_HINT_ENUM, "Off,On,Double-Sided,Shadows Only"), "set_cast_shadow", "get_cast_shadow");

	ClassDB::bind_method(D_METHOD("set_render_layers", "layers"), &MultiNodeVisual3D::set_render_layers);
	ClassDB::bind_method(D_METHOD("get_render_layers"), &MultiNodeVisual3D::get_render_layers);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "render_layers", PROPERTY_HINT_LAYERS_3D_RENDER), "set_render_layers", "get_render_layers");

	ClassDB::bind_method(D_METHOD("set_visibility_range_begin", "distance"), &MultiNodeVisual3D::set_visibility_range_begin);
	ClassDB::bind_method(D_METHOD("get_visibility_range_begin"), &MultiNodeVisual3D::get_visibility_range_begin);

	ClassDB::bind_method(D_METHOD("set_visibility_range_end", "distance"), &MultiNodeVisual3D::set_visibility_range_end);
	ClassDB::bind_method(D_METHOD("get_visibility_range_end"), &MultiNodeVisual3D::get_visibility_range_end);

	ADD_GROUP("Visibility Range", "visibility_range_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "visibility_range_begin", PROPERTY_HINT_RANGE, "0.0,10000.0,0.1,or_greater,suffix:m"), "set_visibility_range_begin", "get_visibility_range_begin");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "visibility_range_end", PROPERTY_HINT_RANGE, "0.0,10000.0,0.1,or_greater,suffix:m"), "set_visibility_range_end", "get_visibility_range_end");

	// --- Tint ---
	ClassDB::bind_method(D_METHOD("set_tint", "color"), &MultiNodeVisual3D::set_tint);
	ClassDB::bind_method(D_METHOD("get_tint"), &MultiNodeVisual3D::get_tint);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "tint"), "set_tint", "get_tint");

	// --- Per-instance color tint ---
	ClassDB::bind_method(D_METHOD("set_instance_color", "index", "color"), &MultiNodeVisual3D::set_instance_color);
	ClassDB::bind_method(D_METHOD("get_instance_color", "index"), &MultiNodeVisual3D::get_instance_color);
	ClassDB::bind_method(D_METHOD("set_all_colors", "colors"), &MultiNodeVisual3D::set_all_colors);

	BIND_ENUM_CONSTANT(SHADOW_CASTING_OFF);
	BIND_ENUM_CONSTANT(SHADOW_CASTING_ON);
	BIND_ENUM_CONSTANT(SHADOW_CASTING_DOUBLE_SIDED);
	BIND_ENUM_CONSTANT(SHADOW_CASTING_SHADOWS_ONLY);
}

RID MultiNodeVisual3D::_get_visual_instance_rid() const {
	return RID(); // Subclasses override.
}

void MultiNodeVisual3D::_apply_visual_properties() {
	RID rid = _get_visual_instance_rid();
	if (!rid.is_valid()) {
		return;
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	rs->instance_geometry_set_cast_shadows_setting(rid,
			(RenderingServer::ShadowCastingSetting)(int)_cast_shadow);
	rs->instance_set_layer_mask(rid, _render_layers);
}

// ---------------------------------------------------------------------------
// Shadow / render layers / visibility range
// ---------------------------------------------------------------------------

void MultiNodeVisual3D::set_cast_shadow(ShadowCastingSetting p_mode) {
	_cast_shadow = p_mode;
	_apply_visual_properties();
}

MultiNodeVisual3D::ShadowCastingSetting MultiNodeVisual3D::get_cast_shadow() const {
	return _cast_shadow;
}

void MultiNodeVisual3D::set_render_layers(uint32_t p_layers) {
	_render_layers = p_layers;
	_apply_visual_properties();
}

uint32_t MultiNodeVisual3D::get_render_layers() const {
	return _render_layers;
}

void MultiNodeVisual3D::set_visibility_range_begin(float p_distance) {
	_visibility_range_begin = p_distance;
	_apply_visual_properties();
}

float MultiNodeVisual3D::get_visibility_range_begin() const {
	return _visibility_range_begin;
}

void MultiNodeVisual3D::set_visibility_range_end(float p_distance) {
	_visibility_range_end = p_distance;
	_apply_visual_properties();
}

float MultiNodeVisual3D::get_visibility_range_end() const {
	return _visibility_range_end;
}

// ---------------------------------------------------------------------------
// Tint
// ---------------------------------------------------------------------------

void MultiNodeVisual3D::set_tint(const Color &p_color) {
	_tint = p_color;
	// Re-push all existing colors with the new tint applied.
	_on_colors_changed();
}

Color MultiNodeVisual3D::get_tint() const {
	return _tint;
}

// ---------------------------------------------------------------------------
// Per-instance color
// ---------------------------------------------------------------------------

void MultiNodeVisual3D::set_instance_color(int p_index, const Color &p_color) {
	if (p_index < 0 || p_index >= _colors.size()) {
		return;
	}
	_colors.set(p_index, p_color);
	// Push tinted color to backend.
	Color tinted = Color(p_color.r * _tint.r, p_color.g * _tint.g, p_color.b * _tint.b, p_color.a * _tint.a);
	_on_instance_color_changed(p_index, tinted);
}

Color MultiNodeVisual3D::get_instance_color(int p_index) const {
	if (p_index < 0 || p_index >= _colors.size()) {
		return Color(1, 1, 1, 1);
	}
	return _colors[p_index];
}

void MultiNodeVisual3D::set_all_colors(const PackedColorArray &p_colors) {
	_colors = p_colors;
	_on_colors_changed();
}

void MultiNodeVisual3D::resize_colors(int p_count) {
	int old_size = _colors.size();
	_colors.resize(p_count);
	for (int i = old_size; i < p_count; i++) {
		_colors.set(i, Color(1, 1, 1, 1));
	}
}

Color MultiNodeVisual3D::get_tinted_color(int p_index) const {
	Color c = (p_index >= 0 && p_index < _colors.size()) ? _colors[p_index] : Color(1, 1, 1, 1);
	return Color(c.r * _tint.r, c.g * _tint.g, c.b * _tint.b, c.a * _tint.a);
}

void MultiNodeVisual3D::_on_instance_color_changed(int p_index, const Color &p_color) {
	// Default: no-op. Subclasses override to push to their backend.
}

void MultiNodeVisual3D::_on_colors_changed() {
	// Default: no-op. Subclasses override for bulk updates.
}
