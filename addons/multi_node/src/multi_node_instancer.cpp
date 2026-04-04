#include "multi_node_instancer.h"
#include "multi_node.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

using namespace godot;

MultiNodeInstancer::MultiNodeInstancer() {}
MultiNodeInstancer::~MultiNodeInstancer() {}

// ===========================================================================
// _bind_methods
// ===========================================================================

void MultiNodeInstancer::_bind_methods() {
	ADD_GROUP("Instancer", "gen_");

	// --- Mode / Count / Seed ---
	ClassDB::bind_method(D_METHOD("set_generator_mode", "mode"), &MultiNodeInstancer::set_generator_mode);
	ClassDB::bind_method(D_METHOD("get_generator_mode"), &MultiNodeInstancer::get_generator_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "gen_mode", PROPERTY_HINT_ENUM, "Array,Mesh Fill,Manual"), "set_generator_mode", "get_generator_mode");

	ClassDB::bind_method(D_METHOD("set_generator_count", "count"), &MultiNodeInstancer::set_generator_count);
	ClassDB::bind_method(D_METHOD("get_generator_count"), &MultiNodeInstancer::get_generator_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "gen_count", PROPERTY_HINT_RANGE, "1,1000000,1,or_greater"), "set_generator_count", "get_generator_count");

	ClassDB::bind_method(D_METHOD("set_generator_seed", "seed"), &MultiNodeInstancer::set_generator_seed);
	ClassDB::bind_method(D_METHOD("get_generator_seed"), &MultiNodeInstancer::get_generator_seed);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "gen_seed", PROPERTY_HINT_RANGE, "0,999999,1"), "set_generator_seed", "get_generator_seed");

	// --- Animate (script-only — not in inspector) ---
	ClassDB::bind_method(D_METHOD("set_animate", "animate"), &MultiNodeInstancer::set_animate);
	ClassDB::bind_method(D_METHOD("get_animate"), &MultiNodeInstancer::get_animate);

	// --- Array ---
	ADD_SUBGROUP("Array", "gen_array_");

	ClassDB::bind_method(D_METHOD("set_array_origin", "origin"), &MultiNodeInstancer::set_array_origin);
	ClassDB::bind_method(D_METHOD("get_array_origin"), &MultiNodeInstancer::get_array_origin);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gen_array_origin", PROPERTY_HINT_RANGE, "-100000,100000,0.01,suffix:m"), "set_array_origin", "get_array_origin");

	ClassDB::bind_method(D_METHOD("set_array_spacing", "spacing"), &MultiNodeInstancer::set_array_spacing);
	ClassDB::bind_method(D_METHOD("get_array_spacing"), &MultiNodeInstancer::get_array_spacing);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gen_array_spacing", PROPERTY_HINT_RANGE, "-1000,1000,0.01"), "set_array_spacing", "get_array_spacing");

	ClassDB::bind_method(D_METHOD("set_array_rows", "rows"), &MultiNodeInstancer::set_array_rows);
	ClassDB::bind_method(D_METHOD("get_array_rows"), &MultiNodeInstancer::get_array_rows);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "gen_array_rows", PROPERTY_HINT_RANGE, "-10000,10000,1"), "set_array_rows", "get_array_rows");

	ClassDB::bind_method(D_METHOD("set_array_cols", "cols"), &MultiNodeInstancer::set_array_cols);
	ClassDB::bind_method(D_METHOD("get_array_cols"), &MultiNodeInstancer::get_array_cols);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "gen_array_cols", PROPERTY_HINT_RANGE, "-10000,10000,1"), "set_array_cols", "get_array_cols");

	ClassDB::bind_method(D_METHOD("set_array_flip_layers", "flip"), &MultiNodeInstancer::set_array_flip_layers);
	ClassDB::bind_method(D_METHOD("get_array_flip_layers"), &MultiNodeInstancer::get_array_flip_layers);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "gen_array_flip_layers"), "set_array_flip_layers", "get_array_flip_layers");

	ClassDB::bind_method(D_METHOD("set_array_follow_rotation", "follow"), &MultiNodeInstancer::set_array_follow_rotation);
	ClassDB::bind_method(D_METHOD("get_array_follow_rotation"), &MultiNodeInstancer::get_array_follow_rotation);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "gen_array_follow_rotation"), "set_array_follow_rotation", "get_array_follow_rotation");

	ClassDB::bind_method(D_METHOD("set_array_rotation", "rotation"), &MultiNodeInstancer::set_array_rotation);
	ClassDB::bind_method(D_METHOD("get_array_rotation"), &MultiNodeInstancer::get_array_rotation);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gen_array_rotation", PROPERTY_HINT_RANGE, "-360,360,0.1,radians_as_degrees"), "set_array_rotation", "get_array_rotation");

	ClassDB::bind_method(D_METHOD("set_array_scale_step", "scale"), &MultiNodeInstancer::set_array_scale_step);
	ClassDB::bind_method(D_METHOD("get_array_scale_step"), &MultiNodeInstancer::get_array_scale_step);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gen_array_scale_step", PROPERTY_HINT_RANGE, "0.01,10.0,0.01"), "set_array_scale_step", "get_array_scale_step");

	// --- Mesh Fill ---
	ADD_SUBGROUP("Mesh Fill", "gen_fill_");

	ClassDB::bind_method(D_METHOD("set_fill_mesh", "mesh"), &MultiNodeInstancer::set_fill_mesh);
	ClassDB::bind_method(D_METHOD("get_fill_mesh"), &MultiNodeInstancer::get_fill_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "gen_fill_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_fill_mesh", "get_fill_mesh");

	ClassDB::bind_method(D_METHOD("set_fill_mode", "mode"), &MultiNodeInstancer::set_fill_mode);
	ClassDB::bind_method(D_METHOD("get_fill_mode"), &MultiNodeInstancer::get_fill_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "gen_fill_mode", PROPERTY_HINT_ENUM, "Cover Surface,Fill Volume"), "set_fill_mode", "get_fill_mode");

	ClassDB::bind_method(D_METHOD("set_fill_align", "align"), &MultiNodeInstancer::set_fill_align);
	ClassDB::bind_method(D_METHOD("get_fill_align"), &MultiNodeInstancer::get_fill_align);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "gen_fill_align"), "set_fill_align", "get_fill_align");

	ClassDB::bind_method(D_METHOD("set_fill_min_distance", "distance"), &MultiNodeInstancer::set_fill_min_distance);
	ClassDB::bind_method(D_METHOD("get_fill_min_distance"), &MultiNodeInstancer::get_fill_min_distance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gen_fill_min_distance", PROPERTY_HINT_RANGE, "0.0,100.0,0.01"), "set_fill_min_distance", "get_fill_min_distance");

	// --- Scatter ---
	ADD_SUBGROUP("Scatter", "gen_scatter_");

	ClassDB::bind_method(D_METHOD("set_scatter_enabled", "enable"), &MultiNodeInstancer::set_scatter_enabled);
	ClassDB::bind_method(D_METHOD("get_scatter_enabled"), &MultiNodeInstancer::get_scatter_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "gen_scatter_enabled"), "set_scatter_enabled", "get_scatter_enabled");

	ClassDB::bind_method(D_METHOD("set_scatter_jitter", "jitter"), &MultiNodeInstancer::set_scatter_jitter);
	ClassDB::bind_method(D_METHOD("get_scatter_jitter"), &MultiNodeInstancer::get_scatter_jitter);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gen_scatter_jitter", PROPERTY_HINT_RANGE, "-1000,1000,0.01"), "set_scatter_jitter", "get_scatter_jitter");

	ClassDB::bind_method(D_METHOD("set_scatter_random_rotation", "enable"), &MultiNodeInstancer::set_scatter_random_rotation);
	ClassDB::bind_method(D_METHOD("get_scatter_random_rotation"), &MultiNodeInstancer::get_scatter_random_rotation);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "gen_scatter_random_rotation"), "set_scatter_random_rotation", "get_scatter_random_rotation");

	ClassDB::bind_method(D_METHOD("set_scatter_scale_min", "min"), &MultiNodeInstancer::set_scatter_scale_min);
	ClassDB::bind_method(D_METHOD("get_scatter_scale_min"), &MultiNodeInstancer::get_scatter_scale_min);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gen_scatter_scale_min", PROPERTY_HINT_RANGE, "0.01,100.0,0.01"), "set_scatter_scale_min", "get_scatter_scale_min");

	ClassDB::bind_method(D_METHOD("set_scatter_scale_max", "max"), &MultiNodeInstancer::set_scatter_scale_max);
	ClassDB::bind_method(D_METHOD("get_scatter_scale_max"), &MultiNodeInstancer::get_scatter_scale_max);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gen_scatter_scale_max", PROPERTY_HINT_RANGE, "0.01,100.0,0.01"), "set_scatter_scale_max", "get_scatter_scale_max");

	// --- Manual Editor ---
	ADD_SUBGROUP("Manual", "gen_manual_");

	ClassDB::bind_method(D_METHOD("set_manual_index", "index"), &MultiNodeInstancer::set_manual_index);
	ClassDB::bind_method(D_METHOD("get_manual_index"), &MultiNodeInstancer::get_manual_index);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "gen_manual_index", PROPERTY_HINT_RANGE, "0,999999,1"), "set_manual_index", "get_manual_index");

	ClassDB::bind_method(D_METHOD("set_manual_position", "position"), &MultiNodeInstancer::set_manual_position);
	ClassDB::bind_method(D_METHOD("get_manual_position"), &MultiNodeInstancer::get_manual_position);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gen_manual_position", PROPERTY_HINT_RANGE, "-100000,100000,0.01,suffix:m"), "set_manual_position", "get_manual_position");

	ClassDB::bind_method(D_METHOD("set_manual_rotation", "rotation"), &MultiNodeInstancer::set_manual_rotation);
	ClassDB::bind_method(D_METHOD("get_manual_rotation"), &MultiNodeInstancer::get_manual_rotation);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gen_manual_rotation", PROPERTY_HINT_RANGE, "-360,360,0.1,radians_as_degrees"), "set_manual_rotation", "get_manual_rotation");

	ClassDB::bind_method(D_METHOD("set_manual_scale", "scale"), &MultiNodeInstancer::set_manual_scale);
	ClassDB::bind_method(D_METHOD("get_manual_scale"), &MultiNodeInstancer::get_manual_scale);
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gen_manual_scale", PROPERTY_HINT_RANGE, "0.001,1000,0.01"), "set_manual_scale", "get_manual_scale");

	ClassDB::bind_method(D_METHOD("manual_insert"), &MultiNodeInstancer::manual_insert);
	ClassDB::bind_method(D_METHOD("manual_delete"), &MultiNodeInstancer::manual_delete);

	ClassDB::bind_method(D_METHOD("set_editor_selected_instance", "index"), &MultiNodeInstancer::set_editor_selected_instance);
	ClassDB::bind_method(D_METHOD("get_editor_selected_instance"), &MultiNodeInstancer::get_editor_selected_instance);

	// --- Actions ---
	ADD_SUBGROUP("Actions", "gen_btn_");

	ClassDB::bind_method(D_METHOD("generate"), &MultiNodeInstancer::generate);
	ClassDB::bind_method(D_METHOD("clear_and_notify"), &MultiNodeInstancer::clear_and_notify);
	ClassDB::bind_method(D_METHOD("save_instance_data"), &MultiNodeInstancer::save_instance_data);
	ClassDB::bind_method(D_METHOD("get_btn_regenerate"), &MultiNodeInstancer::get_btn_regenerate);
	ClassDB::bind_method(D_METHOD("get_btn_clear"), &MultiNodeInstancer::get_btn_clear);
	ClassDB::bind_method(D_METHOD("get_btn_save_data"), &MultiNodeInstancer::get_btn_save_data);

	ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "gen_btn_regenerate", PROPERTY_HINT_TOOL_BUTTON, "Regenerate", PROPERTY_USAGE_EDITOR), "", "get_btn_regenerate");
	ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "gen_btn_clear", PROPERTY_HINT_TOOL_BUTTON, "Clear", PROPERTY_USAGE_EDITOR), "", "get_btn_clear");
	ADD_PROPERTY(PropertyInfo(Variant::CALLABLE, "gen_btn_save_data", PROPERTY_HINT_TOOL_BUTTON, "Save Instance Data", PROPERTY_USAGE_EDITOR), "", "get_btn_save_data");

	ClassDB::bind_method(D_METHOD("set_instance_data_resource", "resource"), &MultiNodeInstancer::set_instance_data_resource);
	ClassDB::bind_method(D_METHOD("get_instance_data_resource"), &MultiNodeInstancer::get_instance_data_resource);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "gen_instance_data", PROPERTY_HINT_RESOURCE_TYPE, "MultiNodeInstanceData"), "set_instance_data_resource", "get_instance_data_resource");

	// --- Enums ---
	BIND_ENUM_CONSTANT(GEN_ARRAY);
	BIND_ENUM_CONSTANT(GEN_MESH_FILL);
	BIND_ENUM_CONSTANT(GEN_MANUAL);
	BIND_ENUM_CONSTANT(FILL_SURFACE);
	BIND_ENUM_CONSTANT(FILL_VOLUME);
}

