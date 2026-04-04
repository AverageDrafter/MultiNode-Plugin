#include "multi_node_sprite.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>

using namespace godot;

static const Transform3D SPRITE_HIDDEN = Transform3D(Basis().scaled(Vector3(0, 0, 0)), Vector3(0, -10000, 0));

MultiNodeSprite::MultiNodeSprite() {}
MultiNodeSprite::~MultiNodeSprite() { _cleanup_rids(); }

void MultiNodeSprite::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_texture", "texture"), &MultiNodeSprite::set_texture);
	ClassDB::bind_method(D_METHOD("get_texture"), &MultiNodeSprite::get_texture);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture");

	ClassDB::bind_method(D_METHOD("set_pixel_size", "size"), &MultiNodeSprite::set_pixel_size);
	ClassDB::bind_method(D_METHOD("get_pixel_size"), &MultiNodeSprite::get_pixel_size);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pixel_size", PROPERTY_HINT_RANGE, "0.001,1.0,0.001,suffix:m/px"), "set_pixel_size", "get_pixel_size");

	ClassDB::bind_method(D_METHOD("set_hframes", "hframes"), &MultiNodeSprite::set_hframes);
	ClassDB::bind_method(D_METHOD("get_hframes"), &MultiNodeSprite::get_hframes);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "hframes", PROPERTY_HINT_RANGE, "1,256,1"), "set_hframes", "get_hframes");

	ClassDB::bind_method(D_METHOD("set_vframes", "vframes"), &MultiNodeSprite::set_vframes);
	ClassDB::bind_method(D_METHOD("get_vframes"), &MultiNodeSprite::get_vframes);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "vframes", PROPERTY_HINT_RANGE, "1,256,1"), "set_vframes", "get_vframes");

	ClassDB::bind_method(D_METHOD("set_billboard", "billboard"), &MultiNodeSprite::set_billboard);
	ClassDB::bind_method(D_METHOD("get_billboard"), &MultiNodeSprite::get_billboard);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "billboard"), "set_billboard", "get_billboard");

	ClassDB::bind_method(D_METHOD("set_pixel_art", "pixel_art"), &MultiNodeSprite::set_pixel_art);
	ClassDB::bind_method(D_METHOD("get_pixel_art"), &MultiNodeSprite::get_pixel_art);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "pixel_art"), "set_pixel_art", "get_pixel_art");

	ClassDB::bind_method(D_METHOD("set_instance_frame", "index", "frame"), &MultiNodeSprite::set_instance_frame);
	ClassDB::bind_method(D_METHOD("get_instance_frame", "index"), &MultiNodeSprite::get_instance_frame);
}

