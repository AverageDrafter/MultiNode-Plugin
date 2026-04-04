#include "multi_node_label.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/system_font.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>

using namespace godot;

static const Transform3D LABEL_HIDDEN = Transform3D(Basis().scaled(Vector3(0, 0, 0)), Vector3(0, -10000, 0));

// Shader: INSTANCE_CUSTOM.r = atlas column, .g = atlas row, .b = quad aspect (width/height).
// Base quad is 1x1 (label_height x label_height); the shader scales x by aspect.
const char *MultiNodeLabel::BILLBOARD_SHADER_CODE = R"(
shader_type spatial;
render_mode blend_mix, cull_disabled, unshaded, shadows_disabled;

uniform sampler2D atlas : source_color, filter_linear_mipmap;
uniform int atlas_columns = 1;
uniform int atlas_rows = 1;
uniform vec4 tint_color : source_color = vec4(1.0, 1.0, 1.0, 1.0);

varying vec2 atlas_cell;

void vertex() {
	atlas_cell = INSTANCE_CUSTOM.rg;
	float aspect = max(INSTANCE_CUSTOM.b, 0.01);

	vec3 cam_right = normalize(vec3(INV_VIEW_MATRIX[0].x, 0.0, INV_VIEW_MATRIX[0].z));
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 forward = normalize(cross(cam_right, up));

	float sx = length(MODEL_MATRIX[0].xyz);
	float sy = length(MODEL_MATRIX[1].xyz);
	float sz = length(MODEL_MATRIX[2].xyz);

	MODELVIEW_MATRIX = VIEW_MATRIX * mat4(
		vec4(cam_right * sx * aspect, 0.0),
		vec4(up * sy, 0.0),
		vec4(forward * sz, 0.0),
		MODEL_MATRIX[3]
	);
}

void fragment() {
	int col = int(round(atlas_cell.x));
	int row = int(round(atlas_cell.y));

	vec2 cell_size = vec2(1.0 / float(atlas_columns), 1.0 / float(atlas_rows));
	vec2 cell_origin = vec2(float(col), float(row)) * cell_size;
	vec2 uv = cell_origin + UV * cell_size;

	vec4 tex = texture(atlas, uv);
	ALBEDO = tex.rgb * tint_color.rgb;
	ALPHA = tex.a * tint_color.a;
	ALPHA_SCISSOR_THRESHOLD = 0.1;
}
)";

MultiNodeLabel::MultiNodeLabel() {}

MultiNodeLabel::~MultiNodeLabel() {
	_cleanup_rids();
}

