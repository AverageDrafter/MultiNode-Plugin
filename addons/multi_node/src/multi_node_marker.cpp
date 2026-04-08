#include "multi_node_marker.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

#include <cmath>

using namespace godot;

static const Transform3D HIDDEN_TRANSFORM = Transform3D(Basis().scaled(Vector3(0, 0, 0)), Vector3(0, -10000, 0));
static const float MN_TAU = 6.28318530718f;

MultiNodeMarker::MultiNodeMarker() {}
MultiNodeMarker::~MultiNodeMarker() { _cleanup_rids(); }

// ---------------------------------------------------------------------------
// Bind
// ---------------------------------------------------------------------------

void MultiNodeMarker::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_shape", "shape"), &MultiNodeMarker::set_shape);
	ClassDB::bind_method(D_METHOD("get_shape"), &MultiNodeMarker::get_shape);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "shape", PROPERTY_HINT_ENUM, "Cross,Diamond,Sphere,Axes"), "set_shape", "get_shape");

	ClassDB::bind_method(D_METHOD("set_marker_size", "size"), &MultiNodeMarker::set_marker_size);
	ClassDB::bind_method(D_METHOD("get_marker_size"), &MultiNodeMarker::get_marker_size);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "marker_size", PROPERTY_HINT_RANGE, "0.01,20.0,0.01,suffix:m"), "set_marker_size", "get_marker_size");

	ClassDB::bind_method(D_METHOD("set_marker_color", "color"), &MultiNodeMarker::set_marker_color);
	ClassDB::bind_method(D_METHOD("get_marker_color"), &MultiNodeMarker::get_marker_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "marker_color"), "set_marker_color", "get_marker_color");

	ClassDB::bind_method(D_METHOD("set_editor_only", "editor_only"), &MultiNodeMarker::set_editor_only);
	ClassDB::bind_method(D_METHOD("get_editor_only"), &MultiNodeMarker::get_editor_only);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "editor_only"), "set_editor_only", "get_editor_only");

	ClassDB::bind_method(D_METHOD("set_always_on_top", "always_on_top"), &MultiNodeMarker::set_always_on_top);
	ClassDB::bind_method(D_METHOD("get_always_on_top"), &MultiNodeMarker::get_always_on_top);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "always_on_top"), "set_always_on_top", "get_always_on_top");

	BIND_ENUM_CONSTANT(SHAPE_CROSS);
	BIND_ENUM_CONSTANT(SHAPE_DIAMOND);
	BIND_ENUM_CONSTANT(SHAPE_SPHERE);
	BIND_ENUM_CONSTANT(SHAPE_AXES);
}

// ---------------------------------------------------------------------------
// Notification
// ---------------------------------------------------------------------------