// ===========================================================================
// Notification
// ===========================================================================

void MultiNodeInstancer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (_animate) {
				set_process(true);
			}
		} break;

		case NOTIFICATION_PROCESS: {
			if (_animate && _parent) {
				generate();
			}
		} break;
	}
}

// ===========================================================================
// Property validation — conditional visibility
// ===========================================================================

void MultiNodeInstancer::_validate_property(PropertyInfo &p_property) const {
	String name = p_property.name;

	if (name.begins_with("gen_array_") && _generator_mode != GEN_ARRAY) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE;
	}
	if (name.begins_with("gen_fill_") && _generator_mode != GEN_MESH_FILL) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE;
	}
	if (name.begins_with("gen_manual_") && _generator_mode != GEN_MANUAL) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE;
	}
	if ((name == "gen_count" || name == "gen_seed") && _generator_mode == GEN_MANUAL) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE;
	}
	if (name.begins_with("gen_scatter_") && _generator_mode == GEN_MANUAL) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE;
	}
	if ((name == "gen_scatter_jitter" || name == "gen_scatter_random_rotation"
			|| name == "gen_scatter_scale_min" || name == "gen_scatter_scale_max")
			&& !_scatter_enabled) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE;
	}
	if ((name == "gen_btn_regenerate" || name == "gen_btn_clear") && _generator_mode == GEN_MANUAL) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR;
	}
}