void MultiNodeLabel::_bind_methods() {
	ADD_GROUP("Text", "");

	ClassDB::bind_method(D_METHOD("set_font", "font"), &MultiNodeLabel::set_font);
	ClassDB::bind_method(D_METHOD("get_font"), &MultiNodeLabel::get_font);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "font", PROPERTY_HINT_RESOURCE_TYPE, "Font"), "set_font", "get_font");

	ClassDB::bind_method(D_METHOD("set_font_size", "size"), &MultiNodeLabel::set_font_size);
	ClassDB::bind_method(D_METHOD("get_font_size"), &MultiNodeLabel::get_font_size);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "font_size", PROPERTY_HINT_RANGE, "8,128,1"), "set_font_size", "get_font_size");

	ClassDB::bind_method(D_METHOD("set_text_color", "color"), &MultiNodeLabel::set_text_color);
	ClassDB::bind_method(D_METHOD("get_text_color"), &MultiNodeLabel::get_text_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "text_color"), "set_text_color", "get_text_color");

	ADD_GROUP("Outline", "outline_");

	ClassDB::bind_method(D_METHOD("set_outline_color", "color"), &MultiNodeLabel::set_outline_color);
	ClassDB::bind_method(D_METHOD("get_outline_color"), &MultiNodeLabel::get_outline_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "outline_color"), "set_outline_color", "get_outline_color");

	ClassDB::bind_method(D_METHOD("set_outline_size", "size"), &MultiNodeLabel::set_outline_size);
	ClassDB::bind_method(D_METHOD("get_outline_size"), &MultiNodeLabel::get_outline_size);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "outline_size", PROPERTY_HINT_RANGE, "0,32,1"), "set_outline_size", "get_outline_size");

	ADD_GROUP("Layout", "");

	ClassDB::bind_method(D_METHOD("set_label_height", "height"), &MultiNodeLabel::set_label_height);
	ClassDB::bind_method(D_METHOD("get_label_height"), &MultiNodeLabel::get_label_height);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "label_height", PROPERTY_HINT_RANGE, "0.1,10.0,0.1"), "set_label_height", "get_label_height");

	ClassDB::bind_method(D_METHOD("set_cell_padding", "padding"), &MultiNodeLabel::set_cell_padding);
	ClassDB::bind_method(D_METHOD("get_cell_padding"), &MultiNodeLabel::get_cell_padding);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cell_padding", PROPERTY_HINT_RANGE, "0,32,1"), "set_cell_padding", "get_cell_padding");

	ClassDB::bind_method(D_METHOD("set_render_scale", "scale"), &MultiNodeLabel::set_render_scale);
	ClassDB::bind_method(D_METHOD("get_render_scale"), &MultiNodeLabel::get_render_scale);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "render_scale", PROPERTY_HINT_RANGE, "1.0,4.0,0.5"), "set_render_scale", "get_render_scale");

	ClassDB::bind_method(D_METHOD("set_max_label_distance", "distance"), &MultiNodeLabel::set_max_label_distance);
	ClassDB::bind_method(D_METHOD("get_max_label_distance"), &MultiNodeLabel::get_max_label_distance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_label_distance", PROPERTY_HINT_RANGE, "0.0,10000.0,1.0"), "set_max_label_distance", "get_max_label_distance");

	ADD_GROUP("", "");

	ClassDB::bind_method(D_METHOD("set_instance_text", "index", "text"), &MultiNodeLabel::set_instance_text);
	ClassDB::bind_method(D_METHOD("get_instance_text", "index"), &MultiNodeLabel::get_instance_text);

	ClassDB::bind_method(D_METHOD("set_all_texts", "text"), &MultiNodeLabel::set_all_texts);
	ClassDB::bind_method(D_METHOD("set_texts", "texts"), &MultiNodeLabel::set_texts);
	ClassDB::bind_method(D_METHOD("begin_text_batch"), &MultiNodeLabel::begin_text_batch);
	ClassDB::bind_method(D_METHOD("end_text_batch"), &MultiNodeLabel::end_text_batch);

	// Internal.
	ClassDB::bind_method(D_METHOD("_finish_atlas_build"), &MultiNodeLabel::_finish_atlas_build);
}

// ---------------------------------------------------------------------------
// Notifications
// ---------------------------------------------------------------------------

void MultiNodeLabel::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (_needs_atlas_rebuild && !_atlas_viewport) {
				_rebuild();
			} else if (_atlas_viewport && _atlas_wait_frames >= 2) {
				_finish_atlas_build();
				set_process_internal(false);
			}
			if (_atlas_viewport) {
				_atlas_wait_frames++;
			}
		} break;

		case NOTIFICATION_ENTER_WORLD: {
			if (_instance.is_valid()) {
				RenderingServer *rs = RenderingServer::get_singleton();
				Ref<World3D> world = get_world_3d();
				if (world.is_valid()) {
					rs->instance_set_scenario(_instance, world->get_scenario());
				}
				rs->instance_set_transform(_instance, get_global_transform());
				rs->instance_set_visible(_instance, is_visible_in_tree());
			}
		} break;

		case NOTIFICATION_EXIT_WORLD: {
			if (_instance.is_valid()) {
				RenderingServer::get_singleton()->instance_set_scenario(_instance, RID());
			}
		} break;

		case NOTIFICATION_TRANSFORM_CHANGED: {
			if (_instance.is_valid()) {
				RenderingServer::get_singleton()->instance_set_transform(_instance, get_global_transform());
			}
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (_instance.is_valid()) {
				RenderingServer::get_singleton()->instance_set_visible(_instance, is_visible_in_tree());
			}
		} break;
	}
}