void MultiNodeMarker::_notification(int p_what) {
	switch (p_what) {
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
// MultiNodeSub overrides
// ---------------------------------------------------------------------------

void MultiNodeMarker::_on_sub_entered() { set_notify_transform(true); _rebuild(); }
void MultiNodeMarker::_on_sub_exiting() { _cleanup_rids(); }
void MultiNodeMarker::_on_parent_sync() { _sync_transforms(); }
void MultiNodeMarker::_on_active_changed() { if (is_inside_tree()) _rebuild(); }

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------

void MultiNodeMarker::set_shape(MarkerShape p_shape) { _shape = p_shape; if (is_inside_tree()) _rebuild(); }
MultiNodeMarker::MarkerShape MultiNodeMarker::get_shape() const { return _shape; }

void MultiNodeMarker::set_marker_size(float p_size) { _marker_size = p_size; if (is_inside_tree()) _rebuild(); }
float MultiNodeMarker::get_marker_size() const { return _marker_size; }

void MultiNodeMarker::set_marker_color(const Color &p_color) {
	_marker_color = p_color;
	// Update material albedo directly without full rebuild when only color changes.
	if (_marker_material.is_valid() && _shape != SHAPE_AXES) {
		_marker_material->set_albedo(_marker_color);
	} else if (is_inside_tree()) {
		_rebuild();
	}
}
Color MultiNodeMarker::get_marker_color() const { return _marker_color; }

void MultiNodeMarker::set_editor_only(bool p_editor_only) { _editor_only = p_editor_only; if (is_inside_tree()) _rebuild(); }
bool MultiNodeMarker::get_editor_only() const { return _editor_only; }

void MultiNodeMarker::set_always_on_top(bool p_always_on_top) { _always_on_top = p_always_on_top; if (is_inside_tree()) _rebuild(); }
bool MultiNodeMarker::get_always_on_top() const { return _always_on_top; }

// ---------------------------------------------------------------------------
// Mesh generation
// ---------------------------------------------------------------------------

Ref<ArrayMesh> MultiNodeMarker::_generate_mesh() const {
	const float s = _marker_size;
	PackedVector3Array verts;
	PackedColorArray  vert_colors;

	switch (_shape) {

		case SHAPE_CROSS: {
			// Three axis-aligned line pairs.
			verts.resize(6);
			verts[0] = Vector3(-s,  0,  0); verts[1] = Vector3( s,  0,  0);
			verts[2] = Vector3( 0, -s,  0); verts[3] = Vector3( 0,  s,  0);
			verts[4] = Vector3( 0,  0, -s); verts[5] = Vector3( 0,  0,  s);
		} break;

		case SHAPE_DIAMOND: {
			// Wireframe octahedron: 6 vertices, 12 edges.
			const Vector3 top(0, s, 0), btm(0, -s, 0);
			const Vector3 px(s, 0, 0), nx(-s, 0, 0), pz(0, 0, s), nz(0, 0, -s);
			// 4 upper edges
			verts.push_back(top); verts.push_back(px);
			verts.push_back(top); verts.push_back(nx);
			verts.push_back(top); verts.push_back(pz);
			verts.push_back(top); verts.push_back(nz);
			// 4 lower edges
			verts.push_back(btm); verts.push_back(px);
			verts.push_back(btm); verts.push_back(nx);
			verts.push_back(btm); verts.push_back(pz);
			verts.push_back(btm); verts.push_back(nz);
			// 4 equatorial edges
			verts.push_back(px); verts.push_back(pz);
			verts.push_back(pz); verts.push_back(nx);
			verts.push_back(nx); verts.push_back(nz);
			verts.push_back(nz); verts.push_back(px);
		} break;

		case SHAPE_SPHERE: {
			// Three orthogonal circles (XZ, XY, YZ), 24 segments each.
			const int N = 24;
			for (int i = 0; i < N; i++) {
				float a0 = MN_TAU * i / N;
				float a1 = MN_TAU * (i + 1) / N;
				float c0 = s * std::cos(a0), s0 = s * std::sin(a0);
				float c1 = s * std::cos(a1), s1 = s * std::sin(a1);
				// XZ (horizontal belt)
				verts.push_back(Vector3(c0, 0, s0)); verts.push_back(Vector3(c1, 0, s1));
				// XY (vertical, facing Z)
				verts.push_back(Vector3(c0, s0, 0)); verts.push_back(Vector3(c1, s1, 0));
				// YZ (vertical, facing X)
				verts.push_back(Vector3(0, c0, s0)); verts.push_back(Vector3(0, c1, s1));
			}
		} break;

		case SHAPE_AXES: {
			// RGB XYZ arrows with vertex colors. Material uses FLAG_ALBEDO_FROM_VERTEX_COLOR.
			// Each axis: shaft (origin → tip) + 4 arrowhead lines.
			const float tip   = s;
			const float base  = s * 0.7f;  // arrowhead start fraction
			const float fan   = s * 0.18f; // arrowhead lateral width

			struct AxisDef {
				Vector3 dir;     // unit vector × s
				Vector3 perp_a;  // first perpendicular × fan
				Vector3 perp_b;  // second perpendicular × fan
				Color   color;
			};

			const AxisDef axes[3] = {
				{ Vector3(tip, 0,   0  ), Vector3(0,   fan, 0  ), Vector3(0,   0,   fan), Color(1.0f, 0.2f, 0.2f, 1) }, // +X red
				{ Vector3(0,   tip, 0  ), Vector3(fan, 0,   0  ), Vector3(0,   0,   fan), Color(0.2f, 1.0f, 0.2f, 1) }, // +Y green
				{ Vector3(0,   0,   tip), Vector3(fan, 0,   0  ), Vector3(0,   fan, 0  ), Color(0.2f, 0.2f, 1.0f, 1) }, // +Z blue
			};

			for (int a = 0; a < 3; a++) {
				const Vector3 &tip_pos  = axes[a].dir;
				const Vector3  base_pos = tip_pos * (base / tip);
				const Color   &c        = axes[a].color;

				// Shaft
				verts.push_back(Vector3()); vert_colors.push_back(c);
				verts.push_back(tip_pos);   vert_colors.push_back(c);
				// 4 arrowhead spokes
				verts.push_back(base_pos + axes[a].perp_a); vert_colors.push_back(c);
				verts.push_back(tip_pos);                   vert_colors.push_back(c);
				verts.push_back(base_pos - axes[a].perp_a); vert_colors.push_back(c);
				verts.push_back(tip_pos);                   vert_colors.push_back(c);
				verts.push_back(base_pos + axes[a].perp_b); vert_colors.push_back(c);
				verts.push_back(tip_pos);                   vert_colors.push_back(c);
				verts.push_back(base_pos - axes[a].perp_b); vert_colors.push_back(c);
				verts.push_back(tip_pos);                   vert_colors.push_back(c);
			}
		} break;
	}

	if (verts.size() == 0) {
		return Ref<ArrayMesh>();
	}

	Array arrays;
	arrays.resize(Mesh::ARRAY_MAX);
	arrays[Mesh::ARRAY_VERTEX] = verts;
	if (_shape == SHAPE_AXES && vert_colors.size() == verts.size()) {
		arrays[Mesh::ARRAY_COLOR] = vert_colors;
	}

	Ref<ArrayMesh> mesh;
	mesh.instantiate();
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, arrays);
	return mesh;
}

Ref<StandardMaterial3D> MultiNodeMarker::_create_material() const {
	Ref<StandardMaterial3D> mat;
	mat.instantiate();
	mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	mat->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, _always_on_top);

	if (_shape == SHAPE_AXES) {
		// Vertex colors carry the RGB per-axis tint. Material color tints all axes equally.
		mat->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		mat->set_albedo(Color(1, 1, 1, 1));
	} else {
		mat->set_albedo(_marker_color);
	}
	return mat;
}

