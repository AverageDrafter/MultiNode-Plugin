#include "multi_node_mesh.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>

using namespace godot;

// Hidden transform for inactive instances — zero scale collapses geometry.
static const Transform3D HIDDEN_TRANSFORM = Transform3D(Basis().scaled(Vector3(0, 0, 0)), Vector3(0, -10000, 0));

MultiNodeMesh::MultiNodeMesh() {}

MultiNodeMesh::~MultiNodeMesh() {
	_cleanup_rids();
}

void MultiNodeMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &MultiNodeMesh::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh"), &MultiNodeMesh::get_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_mesh", "get_mesh");

	ClassDB::bind_method(D_METHOD("set_material_override", "material"), &MultiNodeMesh::set_material_override);
	ClassDB::bind_method(D_METHOD("get_material_override"), &MultiNodeMesh::get_material_override);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_material_override", "get_material_override");

	ClassDB::bind_method(D_METHOD("set_use_instance_colors", "enable"), &MultiNodeMesh::set_use_instance_colors);
	ClassDB::bind_method(D_METHOD("get_use_instance_colors"), &MultiNodeMesh::get_use_instance_colors);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_instance_colors"), "set_use_instance_colors", "get_use_instance_colors");
}

// ---------------------------------------------------------------------------
// Notification — mesh-specific (RenderingServer instance management)
// ---------------------------------------------------------------------------