// ---------------------------------------------------------------------------
// MultiNode3D overrides
// ---------------------------------------------------------------------------

void MultiNodeLabel::_on_sub_entered() {
	set_notify_transform(true);
	_create_shader();
	_rebuild();
}

void MultiNodeLabel::_on_sub_exiting() {
	if (_atlas_viewport) {
		_atlas_viewport->queue_free();
		_atlas_viewport = nullptr;
	}
	if (_atlas_canvas_item.is_valid()) {
		RenderingServer::get_singleton()->free_rid(_atlas_canvas_item);
		_atlas_canvas_item = RID();
	}
	set_process_internal(false);
	_cleanup_rids();
}

void MultiNodeLabel::_on_parent_sync() {
	if (!_parent) {
		return;
	}

	int count = _parent->get_instance_count();
	int old_count = _texts.size();
	if (count != old_count) {
		_texts.resize(count);
		// New entries are empty (no label by default).
	}

	_sync_positions();
}

void MultiNodeLabel::_on_active_changed() {
	if (is_inside_tree()) {
		_rebuild();
	}
}

RID MultiNodeLabel::_get_visual_instance_rid() const {
	return _instance;
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------

void MultiNodeLabel::set_font(const Ref<Font> &p_font) { _font = p_font; _needs_atlas_rebuild = true; if (is_inside_tree()) set_process_internal(true); }
Ref<Font> MultiNodeLabel::get_font() const { return _font; }

void MultiNodeLabel::set_font_size(int p_size) { _font_size = p_size; _needs_atlas_rebuild = true; if (is_inside_tree()) set_process_internal(true); }
int MultiNodeLabel::get_font_size() const { return _font_size; }

void MultiNodeLabel::set_text_color(const Color &p_color) { _text_color = p_color; _needs_atlas_rebuild = true; if (is_inside_tree()) set_process_internal(true); }
Color MultiNodeLabel::get_text_color() const { return _text_color; }

void MultiNodeLabel::set_outline_color(const Color &p_color) { _outline_color = p_color; _needs_atlas_rebuild = true; if (is_inside_tree()) set_process_internal(true); }
Color MultiNodeLabel::get_outline_color() const { return _outline_color; }

void MultiNodeLabel::set_outline_size(int p_size) { _outline_size = p_size; _needs_atlas_rebuild = true; if (is_inside_tree()) set_process_internal(true); }
int MultiNodeLabel::get_outline_size() const { return _outline_size; }

void MultiNodeLabel::set_label_height(float p_height) { _label_height = p_height; if (is_inside_tree()) { _rebuild(); } }
float MultiNodeLabel::get_label_height() const { return _label_height; }

void MultiNodeLabel::set_cell_padding(int p_padding) { _cell_padding = MAX(0, p_padding); _needs_atlas_rebuild = true; if (is_inside_tree()) set_process_internal(true); }
int MultiNodeLabel::get_cell_padding() const { return _cell_padding; }

void MultiNodeLabel::set_render_scale(float p_scale) { _render_scale = CLAMP(p_scale, 1.0f, 4.0f); _needs_atlas_rebuild = true; if (is_inside_tree()) set_process_internal(true); }
float MultiNodeLabel::get_render_scale() const { return _render_scale; }

void MultiNodeLabel::set_max_label_distance(float p_dist) { _max_label_distance = MAX(0.0f, p_dist); }
float MultiNodeLabel::get_max_label_distance() const { return _max_label_distance; }

// ---------------------------------------------------------------------------
// Per-instance text
// ---------------------------------------------------------------------------

void MultiNodeLabel::set_instance_text(int p_index, const String &p_text) {
	if (p_index < 0 || p_index >= _texts.size()) {
		return;
	}
	if (_texts[p_index] == p_text) {
		return;
	}
	_texts.set(p_index, p_text);
	_needs_atlas_rebuild = true;
	if (!_text_batch_mode && is_inside_tree()) {
		set_process_internal(true);
	}
}

String MultiNodeLabel::get_instance_text(int p_index) const {
	if (p_index < 0 || p_index >= _texts.size()) {
		return String();
	}
	return _texts[p_index];
}

// ---------------------------------------------------------------------------
// Bulk text API
// ---------------------------------------------------------------------------

void MultiNodeLabel::set_all_texts(const String &p_text) {
	bool changed = false;
	for (int i = 0; i < _texts.size(); i++) {
		if (_texts[i] != p_text) {
			_texts.set(i, p_text);
			changed = true;
		}
	}
	if (changed) {
		_needs_atlas_rebuild = true;
		if (!_text_batch_mode && is_inside_tree()) {
			set_process_internal(true);
		}
	}
}

void MultiNodeLabel::set_texts(const PackedStringArray &p_texts) {
	int to_set = MIN(p_texts.size(), _texts.size());
	for (int i = 0; i < to_set; i++) {
		if (_texts[i] != p_texts[i]) {
			_texts.set(i, p_texts[i]);
			_needs_atlas_rebuild = true;
		}
	}
	if (_needs_atlas_rebuild && !_text_batch_mode && is_inside_tree()) {
		set_process_internal(true);
	}
}

// ---------------------------------------------------------------------------
// Batch text API
// ---------------------------------------------------------------------------

void MultiNodeLabel::begin_text_batch() {
	_text_batch_mode = true;
}

void MultiNodeLabel::end_text_batch() {
	_text_batch_mode = false;
	if (_needs_atlas_rebuild && is_inside_tree()) {
		set_process_internal(true);
	}
}

// ---------------------------------------------------------------------------
// Shader
// ---------------------------------------------------------------------------

void MultiNodeLabel::_on_colors_changed() {
	if (_shader_material.is_valid()) {
		_shader_material->set_shader_parameter("tint_color", get_tint());
	}
}

void MultiNodeLabel::_create_shader() {
	if (_shader.is_valid()) {
		return;
	}
	_shader.instantiate();
	_shader->set_code(BILLBOARD_SHADER_CODE);

	_shader_material.instantiate();
	_shader_material->set_shader(_shader);
	_shader_material->set_shader_parameter("tint_color", get_tint());
}

// ---------------------------------------------------------------------------
// Font resolution helper
// ---------------------------------------------------------------------------

Ref<Font> MultiNodeLabel::_resolve_font() const {
	if (_font.is_valid()) {
		return _font;
	}
	ThemeDB *theme_db = ThemeDB::get_singleton();
	if (theme_db) {
		Ref<Font> fallback = theme_db->get_fallback_font();
		if (fallback.is_valid()) {
			return fallback;
		}
	}
	Ref<SystemFont> sys_font;
	sys_font.instantiate();
	return sys_font;
}

// ---------------------------------------------------------------------------
// RID management
// ---------------------------------------------------------------------------

void MultiNodeLabel::_rebuild() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs || !is_inside_tree() || !_parent) {
		return;
	}

	if (!_multimesh.is_valid()) {
		_multimesh = rs->multimesh_create();
	}
	if (!_instance.is_valid()) {
		_instance = rs->instance_create();
		rs->instance_set_base(_instance, _multimesh);
	}

	Ref<World3D> world = get_world_3d();
	if (world.is_valid()) {
		rs->instance_set_scenario(_instance, world->get_scenario());
	}
	rs->instance_set_transform(_instance, get_global_transform());
	rs->instance_set_visible(_instance, is_visible_in_tree());
	_apply_visual_properties();

	// 1:1 base quad — shader scales width via INSTANCE_CUSTOM.b (aspect ratio).
	Ref<QuadMesh> quad;
	quad.instantiate();
	quad->set_size(Vector2(_label_height, _label_height));
	_quad_mesh = quad;
	rs->multimesh_set_mesh(_multimesh, _quad_mesh->get_rid());

	if (_shader_material.is_valid()) {
		rs->instance_geometry_set_material_override(_instance, _shader_material->get_rid());
	}

	if (_needs_atlas_rebuild) {
		_start_atlas_build();
	} else {
		_current_count = 0;
		_sync_positions();
	}
}