// ===========================================================================
// Sub-node callback — suppress feedback during generation
// ===========================================================================

void MultiNodeInstancer::_on_parent_sync() {
	// Nothing to sync — the instancer doesn't track local copies.
	// The _generating flag in the base class signal handler prevents
	// redundant active-array recomputation during our own writes.
}

// ===========================================================================
// Getters / setters — all call _editor_generate() for live preview
// ===========================================================================

void MultiNodeInstancer::_editor_generate() {
	if (Engine::get_singleton()->is_editor_hint() && is_inside_tree() && _parent && _generator_mode != GEN_MANUAL) {
		generate();
	}
}

void MultiNodeInstancer::set_generator_mode(int p_mode) { _generator_mode = p_mode; notify_property_list_changed(); _editor_generate(); }
int MultiNodeInstancer::get_generator_mode() const { return _generator_mode; }
void MultiNodeInstancer::set_generator_count(int p_count) { _generator_count = MAX(1, p_count); _editor_generate(); }
int MultiNodeInstancer::get_generator_count() const { return _generator_count; }
void MultiNodeInstancer::set_generator_seed(int p_seed) { _generator_seed = p_seed; _editor_generate(); }
int MultiNodeInstancer::get_generator_seed() const { return _generator_seed; }

void MultiNodeInstancer::set_animate(bool p_animate) {
	_animate = p_animate;
	if (_animate) {
		set_process(true);
	} else if (!get_compute_enabled()) {
		set_process(false);
	}
}
bool MultiNodeInstancer::get_animate() const { return _animate; }