void MultiNodeSprite::_notification(int p_what) {
	RenderingServer *rs = RenderingServer::get_singleton();
	switch (p_what) {
		case NOTIFICATION_ENTER_WORLD: {
			if (_instance.is_valid()) {
				Ref<World3D> world = get_world_3d();
				if (world.is_valid()) {
					rs->instance_set_scenario(_instance, world->get_scenario());
				}
				rs->instance_set_transform(_instance, get_global_transform());
				rs->instance_set_visible(_instance, is_visible_in_tree());
			}
		} break;
		case NOTIFICATION_EXIT_WORLD: {
			if (_instance.is_valid()) rs->instance_set_scenario(_instance, RID());
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {
			if (_instance.is_valid()) rs->instance_set_transform(_instance, get_global_transform());
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (_instance.is_valid()) rs->instance_set_visible(_instance, is_visible_in_tree());
		} break;
	}
}

void MultiNodeSprite::_on_sub_entered() { set_notify_transform(true); _create_shader(); _rebuild(); }
void MultiNodeSprite::_on_sub_exiting() { _cleanup_rids(); }
void MultiNodeSprite::_on_parent_sync() { _sync_transforms(); }
void MultiNodeSprite::_on_active_changed() { if (is_inside_tree()) _rebuild(); }
RID MultiNodeSprite::_get_visual_instance_rid() const { return _instance; }

// --- Properties ---

void MultiNodeSprite::set_texture(const Ref<Texture2D> &p_texture) {
	_texture = p_texture;
	if (_shader_material.is_valid()) _shader_material->set_shader_parameter("sprite_texture", _texture);
	if (is_inside_tree()) _rebuild();
}
Ref<Texture2D> MultiNodeSprite::get_texture() const { return _texture; }

void MultiNodeSprite::set_pixel_size(float p_size) { _pixel_size = p_size; if (is_inside_tree()) _rebuild(); }
float MultiNodeSprite::get_pixel_size() const { return _pixel_size; }

void MultiNodeSprite::set_hframes(int p_hframes) {
	_hframes = MAX(1, p_hframes);
	if (_shader_material.is_valid()) _shader_material->set_shader_parameter("hframes", (float)_hframes);
	if (is_inside_tree()) _rebuild();
}
int MultiNodeSprite::get_hframes() const { return _hframes; }

void MultiNodeSprite::set_vframes(int p_vframes) {
	_vframes = MAX(1, p_vframes);
	if (_shader_material.is_valid()) _shader_material->set_shader_parameter("vframes", (float)_vframes);
	if (is_inside_tree()) _rebuild();
}
int MultiNodeSprite::get_vframes() const { return _vframes; }

void MultiNodeSprite::set_billboard(bool p_billboard) {
	_billboard = p_billboard;
	_create_shader();
	if (is_inside_tree()) _rebuild();
}
bool MultiNodeSprite::get_billboard() const { return _billboard; }

void MultiNodeSprite::set_pixel_art(bool p_pixel_art) {
	_pixel_art = p_pixel_art;
	_create_shader();
	if (is_inside_tree()) _rebuild();
}
bool MultiNodeSprite::get_pixel_art() const { return _pixel_art; }

// --- Per-instance ---

void MultiNodeSprite::_on_instance_color_changed(int p_index, const Color &p_color) {
	if (_multimesh.is_valid() && p_index < _current_count) {
		RenderingServer::get_singleton()->multimesh_instance_set_color(_multimesh, p_index, p_color);
	}
}

void MultiNodeSprite::set_instance_frame(int p_index, int p_frame) {
	if (p_index >= 0 && p_index < _frames.size()) {
		_frames.set(p_index, (float)p_frame);
		if (_multimesh.is_valid() && p_index < _current_count) {
			RenderingServer::get_singleton()->multimesh_instance_set_custom_data(
					_multimesh, p_index, Color((float)p_frame, 0, 0, 0));
		}
	}
}

int MultiNodeSprite::get_instance_frame(int p_index) const {
	return (p_index >= 0 && p_index < _frames.size()) ? (int)_frames[p_index] : 0;
}

// --- Shader generation ---

String MultiNodeSprite::_build_shader_code() const {
	String filter = _pixel_art ? "filter_nearest" : "filter_linear_mipmap";
	String code = "shader_type spatial;\n";
	code += "render_mode blend_mix, cull_disabled, unshaded;\n\n";
	code += "uniform sampler2D sprite_texture : source_color, " + filter + ";\n";
	code += "uniform float hframes = 1.0;\n";
	code += "uniform float vframes = 1.0;\n\n";
	code += "varying vec4 inst_color;\n";
	code += "varying float inst_frame;\n\n";
	code += "void vertex() {\n";
	code += "\tinst_color = COLOR;\n";
	code += "\tinst_frame = INSTANCE_CUSTOM.r;\n";

	if (_billboard) {
		code += "\tvec3 cam_right = normalize(vec3(INV_VIEW_MATRIX[0].x, 0.0, INV_VIEW_MATRIX[0].z));\n";
		code += "\tvec3 up = vec3(0.0, 1.0, 0.0);\n";
		code += "\tvec3 forward = normalize(cross(cam_right, up));\n";
		code += "\tfloat sx = length(MODEL_MATRIX[0].xyz);\n";
		code += "\tfloat sy = length(MODEL_MATRIX[1].xyz);\n";
		code += "\tfloat sz = length(MODEL_MATRIX[2].xyz);\n";
		code += "\tMODELVIEW_MATRIX = VIEW_MATRIX * mat4(\n";
		code += "\t\tvec4(cam_right * sx, 0.0),\n";
		code += "\t\tvec4(up * sy, 0.0),\n";
		code += "\t\tvec4(forward * sz, 0.0),\n";
		code += "\t\tMODEL_MATRIX[3]\n";
		code += "\t);\n";
	}

	code += "}\n\n";
	code += "void fragment() {\n";
	code += "\tint frame = int(round(inst_frame));\n";
	code += "\tint col = frame % int(hframes);\n";
	code += "\tint row = frame / int(hframes);\n";
	code += "\tvec2 frame_size = vec2(1.0 / hframes, 1.0 / vframes);\n";
	code += "\tvec2 uv = frame_size * (vec2(float(col), float(row)) + UV);\n";
	code += "\tvec4 tex = texture(sprite_texture, uv);\n";
	code += "\tALBEDO = tex.rgb * inst_color.rgb;\n";
	code += "\tALPHA = tex.a * inst_color.a;\n";
	code += "\tALPHA_SCISSOR_THRESHOLD = 0.1;\n";
	code += "}\n";
	return code;
}

void MultiNodeSprite::_create_shader() {
	// Skip if the shader already matches the current settings.
	if (_shader.is_valid() && _shader_billboard == _billboard && _shader_pixel_art == _pixel_art) {
		return;
	}

	// Release old resources before creating new ones.
	_shader.unref();
	_shader_material.unref();

	_shader.instantiate();
	_shader->set_code(_build_shader_code());
	_shader_material.instantiate();
	_shader_material->set_shader(_shader);
	if (_texture.is_valid()) _shader_material->set_shader_parameter("sprite_texture", _texture);
	_shader_material->set_shader_parameter("hframes", (float)_hframes);
	_shader_material->set_shader_parameter("vframes", (float)_vframes);

	_shader_billboard = _billboard;
	_shader_pixel_art = _pixel_art;
}

// --- RID management ---

void MultiNodeSprite::_rebuild() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs || !is_inside_tree() || !_parent) return;

	if (!_multimesh.is_valid()) _multimesh = rs->multimesh_create();
	if (!_instance.is_valid()) {
		_instance = rs->instance_create();
		rs->instance_set_base(_instance, _multimesh);
	}

	Ref<World3D> world = get_world_3d();
	if (world.is_valid()) rs->instance_set_scenario(_instance, world->get_scenario());
	rs->instance_set_transform(_instance, get_global_transform());
	rs->instance_set_visible(_instance, is_visible_in_tree());
	_apply_visual_properties();

	// Size quad based on one frame's dimensions.
	Vector2 size = Vector2(1, 1);
	if (_texture.is_valid()) {
		float frame_w = (float)_texture->get_width() / (float)_hframes;
		float frame_h = (float)_texture->get_height() / (float)_vframes;
		size = Vector2(frame_w, frame_h) * _pixel_size;
	}
	_quad_mesh.instantiate();
	_quad_mesh->set_size(size);
	rs->multimesh_set_mesh(_multimesh, _quad_mesh->get_rid());

	if (_shader_material.is_valid()) {
		rs->instance_geometry_set_material_override(_instance, _shader_material->get_rid());
	}

	_current_count = 0;
	_sync_transforms();
}