void MultiNodeLabel::_cleanup_rids() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs) {
		return;
	}
	if (_instance.is_valid()) {
		rs->free_rid(_instance);
		_instance = RID();
	}
	if (_multimesh.is_valid()) {
		rs->free_rid(_multimesh);
		_multimesh = RID();
	}
	_current_count = 0;
}

// ---------------------------------------------------------------------------
// Atlas build — direct font drawing (no Label control nodes)
// ---------------------------------------------------------------------------

void MultiNodeLabel::_start_atlas_build() {
	if (_atlas_viewport) {
		_atlas_viewport->queue_free();
		_atlas_viewport = nullptr;
	}
	if (_atlas_canvas_item.is_valid()) {
		RenderingServer::get_singleton()->free_rid(_atlas_canvas_item);
		_atlas_canvas_item = RID();
	}

	// Collect unique non-empty texts.
	_text_to_cell.clear();
	_text_to_aspect.clear();
	PackedStringArray unique_texts;

	for (int i = 0; i < _texts.size(); i++) {
		const String &text = _texts[i];
		if (text.is_empty()) {
			continue;
		}
		if (!_text_to_cell.has(text)) {
			_text_to_cell.insert(text, unique_texts.size());
			unique_texts.push_back(text);
		}
	}

	int unique_count = unique_texts.size();
	if (unique_count == 0) {
		_needs_atlas_rebuild = false;
		_atlas_texture.unref();
		_current_count = 0;
		return;
	}

	Ref<Font> used_font = _resolve_font();

	// --- #1: Dynamic cell sizing — measure all unique texts, use max dimensions. ---
	int max_text_w = 1;
	int max_text_h = 1;
	for (int i = 0; i < unique_count; i++) {
		Vector2 ts = used_font->get_string_size(unique_texts[i], HORIZONTAL_ALIGNMENT_LEFT, -1, _font_size);
		max_text_w = MAX(max_text_w, (int)Math::ceil(ts.x));
		max_text_h = MAX(max_text_h, (int)Math::ceil(ts.y));
	}
	int cell_w = max_text_w + _cell_padding * 2;
	int cell_h = max_text_h + _cell_padding * 2;

	// --- #4: Per-text aspect ratios (text content width / cell height). ---
	// Even with a uniform grid, each text's quad is only as wide as its content.
	// Transparent cell margins are clipped by ALPHA_SCISSOR_THRESHOLD in the shader.
	for (int i = 0; i < unique_count; i++) {
		Vector2 ts = used_font->get_string_size(unique_texts[i], HORIZONTAL_ALIGNMENT_LEFT, -1, _font_size);
		float text_content_w = ts.x + _cell_padding * 2.0f;
		_text_to_aspect.insert(unique_texts[i], text_content_w / (float)cell_h);
	}

	// Grid layout.
	_atlas_columns = (int)Math::ceil(Math::sqrt((double)unique_count));
	_atlas_rows = (int)Math::ceil((double)unique_count / (double)_atlas_columns);

	// --- #3: Scale viewport for high-resolution rendering. ---
	int render_font_size  = (int)(_font_size * _render_scale);
	int render_outline_sz = (int)(_outline_size * _render_scale);
	int render_padding    = (int)(_cell_padding * _render_scale);
	int cell_w_scaled     = (int)(cell_w * _render_scale);
	int cell_h_scaled     = (int)(cell_h * _render_scale);

	_atlas_viewport = memnew(SubViewport);
	_atlas_viewport->set_size(Vector2i(_atlas_columns * cell_w_scaled, _atlas_rows * cell_h_scaled));
	_atlas_viewport->set_transparent_background(true);
	_atlas_viewport->set_update_mode(SubViewport::UPDATE_DISABLED);
	add_child(_atlas_viewport);

	// --- #2: Direct font drawing — no Label control nodes. ---
	Ref<World2D> world_2d = _atlas_viewport->find_world_2d();
	if (world_2d.is_valid()) {
		RenderingServer *rs = RenderingServer::get_singleton();
		RID canvas = world_2d->get_canvas();
		_atlas_canvas_item = rs->canvas_item_create();
		rs->canvas_item_set_parent(_atlas_canvas_item, canvas);

		float ascent = used_font->get_ascent(render_font_size);

		for (int i = 0; i < unique_count; i++) {
			int col = i % _atlas_columns;
			int row = i / _atlas_columns;

			// Cell origin in the scaled viewport.
			float cell_x = (float)(col * cell_w_scaled);
			float cell_y = (float)(row * cell_h_scaled);

			// Baseline: top of cell + padding + font ascent.
			// Horizontal alignment is CENTER within the full cell width.
			Vector2 pos(cell_x, cell_y + render_padding + ascent);

			if (_outline_size > 0) {
				used_font->draw_string_outline(
						_atlas_canvas_item, pos, unique_texts[i],
						HORIZONTAL_ALIGNMENT_CENTER, (float)cell_w_scaled,
						render_font_size, render_outline_sz, _outline_color);
			}
			used_font->draw_string(
					_atlas_canvas_item, pos, unique_texts[i],
					HORIZONTAL_ALIGNMENT_CENTER, (float)cell_w_scaled,
					render_font_size, _text_color);
		}
	}

	_atlas_viewport->set_update_mode(SubViewport::UPDATE_ONCE);
	_atlas_wait_frames = 0;
	_needs_atlas_rebuild = false;
	set_process_internal(true);
}