void MultiNodeInstancer::set_array_origin(const Vector3 &p_origin) { _array_origin = p_origin; _editor_generate(); }
Vector3 MultiNodeInstancer::get_array_origin() const { return _array_origin; }
void MultiNodeInstancer::set_array_spacing(const Vector3 &p_spacing) { _array_spacing = p_spacing; _editor_generate(); }
Vector3 MultiNodeInstancer::get_array_spacing() const { return _array_spacing; }
void MultiNodeInstancer::set_array_rows(int p_rows) { _array_rows = p_rows; _editor_generate(); }
int MultiNodeInstancer::get_array_rows() const { return _array_rows; }
void MultiNodeInstancer::set_array_cols(int p_cols) { _array_cols = p_cols; _editor_generate(); }
int MultiNodeInstancer::get_array_cols() const { return _array_cols; }
void MultiNodeInstancer::set_array_flip_layers(bool p_flip) { _array_flip_layers = p_flip; _editor_generate(); }
bool MultiNodeInstancer::get_array_flip_layers() const { return _array_flip_layers; }
void MultiNodeInstancer::set_array_follow_rotation(bool p_follow) { _array_follow_rotation = p_follow; _editor_generate(); }
bool MultiNodeInstancer::get_array_follow_rotation() const { return _array_follow_rotation; }
void MultiNodeInstancer::set_array_rotation(const Vector3 &p_rot) { _array_rotation = p_rot; _editor_generate(); }
Vector3 MultiNodeInstancer::get_array_rotation() const { return _array_rotation; }
void MultiNodeInstancer::set_array_scale_step(float p_scale) { _array_scale_step = p_scale; _editor_generate(); }
float MultiNodeInstancer::get_array_scale_step() const { return _array_scale_step; }

void MultiNodeInstancer::set_fill_mesh(const Ref<Mesh> &p_mesh) { _fill_mesh = p_mesh; _editor_generate(); }
Ref<Mesh> MultiNodeInstancer::get_fill_mesh() const { return _fill_mesh; }
void MultiNodeInstancer::set_fill_mode(int p_mode) { _fill_mode = p_mode; _editor_generate(); }
int MultiNodeInstancer::get_fill_mode() const { return _fill_mode; }
void MultiNodeInstancer::set_fill_align(bool p_align) { _fill_align = p_align; _editor_generate(); }
bool MultiNodeInstancer::get_fill_align() const { return _fill_align; }
void MultiNodeInstancer::set_fill_min_distance(float p_dist) { _fill_min_distance = MAX(0.0f, p_dist); _editor_generate(); }
float MultiNodeInstancer::get_fill_min_distance() const { return _fill_min_distance; }