void MultiNodeSprite::_cleanup_rids() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs) return;
	if (_instance.is_valid()) { rs->free_rid(_instance); _instance = RID(); }
	if (_multimesh.is_valid()) { rs->free_rid(_multimesh); _multimesh = RID(); }
	_current_count = 0;
}

void MultiNodeSprite::_sync_transforms() {
	if (!_parent || !_multimesh.is_valid() || !is_inside_tree()) return;

	RenderingServer *rs = RenderingServer::get_singleton();
	int count = _parent->get_instance_count();

	bool resized = (_current_count != count);
	if (resized) {
		rs->multimesh_allocate_data(_multimesh, count,
				RenderingServer::MULTIMESH_TRANSFORM_3D, true, true);
		_current_count = count;

		resize_colors(count);

		int old_frames = _frames.size();
		_frames.resize(count);
		for (int i = old_frames; i < count; i++) _frames.set(i, 0.0f);
	}

	if (count == 0) return;

	const Vector<Transform3D> &transforms = _parent->get_transforms_internal();
	const PackedByteArray &dirty = _parent->get_dirty_flags_internal();
	const uint8_t *active_ptr = _active.size() > 0 ? _active.ptr() : nullptr;

	if (resized) {
		// Full upload via bulk buffer — Transform(12) + Color(4) + CustomData(4) = 20 floats per instance.
		int stride = 20;
		PackedFloat32Array buffer;
		buffer.resize(count * stride);
		float *buf = buffer.ptrw();

		for (int i = 0; i < count; i++) {
			int off = i * stride;
			Transform3D xform;
			if (active_ptr && i < _active.size() && !active_ptr[i]) {
				xform = SPRITE_HIDDEN;
			} else {
				xform = compute_instance_transform(transforms[i]);
			}
			buf[off + 0] = xform.basis[0][0]; buf[off + 1] = xform.basis[0][1]; buf[off + 2] = xform.basis[0][2]; buf[off + 3] = xform.origin.x;
			buf[off + 4] = xform.basis[1][0]; buf[off + 5] = xform.basis[1][1]; buf[off + 6] = xform.basis[1][2]; buf[off + 7] = xform.origin.y;
			buf[off + 8] = xform.basis[2][0]; buf[off + 9] = xform.basis[2][1]; buf[off + 10] = xform.basis[2][2]; buf[off + 11] = xform.origin.z;
			// Color.
			Color tc = get_tinted_color(i);
			buf[off + 12] = tc.r; buf[off + 13] = tc.g;
			buf[off + 14] = tc.b; buf[off + 15] = tc.a;
			// Custom data (frame index).
			buf[off + 16] = _frames[i]; buf[off + 17] = 0.0f;
			buf[off + 18] = 0.0f; buf[off + 19] = 0.0f;
		}
		rs->multimesh_set_buffer(_multimesh, buffer);
	} else {
		for (int i = 0; i < count; i++) {
			if (dirty[i]) {
				if (active_ptr && i < _active.size() && !active_ptr[i]) {
					rs->multimesh_instance_set_transform(_multimesh, i, SPRITE_HIDDEN);
				} else {
					rs->multimesh_instance_set_transform(_multimesh, i, compute_instance_transform(transforms[i]));
				}
			}
		}
		// Handle runtime active flag changes on non-dirty instances.
		if (_any_active_changed) {
			for (int i = 0; i < count; i++) {
				if (!dirty[i] && did_instance_active_change(i)) {
					if (active_ptr && i < _active.size() && !active_ptr[i]) {
						rs->multimesh_instance_set_transform(_multimesh, i, SPRITE_HIDDEN);
					} else {
						rs->multimesh_instance_set_transform(_multimesh, i, compute_instance_transform(transforms[i]));
					}
				}
			}
		}
	}
	rs->multimesh_set_visible_instances(_multimesh, -1);
}