void MultiNodeLabel::_finish_atlas_build() {
	if (!_atlas_viewport) {
		return;
	}

	// Free the canvas item used for drawing.
	if (_atlas_canvas_item.is_valid()) {
		RenderingServer::get_singleton()->free_rid(_atlas_canvas_item);
		_atlas_canvas_item = RID();
	}

	// Grab rendered image and generate mipmaps (#3).
	Ref<ViewportTexture> vp_tex = _atlas_viewport->get_texture();
	if (vp_tex.is_valid()) {
		Ref<Image> img = vp_tex->get_image();
		if (img.is_valid()) {
			img->generate_mipmaps();
			_atlas_texture = ImageTexture::create_from_image(img);
		}
	}

	_atlas_viewport->queue_free();
	_atlas_viewport = nullptr;

	if (_shader_material.is_valid() && _atlas_texture.is_valid()) {
		_shader_material->set_shader_parameter("atlas", _atlas_texture);
		_shader_material->set_shader_parameter("atlas_columns", _atlas_columns);
		_shader_material->set_shader_parameter("atlas_rows", _atlas_rows);
	}

	_current_count = 0;
	_sync_positions();
}

// ---------------------------------------------------------------------------
// Position sync
// ---------------------------------------------------------------------------

void MultiNodeLabel::_sync_positions() {
	if (!_parent || !_multimesh.is_valid() || !is_inside_tree()) {
		return;
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	int count = _parent->get_instance_count();

	if (count == 0) {
		if (_current_count > 0) {
			rs->multimesh_allocate_data(_multimesh, 0,
					RenderingServer::MULTIMESH_TRANSFORM_3D, false, true);
			_current_count = 0;
		}
		return;
	}

	bool resized = (_current_count != count);
	if (resized) {
		rs->multimesh_allocate_data(_multimesh, count,
				RenderingServer::MULTIMESH_TRANSFORM_3D, false, true);
		_current_count = count;
	}

	int dirty_count = _parent->get_dirty_count();
	if (dirty_count == 0 && !resized && !_any_active_changed) {
		return;
	}

	const Vector<Transform3D> &transforms = _parent->get_transforms_internal();
	const PackedByteArray &dirty = _parent->get_dirty_flags_internal();
	const uint8_t *active_ptr = _active.ptr();

	// --- #7: Distance-based label visibility. ---
	bool do_dist_cull = (_max_label_distance > 0.0f);
	float dist_sq_limit = _max_label_distance * _max_label_distance;
	Vector3 local_cam;
	if (do_dist_cull) {
		Viewport *vp = get_viewport();
		Camera3D *cam = vp ? Object::cast_to<Camera3D>(vp->get_camera_3d()) : nullptr;
		if (cam) {
			local_cam = get_global_transform().affine_inverse().xform(cam->get_global_position());
		} else {
			do_dist_cull = false;
		}
	}

	auto get_aspect = [&](const String &text) -> float {
		const float *asp = _text_to_aspect.getptr(text);
		return asp ? *asp : 1.0f;
	};

	auto is_visible = [&](int i) -> bool {
		if (!(active_ptr && i < _active.size() && active_ptr[i])) return false;
		if (i >= _texts.size() || _texts[i].is_empty()) return false;
		if (!_text_to_cell.has(_texts[i])) return false;
		if (do_dist_cull && transforms[i].origin.distance_squared_to(local_cam) > dist_sq_limit) return false;
		return true;
	};

	if (resized) {
		// Full upload — Transform(12) + CustomData(4) = 16 floats per instance.
		int stride = 16;
		PackedFloat32Array buffer;
		buffer.resize(count * stride);
		float *buf = buffer.ptrw();

		for (int i = 0; i < count; i++) {
			int off = i * stride;
			Transform3D xform;
			float custom_col = 0.0f, custom_row = 0.0f, custom_aspect = 1.0f;

			if (is_visible(i)) {
				xform = compute_instance_transform(transforms[i]);
				int cell = _text_to_cell[_texts[i]];
				custom_col    = (float)(cell % _atlas_columns);
				custom_row    = (float)(cell / _atlas_columns);
				custom_aspect = get_aspect(_texts[i]);
			} else {
				xform = LABEL_HIDDEN;
			}

			buf[off + 0]  = xform.basis[0][0]; buf[off + 1]  = xform.basis[0][1]; buf[off + 2]  = xform.basis[0][2]; buf[off + 3]  = xform.origin.x;
			buf[off + 4]  = xform.basis[1][0]; buf[off + 5]  = xform.basis[1][1]; buf[off + 6]  = xform.basis[1][2]; buf[off + 7]  = xform.origin.y;
			buf[off + 8]  = xform.basis[2][0]; buf[off + 9]  = xform.basis[2][1]; buf[off + 10] = xform.basis[2][2]; buf[off + 11] = xform.origin.z;
			buf[off + 12] = custom_col;
			buf[off + 13] = custom_row;
			buf[off + 14] = custom_aspect;
			buf[off + 15] = 0.0f;
		}
		rs->multimesh_set_buffer(_multimesh, buffer);
	} else {
		// Sparse — only dirty instances.
		for (int i = 0; i < count; i++) {
			if (!dirty[i]) {
				continue;
			}
			if (is_visible(i)) {
				int cell = _text_to_cell[_texts[i]];
				rs->multimesh_instance_set_transform(_multimesh, i, compute_instance_transform(transforms[i]));
				rs->multimesh_instance_set_custom_data(_multimesh, i, Color(
						(float)(cell % _atlas_columns),
						(float)(cell / _atlas_columns),
						get_aspect(_texts[i]), 0.0f));
			} else {
				rs->multimesh_instance_set_transform(_multimesh, i, LABEL_HIDDEN);
			}
		}
		// Runtime active-flag changes on non-dirty instances.
		if (_any_active_changed) {
			for (int i = 0; i < count; i++) {
				if (!dirty[i] && did_instance_active_change(i)) {
					if (is_visible(i)) {
						int cell = _text_to_cell[_texts[i]];
						rs->multimesh_instance_set_transform(_multimesh, i, compute_instance_transform(transforms[i]));
						rs->multimesh_instance_set_custom_data(_multimesh, i, Color(
								(float)(cell % _atlas_columns),
								(float)(cell / _atlas_columns),
								get_aspect(_texts[i]), 0.0f));
					} else {
						rs->multimesh_instance_set_transform(_multimesh, i, LABEL_HIDDEN);
					}
				}
			}
		}
	}

	rs->multimesh_set_visible_instances(_multimesh, -1);
}