void MultiNodeInstancer::set_scatter_enabled(bool p_enable) { _scatter_enabled = p_enable; notify_property_list_changed(); _editor_generate(); }
bool MultiNodeInstancer::get_scatter_enabled() const { return _scatter_enabled; }
void MultiNodeInstancer::set_scatter_jitter(const Vector3 &p_jitter) { _scatter_jitter = p_jitter; _editor_generate(); }
Vector3 MultiNodeInstancer::get_scatter_jitter() const { return _scatter_jitter; }
void MultiNodeInstancer::set_scatter_random_rotation(bool p_enable) { _scatter_random_rotation = p_enable; _editor_generate(); }
bool MultiNodeInstancer::get_scatter_random_rotation() const { return _scatter_random_rotation; }
void MultiNodeInstancer::set_scatter_scale_min(float p_min) { _scatter_scale_min = p_min; _editor_generate(); }
float MultiNodeInstancer::get_scatter_scale_min() const { return _scatter_scale_min; }
void MultiNodeInstancer::set_scatter_scale_max(float p_max) { _scatter_scale_max = p_max; _editor_generate(); }
float MultiNodeInstancer::get_scatter_scale_max() const { return _scatter_scale_max; }

// ===========================================================================
// Manual Editor
// ===========================================================================

void MultiNodeInstancer::_sync_manual_from_index() {
	_editor_selected_instance = _manual_index;
	_manual_updating = true;
	notify_property_list_changed();
	_manual_updating = false;
}

void MultiNodeInstancer::set_manual_index(int p_index) {
	if (!_parent || _parent->get_instance_count() == 0) { _manual_index = 0; return; }
	_manual_index = CLAMP(p_index, 0, _parent->get_instance_count() - 1);
	_sync_manual_from_index();
}
int MultiNodeInstancer::get_manual_index() const { return _manual_index; }

void MultiNodeInstancer::set_manual_position(const Vector3 &p_pos) {
	if (_manual_updating || !_parent) return;
	int idx = _manual_index;
	if (idx < 0 || idx >= _parent->get_instance_count()) return;
	Transform3D xform = _parent->get_instance_transform(idx);
	xform.origin = p_pos;
	_generating = true;
	_parent->begin_batch();
	_parent->set_instance_transform(idx, xform);
	_parent->end_batch();
	_generating = false;
}

Vector3 MultiNodeInstancer::get_manual_position() const {
	if (_parent && _manual_index >= 0 && _manual_index < _parent->get_instance_count()) {
		return _parent->get_instance_transform(_manual_index).origin;
	}
	return Vector3();
}

void MultiNodeInstancer::set_manual_rotation(const Vector3 &p_rot) {
	if (_manual_updating || !_parent) return;
	int idx = _manual_index;
	if (idx < 0 || idx >= _parent->get_instance_count()) return;
	Transform3D xform = _parent->get_instance_transform(idx);
	Vector3 scale = xform.basis.get_scale();
	xform.basis = Basis::from_euler(p_rot);
	xform.basis.scale_local(scale);
	_generating = true;
	_parent->begin_batch();
	_parent->set_instance_transform(idx, xform);
	_parent->end_batch();
	_generating = false;
}

Vector3 MultiNodeInstancer::get_manual_rotation() const {
	if (_parent && _manual_index >= 0 && _manual_index < _parent->get_instance_count()) {
		return _parent->get_instance_transform(_manual_index).basis.get_euler();
	}
	return Vector3();
}

void MultiNodeInstancer::set_manual_scale(const Vector3 &p_scale) {
	if (_manual_updating || !_parent) return;
	int idx = _manual_index;
	if (idx < 0 || idx >= _parent->get_instance_count()) return;
	Transform3D xform = _parent->get_instance_transform(idx);
	Vector3 euler = xform.basis.get_euler();
	xform.basis = Basis::from_euler(euler);
	xform.basis.scale_local(p_scale);
	_generating = true;
	_parent->begin_batch();
	_parent->set_instance_transform(idx, xform);
	_parent->end_batch();
	_generating = false;
}

Vector3 MultiNodeInstancer::get_manual_scale() const {
	if (_parent && _manual_index >= 0 && _manual_index < _parent->get_instance_count()) {
		return _parent->get_instance_transform(_manual_index).basis.get_scale();
	}
	return Vector3(1, 1, 1);
}

void MultiNodeInstancer::manual_insert() {
	if (!_parent) return;
	Vector3 pos = get_manual_position() + Vector3(1, 0, 0);
	_generating = true;
	_parent->add_instance(Transform3D(Basis(), pos));
	_generating = false;
	_manual_index = _parent->get_instance_count() - 1;
	_sync_manual_from_index();
}