void MultiNodeMesh::_notification(int p_what) {
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
// MultiNodeSub virtual overrides
// ---------------------------------------------------------------------------

void MultiNodeMesh::_on_sub_entered() {
	set_notify_transform(true);
	_rebuild();
}

void MultiNodeMesh::_on_sub_exiting() {
	_cleanup_rids();
}

void MultiNodeMesh::_on_parent_sync() {
	_sync_transforms();
}

void MultiNodeMesh::_on_active_changed() {
	if (is_inside_tree()) {
		_rebuild();
	}
}

RID MultiNodeMesh::_get_visual_instance_rid() const {
	return _instance;
}

// ---------------------------------------------------------------------------
// Property setters / getters
// ---------------------------------------------------------------------------

void MultiNodeMesh::set_mesh(const Ref<Mesh> &p_mesh) {
	_mesh = p_mesh;
	if (is_inside_tree()) {
		_rebuild();
	}
}

Ref<Mesh> MultiNodeMesh::get_mesh() const {
	return _mesh;
}

void MultiNodeMesh::set_material_override(const Ref<Material> &p_material) {
	_material_override = p_material;
	if (_instance.is_valid()) {
		RenderingServer::get_singleton()->instance_geometry_set_material_override(
				_instance, p_material.is_valid() ? p_material->get_rid() : RID());
	}
}

Ref<Material> MultiNodeMesh::get_material_override() const {
	return _material_override;
}

void MultiNodeMesh::set_use_instance_colors(bool p_enable) {
	_use_instance_colors = p_enable;
	if (is_inside_tree()) {
		_rebuild();
	}
}

bool MultiNodeMesh::get_use_instance_colors() const {
	return _use_instance_colors;
}

// ---------------------------------------------------------------------------
// Color virtual overrides — push to MultiMesh backend
// ---------------------------------------------------------------------------

void MultiNodeMesh::_on_instance_color_changed(int p_index, const Color &p_color) {
	if (_multimesh.is_valid() && _use_instance_colors && p_index < _current_count) {
		RenderingServer::get_singleton()->multimesh_instance_set_color(_multimesh, p_index, p_color);
	}
}

void MultiNodeMesh::_on_colors_changed() {
	_sync_colors();
}

// ---------------------------------------------------------------------------
// Internal — RID management
// ---------------------------------------------------------------------------

void MultiNodeMesh::_rebuild() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (!rs || !is_inside_tree()) {
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

	rs->multimesh_set_mesh(_multimesh, _mesh.is_valid() ? _mesh->get_rid() : RID());
	rs->instance_geometry_set_material_override(
			_instance, _material_override.is_valid() ? _material_override->get_rid() : RID());

	_current_count = 0;
	_sync_transforms();
}

void MultiNodeMesh::_cleanup_rids() {
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
// Transform sync
// ---------------------------------------------------------------------------

void MultiNodeMesh::_sync_transforms() {
	if (!_parent || !_multimesh.is_valid() || !is_inside_tree() || !_mesh.is_valid()) {
		return;
	}

	RenderingServer *rs = RenderingServer::get_singleton();
	int count = _parent->get_instance_count();

	if (count == 0) {
		if (_current_count > 0) {
			rs->multimesh_allocate_data(_multimesh, 0,
					RenderingServer::MULTIMESH_TRANSFORM_3D, _use_instance_colors, false);
			_current_count = 0;
		}
		return;
	}

	bool resized = (_current_count != count);
	if (resized) {
		rs->multimesh_allocate_data(_multimesh, count,
				RenderingServer::MULTIMESH_TRANSFORM_3D, _use_instance_colors, false);
		_current_count = count;

		// Resize color array to match (base class fills new entries with white).
		resize_colors(count);
	}

	int dirty_count = _parent->get_dirty_count();
	if (dirty_count == 0 && !resized && !_any_active_changed) {
		return;
	}

	const Vector<Transform3D> &transforms = _parent->get_transforms_internal();
	const PackedByteArray &dirty = _parent->get_dirty_flags_internal();
	const uint8_t *active_ptr = _active.ptr();

	if (resized) {
		// Full upload via bulk buffer — one RenderingServer call for all instances.
		// Buffer layout per instance: 12 floats (Transform3D) + 4 floats (Color, if enabled).
		int stride = 12 + (_use_instance_colors ? 4 : 0);
		PackedFloat32Array buffer;
		buffer.resize(count * stride);
		float *buf = buffer.ptrw();

		for (int i = 0; i < count; i++) {
			int off = i * stride;
			Transform3D xform;
			if (active_ptr && i < _active.size() && !active_ptr[i]) {
				xform = HIDDEN_TRANSFORM;
			} else {
				xform = compute_instance_transform(transforms[i]);
			}
			// Row-major 4x3 matrix (Godot MultiMesh buffer format).
			buf[off + 0] = xform.basis[0][0]; buf[off + 1] = xform.basis[0][1]; buf[off + 2] = xform.basis[0][2]; buf[off + 3] = xform.origin.x;
			buf[off + 4] = xform.basis[1][0]; buf[off + 5] = xform.basis[1][1]; buf[off + 6] = xform.basis[1][2]; buf[off + 7] = xform.origin.y;
			buf[off + 8] = xform.basis[2][0]; buf[off + 9] = xform.basis[2][1]; buf[off + 10] = xform.basis[2][2]; buf[off + 11] = xform.origin.z;

			if (_use_instance_colors) {
				Color tc = get_tinted_color(i);
				buf[off + 12] = tc.r; buf[off + 13] = tc.g;
				buf[off + 14] = tc.b; buf[off + 15] = tc.a;
			}
		}
		rs->multimesh_set_buffer(_multimesh, buffer);
	} else {
		// Sparse — only dirty instances.
		const Vector<int> &dirty_indices = _parent->get_dirty_indices();
		if (dirty_indices.size() > 0 && dirty_count < count) {
			// Use sparse index list for small dirty counts.
			for (int d = 0; d < dirty_indices.size(); d++) {
				int i = dirty_indices[d];
				if (active_ptr && i < _active.size() && !active_ptr[i]) {
					rs->multimesh_instance_set_transform(_multimesh, i, HIDDEN_TRANSFORM);
				} else {
					rs->multimesh_instance_set_transform(_multimesh, i, compute_instance_transform(transforms[i]));
				}
			}
		} else {
			// Full scan fallback (mark_all_dirty was called).
			for (int i = 0; i < count; i++) {
				if (dirty[i]) {
					if (active_ptr && i < _active.size() && !active_ptr[i]) {
						rs->multimesh_instance_set_transform(_multimesh, i, HIDDEN_TRANSFORM);
					} else {
						rs->multimesh_instance_set_transform(_multimesh, i, compute_instance_transform(transforms[i]));
					}
				}
			}
		}
		// Handle runtime active flag changes on non-dirty instances.
		if (_any_active_changed) {
			for (int i = 0; i < count; i++) {
				if (!dirty[i] && did_instance_active_change(i)) {
					if (active_ptr && i < _active.size() && !active_ptr[i]) {
						rs->multimesh_instance_set_transform(_multimesh, i, HIDDEN_TRANSFORM);
					} else {
						rs->multimesh_instance_set_transform(_multimesh, i, compute_instance_transform(transforms[i]));
					}
				}
			}
		}
	}

	rs->multimesh_set_visible_instances(_multimesh, -1);
}

// ---------------------------------------------------------------------------
// Color sync
// ---------------------------------------------------------------------------

void MultiNodeMesh::_sync_colors() {
	if (!_multimesh.is_valid() || !_use_instance_colors || !is_inside_tree()) {
		return;
	}
	// Re-upload the full buffer with updated colors.
	// This is cheaper than N individual set_color calls when set_all_colors is used.
	RenderingServer *rs = RenderingServer::get_singleton();
	int count = MIN(_colors.size(), _current_count);
	if (count == 0) return;

	PackedFloat32Array buffer = rs->multimesh_get_buffer(_multimesh);
	if (buffer.size() == 0) return;

	int stride = 12 + 4; // Transform + Color (colors are enabled if we're here).
	float *buf = buffer.ptrw();

	for (int i = 0; i < count; i++) {
		int off = i * stride + 12;
		Color tc = get_tinted_color(i);
		buf[off + 0] = tc.r; buf[off + 1] = tc.g;
		buf[off + 2] = tc.b; buf[off + 3] = tc.a;
	}
	rs->multimesh_set_buffer(_multimesh, buffer);
}