// ---------------------------------------------------------------------------
// RID management
// ---------------------------------------------------------------------------

void MultiNodeMarker::_rebuild() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs || !is_inside_tree()) return;

	// editor_only: do not create any rendering objects at runtime.
	if (_editor_only && !Engine::get_singleton()->is_editor_hint()) {
		_cleanup_rids();
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

	_marker_mesh     = _generate_mesh();
	_marker_material = _create_material();

	rs->multimesh_set_mesh(_multimesh, _marker_mesh.is_valid() ? _marker_mesh->get_rid() : RID());
	rs->instance_geometry_set_material_override(_instance, _marker_material.is_valid() ? _marker_material->get_rid() : RID());

	_current_count = 0;
	_sync_transforms();
}

void MultiNodeMarker::_cleanup_rids() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs) return;
	if (_instance.is_valid()) { rs->free_rid(_instance); _instance = RID(); }
	if (_multimesh.is_valid()) { rs->free_rid(_multimesh); _multimesh = RID(); }
	_marker_mesh     = Ref<ArrayMesh>();
	_marker_material = Ref<StandardMaterial3D>();
	_current_count = 0;
}

// ---------------------------------------------------------------------------
// Transform sync — mirrors MultiNodeMesh pattern, no per-instance color
// ---------------------------------------------------------------------------

void MultiNodeMarker::_sync_transforms() {
	if (!_parent || !_multimesh.is_valid() || !is_inside_tree()) return;

	RenderingServer *rs = RenderingServer::get_singleton();
	int count = _parent->get_instance_count();

	if (count == 0) {
		if (_current_count > 0) {
			rs->multimesh_allocate_data(_multimesh, 0, RenderingServer::MULTIMESH_TRANSFORM_3D, false, false);
			_current_count = 0;
		}
		return;
	}

	bool resized = (_current_count != count);
	if (resized) {
		rs->multimesh_allocate_data(_multimesh, count, RenderingServer::MULTIMESH_TRANSFORM_3D, false, false);
		_current_count = count;
	}

	int dirty_count = _parent->get_dirty_count();
	if (dirty_count == 0 && !resized && !_any_active_changed) return;

	const Vector<Transform3D> &transforms = _parent->get_transforms_internal();
	const PackedByteArray     &dirty      = _parent->get_dirty_flags_internal();
	const uint8_t *active_ptr = _active.ptr();

	if (resized) {
		// Full upload.
		PackedFloat32Array buffer;
		buffer.resize(count * 12);
		float *buf = buffer.ptrw();
		for (int i = 0; i < count; i++) {
			int off = i * 12;
			Transform3D xform;
			if (active_ptr && i < _active.size() && !active_ptr[i]) {
				xform = HIDDEN_TRANSFORM;
			} else {
				xform = compute_instance_transform(transforms[i]);
			}
			buf[off+0]  = xform.basis[0][0]; buf[off+1]  = xform.basis[0][1]; buf[off+2]  = xform.basis[0][2]; buf[off+3]  = xform.origin.x;
			buf[off+4]  = xform.basis[1][0]; buf[off+5]  = xform.basis[1][1]; buf[off+6]  = xform.basis[1][2]; buf[off+7]  = xform.origin.y;
			buf[off+8]  = xform.basis[2][0]; buf[off+9]  = xform.basis[2][1]; buf[off+10] = xform.basis[2][2]; buf[off+11] = xform.origin.z;
		}
		rs->multimesh_set_buffer(_multimesh, buffer);
	} else {
		// Sparse update — dirty instances only.
		for (int i = 0; i < count; i++) {
			if (dirty[i]) {
				bool is_active = !active_ptr || (i < _active.size() && active_ptr[i]);
				rs->multimesh_instance_set_transform(_multimesh, i,
					is_active ? compute_instance_transform(transforms[i]) : HIDDEN_TRANSFORM);
			}
		}
		if (_any_active_changed) {
			for (int i = 0; i < count; i++) {
				if (!dirty[i] && did_instance_active_change(i)) {
					bool is_active = !active_ptr || (i < _active.size() && active_ptr[i]);
					rs->multimesh_instance_set_transform(_multimesh, i,
						is_active ? compute_instance_transform(transforms[i]) : HIDDEN_TRANSFORM);
				}
			}
		}
	}

	rs->multimesh_set_visible_instances(_multimesh, -1);
}