void MultiNodeInstancer::manual_delete() {
	if (!_parent || _parent->get_instance_count() == 0) return;
	_generating = true;
	_parent->remove_instance(_manual_index);
	_generating = false;
	int count = _parent->get_instance_count();
	if (_manual_index >= count) _manual_index = count - 1;
	if (_manual_index < 0) _manual_index = 0;
	_sync_manual_from_index();
}

void MultiNodeInstancer::set_editor_selected_instance(int p_index) {
	_editor_selected_instance = p_index;
	if (p_index >= 0 && _generator_mode == GEN_MANUAL) {
		set_manual_index(p_index);
	}
}

int MultiNodeInstancer::get_editor_selected_instance() const {
	return _editor_selected_instance;
}

// ===========================================================================
// Instance Data resource
// ===========================================================================

void MultiNodeInstancer::set_instance_data_resource(const Ref<Resource> &p_res) {
	_instance_data_res = p_res;
	if (_instance_data_res.is_valid() && _parent && Engine::get_singleton()->is_editor_hint() && is_inside_tree()) {
		Array raw = _instance_data_res->call("get_transforms");
		if (raw.size() > 0) {
			_generating = true;
			_parent->set_instance_count(raw.size());
			_parent->begin_batch();
			for (int i = 0; i < raw.size(); i++) {
				_parent->set_instance_transform(i, raw[i]);
			}
			_parent->end_batch();
			_generating = false;
			UtilityFunctions::print("MultiNodeInstancer: Loaded ", raw.size(), " instances from resource.");
		}
	}
}

Ref<Resource> MultiNodeInstancer::get_instance_data_resource() const {
	return _instance_data_res;
}

void MultiNodeInstancer::save_instance_data() {
	if (!_parent || _parent->get_instance_count() == 0) return;

	if (!_instance_data_res.is_valid()) {
		_instance_data_res = ClassDB::instantiate("MultiNodeInstanceData");
		notify_property_list_changed();
	}

	int count = _parent->get_instance_count();
	Array transforms;
	transforms.resize(count);
	for (int i = 0; i < count; i++) {
		transforms[i] = _parent->get_instance_transform(i);
	}
	_instance_data_res->call("set_transforms", transforms);

	String path = _instance_data_res->get_path();
	if (!path.is_empty()) {
		ResourceSaver::get_singleton()->save(_instance_data_res, path);
		UtilityFunctions::print("MultiNodeInstancer: Saved ", count, " instances to ", path);
	} else {
		UtilityFunctions::print("MultiNodeInstancer: Saved ", count, " instances to resource (save scene to persist).");
	}
}

// ===========================================================================
// generate() — the core method
// ===========================================================================

void MultiNodeInstancer::generate() {
	if (!_parent || !_parent->get_use_transforms()) return;

	if (_generator_seed > 0) {
		UtilityFunctions::seed(_generator_seed);
	}

	Vector<Transform3D> result;
	if (_generator_mode == GEN_ARRAY) {
		_generate_array(result);
	} else if (_generator_mode == GEN_MESH_FILL) {
		_generate_mesh_fill(result);
	}

	if (_scatter_enabled) {
		_apply_scatter(result);
	}

	int gen_count = result.size();

	_generating = true;

	// Ensure parent has enough instances.
	if (_parent->get_instance_count() < gen_count) {
		_parent->set_instance_count(gen_count);
	}

	// Write transforms, skipping inactive indices (filtered by instance_every + instance_range).
	_parent->begin_batch();
	for (int i = 0; i < gen_count; i++) {
		if (i < _active.size() && _active[i]) {
			_parent->set_instance_transform(i, result[i]);
		}
	}
	_parent->end_batch();

	_generating = false;
}

void MultiNodeInstancer::clear_and_notify() {
	if (!_parent) return;
	_generating = true;
	_parent->clear_instances();
	_generating = false;
}

// ===========================================================================
// Tool button callables
// ===========================================================================

Callable MultiNodeInstancer::get_btn_regenerate() const { return callable_mp(const_cast<MultiNodeInstancer *>(this), &MultiNodeInstancer::generate); }
Callable MultiNodeInstancer::get_btn_clear() const { return callable_mp(const_cast<MultiNodeInstancer *>(this), &MultiNodeInstancer::clear_and_notify); }
Callable MultiNodeInstancer::get_btn_save_data() const { return callable_mp(const_cast<MultiNodeInstancer *>(this), &MultiNodeInstancer::save_instance_data); }

// ===========================================================================
// Array generation
// ===========================================================================

