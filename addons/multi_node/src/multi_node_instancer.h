#ifndef MULTI_NODE_INSTANCER_H
#define MULTI_NODE_INSTANCER_H

#include "multi_node_sub.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/resource.hpp>

namespace godot {

/// Composable instancer node. Add as a child of MultiNode.
/// Generates transforms and writes them to the parent's instance list.
/// The instancer does NOT own instances — it is a tool that creates/updates
/// transforms. Deleting the instancer leaves instances intact in the parent.
///
/// Active filtering (instance_every + instance_range) controls which parent
/// indices the instancer writes to. Inactive indices are left untouched.
///
/// Multiple instancers can coexist, each targeting different index ranges.
class MultiNodeInstancer : public MultiNodeSub {
	GDCLASS(MultiNodeInstancer, MultiNodeSub)

public:
	enum GeneratorMode {
		GEN_ARRAY = 0,
		GEN_MESH_FILL = 1,
		GEN_MANUAL = 2,
	};

	enum FillMode {
		FILL_SURFACE = 0,
		FILL_VOLUME = 1,
	};

protected:
	static void _bind_methods();
	void _notification(int p_what);
	void _validate_property(PropertyInfo &p_property) const;

	void _on_parent_sync() override;

public:
	MultiNodeInstancer();
	~MultiNodeInstancer();

	// --- Generator mode ---
	void set_generator_mode(int p_mode);
	int get_generator_mode() const;
	void set_generator_count(int p_count);
	int get_generator_count() const;
	void set_generator_seed(int p_seed);
	int get_generator_seed() const;

	// --- Animate (script-only, not in inspector) ---
	void set_animate(bool p_animate);
	bool get_animate() const;

	// --- Array mode ---
	void set_array_origin(const Vector3 &p_origin);
	Vector3 get_array_origin() const;
	void set_array_spacing(const Vector3 &p_spacing);
	Vector3 get_array_spacing() const;
	void set_array_rows(int p_rows);
	int get_array_rows() const;
	void set_array_cols(int p_cols);
	int get_array_cols() const;
	void set_array_flip_layers(bool p_flip);
	bool get_array_flip_layers() const;
	void set_array_follow_rotation(bool p_follow);
	bool get_array_follow_rotation() const;
	void set_array_rotation(const Vector3 &p_rot);
	Vector3 get_array_rotation() const;
	void set_array_scale_step(float p_scale);
	float get_array_scale_step() const;

	// --- Mesh Fill mode ---
	void set_fill_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_fill_mesh() const;
	void set_fill_mode(int p_mode);
	int get_fill_mode() const;
	void set_fill_align(bool p_align);
	bool get_fill_align() const;
	void set_fill_min_distance(float p_dist);
	float get_fill_min_distance() const;

	// --- Scatter ---
	void set_scatter_enabled(bool p_enable);
	bool get_scatter_enabled() const;
	void set_scatter_jitter(const Vector3 &p_jitter);
	Vector3 get_scatter_jitter() const;
	void set_scatter_random_rotation(bool p_enable);
	bool get_scatter_random_rotation() const;
	void set_scatter_scale_min(float p_min);
	float get_scatter_scale_min() const;
	void set_scatter_scale_max(float p_max);
	float get_scatter_scale_max() const;

	// --- Generation ---
	void generate();
	void clear_and_notify();

	// --- Manual editor ---
	void set_manual_index(int p_index);
	int get_manual_index() const;
	void set_manual_position(const Vector3 &p_pos);
	Vector3 get_manual_position() const;
	void set_manual_rotation(const Vector3 &p_rot);
	Vector3 get_manual_rotation() const;
	void set_manual_scale(const Vector3 &p_scale);
	Vector3 get_manual_scale() const;

	void manual_insert();
	void manual_delete();

	void set_editor_selected_instance(int p_index);
	int get_editor_selected_instance() const;

	// --- Instance Data Resource ---
	void set_instance_data_resource(const Ref<Resource> &p_res);
	Ref<Resource> get_instance_data_resource() const;
	void save_instance_data();

	// --- Tool buttons ---
	Callable get_btn_regenerate() const;
	Callable get_btn_clear() const;
	Callable get_btn_save_data() const;

private:
	// --- Generator state ---
	int _generator_mode = GEN_ARRAY;
	int _generator_count = 10;
	int _generator_seed = 0;
	bool _animate = false;

	// Array
	Vector3 _array_origin;
	Vector3 _array_spacing = Vector3(2, 2, 2);
	int _array_rows = 0;
	int _array_cols = 0;
	bool _array_flip_layers = false;
	bool _array_follow_rotation = false;
	Vector3 _array_rotation;
	float _array_scale_step = 1.0f;

	// Mesh Fill
	Ref<Mesh> _fill_mesh;
	int _fill_mode = FILL_SURFACE;
	bool _fill_align = true;
	float _fill_min_distance = 0.0f;

	// Scatter
	bool _scatter_enabled = false;
	Vector3 _scatter_jitter;
	bool _scatter_random_rotation = false;
	float _scatter_scale_min = 1.0f;
	float _scatter_scale_max = 1.0f;

	// Manual
	int _manual_index = 0;
	int _editor_selected_instance = -1;
	bool _manual_updating = false;

	// Instance data resource
	Ref<Resource> _instance_data_res;

	// Prevents feedback loops when this instancer modifies the parent.
	bool _generating = false;

	// --- Internal generation ---
	void _editor_generate();
	void _sync_manual_from_index();
	void _generate_array(Vector<Transform3D> &p_out);
	void _generate_mesh_fill(Vector<Transform3D> &p_out);
	void _apply_scatter(Vector<Transform3D> &p_transforms);

	// --- Mesh fill helpers ---
	static bool _ray_tri_intersect(const Vector3 &p_origin, const Vector3 &p_dir,
			const Vector3 &p_a, const Vector3 &p_b, const Vector3 &p_c);
	static bool _point_in_mesh(const Vector3 &p_point,
			const PackedVector3Array &p_faces, int p_tri_count);
	static Basis _basis_from_normal(const Vector3 &p_normal);
};

} // namespace godot

VARIANT_ENUM_CAST(MultiNodeInstancer::GeneratorMode);
VARIANT_ENUM_CAST(MultiNodeInstancer::FillMode);

#endif // MULTI_NODE_INSTANCER_H