void MultiNodeInstancer::_generate_array(Vector<Transform3D> &p_out) {
	int count = _generator_count;
	Vector3 spacing = _array_spacing;
	Vector3 rot_rad = _array_rotation;
	float scale_step = _array_scale_step;
	int rows = _array_rows;
	int cols = _array_cols;

	int abs_r = (rows != 0) ? Math::abs(rows) : 0;
	int abs_c = (cols != 0) ? Math::abs(cols) : 0;
	float r_sign = (rows < 0) ? -1.0f : 1.0f;
	float c_sign = (cols < 0) ? -1.0f : 1.0f;
	float y_sign = _array_flip_layers ? -1.0f : 1.0f;

	p_out.resize(count);

	if (_array_follow_rotation) {
		Vector3 step_offset = Vector3(spacing.x, 0, 0);
		Basis step_basis = Basis::from_euler(rot_rad);
		if (scale_step != 1.0f) {
			step_basis.scale_local(Vector3(scale_step, scale_step, scale_step));
		}
		Transform3D step_xform(step_basis, step_offset);

		if (abs_r == 0) {
			Transform3D current(Basis(), _array_origin);
			for (int i = 0; i < count; i++) {
				p_out.write[i] = current;
				current = current * step_xform;
			}
		} else {
			int idx = 0;
			int layer = 0;
			int col = 0;
			while (idx < count) {
				Vector3 row_origin = _array_origin + Vector3(
						0,
						(float)layer * spacing.y * y_sign,
						(float)col * spacing.z * c_sign);
				Transform3D current(Basis(), row_origin);
				for (int ri = 0; ri < abs_r && idx < count; ri++, idx++) {
					p_out.write[idx] = current;
					current = current * step_xform;
				}
				col++;
				if (abs_c > 0 && col >= abs_c) {
					col = 0;
					layer++;
				}
			}
		}
	} else {
		for (int i = 0; i < count; i++) {
			int ri = 0, ci = 0, li = 0;
			if (abs_r > 0 && abs_c > 0) {
				ri = i % abs_r;
				int rem = i / abs_r;
				ci = rem % abs_c;
				li = rem / abs_c;
			} else if (abs_r > 0) {
				ri = i % abs_r;
				ci = i / abs_r;
			} else {
				ri = i;
			}

			Vector3 pos = _array_origin + Vector3(
					(float)ri * spacing.x * r_sign,
					(float)li * spacing.y * y_sign,
					(float)ci * spacing.z * c_sign);

			Basis basis;
			if (rot_rad != Vector3()) {
				basis = Basis::from_euler(rot_rad * (float)i);
			}
			if (scale_step != 1.0f) {
				float s = Math::pow(scale_step, (float)i);
				basis.scale_local(Vector3(s, s, s));
			}
			p_out.write[i] = Transform3D(basis, pos);
		}
	}
}

// ===========================================================================
// Mesh Fill generation
// ===========================================================================

Basis MultiNodeInstancer::_basis_from_normal(const Vector3 &p_normal) {
	Vector3 up = p_normal.normalized();
	Vector3 ref = (Math::abs(up.dot(Vector3(1, 0, 0))) < 0.99f) ? Vector3(1, 0, 0) : Vector3(0, 0, 1);
	Vector3 right = up.cross(ref).normalized();
	Vector3 forward = right.cross(up).normalized();
	return Basis(right, up, forward);
}

bool MultiNodeInstancer::_ray_tri_intersect(const Vector3 &p_origin, const Vector3 &p_dir,
		const Vector3 &p_a, const Vector3 &p_b, const Vector3 &p_c) {
	Vector3 edge1 = p_b - p_a;
	Vector3 edge2 = p_c - p_a;
	Vector3 h = p_dir.cross(edge2);
	float det = edge1.dot(h);
	if (Math::abs(det) < 1e-8f) return false;
	float inv_det = 1.0f / det;
	Vector3 s = p_origin - p_a;
	float u = inv_det * s.dot(h);
	if (u < 0.0f || u > 1.0f) return false;
	Vector3 q = s.cross(edge1);
	float v = inv_det * p_dir.dot(q);
	if (v < 0.0f || u + v > 1.0f) return false;
	float t = inv_det * edge2.dot(q);
	return t > 1e-8f;
}

bool MultiNodeInstancer::_point_in_mesh(const Vector3 &p_point,
		const PackedVector3Array &p_faces, int p_tri_count) {
	Vector3 dir(1, 0, 0);
	int hits = 0;
	for (int t = 0; t < p_tri_count; t++) {
		if (_ray_tri_intersect(p_point, dir,
				p_faces[t * 3], p_faces[t * 3 + 1], p_faces[t * 3 + 2])) {
			hits++;
		}
	}
	return (hits % 2) == 1;
}

void MultiNodeInstancer::_generate_mesh_fill(Vector<Transform3D> &p_out) {
	if (!_fill_mesh.is_valid()) return;

	PackedVector3Array faces = _fill_mesh->get_faces();
	int tri_count = faces.size() / 3;
	if (tri_count == 0) return;

	int count = _generator_count;
	float min_dist = _fill_min_distance;
	float min_dist_sq = min_dist * min_dist;

	if (_fill_mode == FILL_SURFACE) {
		PackedFloat32Array cdf;
		cdf.resize(tri_count);
		float total_area = 0.0f;
		for (int t = 0; t < tri_count; t++) {
			Vector3 a = faces[t * 3];
			Vector3 b = faces[t * 3 + 1];
			Vector3 c = faces[t * 3 + 2];
			total_area += (b - a).cross(c - a).length() * 0.5f;
			cdf.set(t, total_area);
		}

		Vector<Vector3> placed;
		int max_attempts = count * 10;
		for (int attempt = 0; attempt < max_attempts && p_out.size() < count; attempt++) {
			float r = UtilityFunctions::randf() * total_area;
			int lo = 0, hi = tri_count;
			while (lo < hi) {
				int mid = (lo + hi) / 2;
				if (cdf[mid] < r) lo = mid + 1;
				else hi = mid;
			}
			int ti = MIN(lo, tri_count - 1);

			Vector3 a = faces[ti * 3];
			Vector3 b = faces[ti * 3 + 1];
			Vector3 c = faces[ti * 3 + 2];

			float u = UtilityFunctions::randf();
			float v = UtilityFunctions::randf();
			if (u + v > 1.0f) { u = 1.0f - u; v = 1.0f - v; }
			Vector3 pos = a + u * (b - a) + v * (c - a);

			if (min_dist > 0.0f) {
				bool too_close = false;
				for (int j = 0; j < placed.size(); j++) {
					if (pos.distance_squared_to(placed[j]) < min_dist_sq) {
						too_close = true; break;
					}
				}
				if (too_close) continue;
			}

			Basis basis;
			if (_fill_align) {
				Vector3 normal = (b - a).cross(c - a).normalized();
				basis = _basis_from_normal(normal);
			}
			if (_scatter_random_rotation) {
				Vector3 axis = _fill_align ? basis.get_column(1) : Vector3(0, 1, 0);
				basis.rotate(axis, UtilityFunctions::randf() * Math_TAU);
			}
			p_out.push_back(Transform3D(basis, pos));
			placed.push_back(pos);
		}
	} else {
		AABB aabb = _fill_mesh->get_aabb();
		Vector<Vector3> placed;
		int max_attempts = count * 20;
		for (int attempt = 0; attempt < max_attempts && p_out.size() < count; attempt++) {
			Vector3 pos = aabb.position + Vector3(
					UtilityFunctions::randf() * aabb.size.x,
					UtilityFunctions::randf() * aabb.size.y,
					UtilityFunctions::randf() * aabb.size.z);

			if (!_point_in_mesh(pos, faces, tri_count)) continue;

			if (min_dist > 0.0f) {
				bool too_close = false;
				for (int j = 0; j < placed.size(); j++) {
					if (pos.distance_squared_to(placed[j]) < min_dist_sq) {
						too_close = true; break;
					}
				}
				if (too_close) continue;
			}

			Basis basis;
			if (_scatter_random_rotation) {
				basis.rotate(Vector3(0, 1, 0), UtilityFunctions::randf() * Math_TAU);
			}
			p_out.push_back(Transform3D(basis, pos));
			placed.push_back(pos);
		}
	}
}

// ===========================================================================
// Scatter
// ===========================================================================

void MultiNodeInstancer::_apply_scatter(Vector<Transform3D> &p_transforms) {
	Vector3 jitter = _scatter_jitter;
	bool rand_rot = _scatter_random_rotation;
	float smin = _scatter_scale_min;
	float smax = _scatter_scale_max;

	for (int i = 0; i < p_transforms.size(); i++) {
		Transform3D &xform = p_transforms.write[i];
		Vector3 pos = xform.origin;
		if (jitter.x != 0.0f) pos.x += UtilityFunctions::randf_range(-jitter.x, jitter.x);
		if (jitter.y != 0.0f) pos.y += UtilityFunctions::randf_range(-jitter.y, jitter.y);
		if (jitter.z != 0.0f) pos.z += UtilityFunctions::randf_range(-jitter.z, jitter.z);
		xform.origin = pos;

		if (rand_rot) {
			xform.basis.rotate(Vector3(0, 1, 0), UtilityFunctions::randf() * Math_TAU);
		}
		if (smin != smax) {
			float s = UtilityFunctions::randf_range(smin, smax);
			xform.basis.scale_local(Vector3(s, s, s));
		}
	}
}
