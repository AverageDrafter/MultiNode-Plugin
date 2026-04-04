#ifndef MULTI_NODE_SUB_H
#define MULTI_NODE_SUB_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

class MultiNode;

/// Base class for all MultiNode child nodes (Mesh, Collider, Label, etc.).
/// Handles parent discovery, signal wiring, per-instance active array,
/// general-purpose per-instance data, and optional GPU compute shader dispatch.
///
/// === Active filtering — two layers, ANDed together ===
///
/// instance_every:  coarse pattern filter
///    0 = DISABLED — this sub skips all instances
///    1 = every instance (all)
///    N = every Nth instance (2=every other, 3=every 3rd, etc.)
///   -N = complement (all EXCEPT every Nth)
///
/// instance_range:  Excel-style range string
///    ""            = all instances (no restriction)
///    "0:9"         = indices 0 through 9
///    "1:10,15"     = 1-10 and 15
///    "0:99,-50:59" = 0-99 except 50-59
///    "-42"         = exclude index 42 from whatever was included
///    Processed left-to-right: inclusions add, exclusions (-) remove.
///
/// The computed _active array is: every_ok(i) AND range_ok(i)
/// For anything more complex, manipulate the _active array directly via
/// set_instance_active() at runtime.
///
/// Instance data:
///    A flat float array with configurable stride (floats per instance).
///    Accessible from GDScript, C++, and compute shaders (binding 2).
///
/// Compute shaders:
///    Set compute_code to a GLSL compute shader string. The shader runs
///    once per instance per frame on the GPU. It receives:
///      - binding 0: transforms (3 x vec4 per instance, read/write)
///      - binding 1: active flags (1 x uint per instance, read)
///      - binding 2: instance data (stride floats per instance, read/write)
///      - push constants: delta, time, count, stride
///    Output mode controls how results are applied (transforms or impulses).
class MultiNodeSub : public Node3D {
	GDCLASS(MultiNodeSub, Node3D)

protected:
	static void _bind_methods();
	void _notification(int p_what);

	MultiNode *_parent = nullptr;

	// --- Active filtering (computed output) ---
	PackedByteArray _active;
	PackedByteArray _prev_active; // Previous frame's active state for change detection.
	bool _any_active_changed = false;

	// --- Layer 1: instance_every ---
	int32_t _instance_every = 1;

	// --- Layer 2: range string ---
	String _range_string;
	PackedByteArray _range_flags; // Cached parsed result. 1 = in range.

	virtual void _on_sub_entered();
	virtual void _on_sub_exiting();
	virtual void _on_parent_sync();
	virtual void _on_active_changed();

public:
	MultiNodeSub();
	~MultiNodeSub();

	PackedStringArray _get_configuration_warnings() const override;

	// --- Layer 1: instance_every ---
	void set_instance_every(int32_t p_every);
	int32_t get_instance_every() const;

	// --- Layer 2: range string ---
	void set_instance_range(const String &p_range);
	String get_instance_range() const;

	// --- Direct active array access (for runtime code) ---
	void set_instance_active(int p_index, bool p_active);
	bool is_instance_active(int p_index) const;
	void set_all_active(bool p_active);
	int get_active_count() const;

	/// Returns true if any per-instance active flags changed since last commit.
	bool has_active_state_changes() const;
	/// Returns true if the given instance's active flag differs from previous state.
	bool did_instance_active_change(int p_index) const;
	/// Snapshots current active state as the baseline for future change detection.
	void commit_active_state();

	MultiNode *get_multi_node_parent() const;

	// --- Per-instance data array (stride controlled by parent MultiNode) ---
	int get_data_stride() const;

	void set_instance_data(int p_index, int p_channel, float p_value);
	float get_instance_data(int p_index, int p_channel) const;

	void set_instance_data_vec(int p_index, const PackedFloat32Array &p_values);
	PackedFloat32Array get_instance_data_vec(int p_index) const;

	// --- Compute shader ---
	void set_compute_code(const String &p_code);
	String get_compute_code() const;

	void set_compute_enabled(bool p_enabled);
	bool get_compute_enabled() const;

private:
	void _setup_parent();
	void _cleanup();
	void _on_instances_changed();

	/// Recomputes _active by ANDing instance_every and range_flags.
	void _recompute_active();
	/// Parses _range_string into _range_flags for current instance count.
	void _parse_range_string();

	// --- Per-instance data (stride from parent's sub_data_stride) ---
	PackedFloat32Array _instance_data;
	int _cached_stride = 4;

	void _resize_instance_data(int p_count);
	void _cache_stride();

	// --- Compute shader state ---
	String _compute_code;
	bool _compute_enabled = false;

	RenderingDevice *_rd = nullptr;
	RID _compute_shader;
	RID _compute_pipeline;
	RID _transform_buffer;
	RID _active_buffer;
	RID _data_buffer;
	RID _uniform_set;
	bool _compute_ready = false;
	int _compute_count = 0;

	PackedByteArray _stage_transforms;
	PackedByteArray _stage_active;
	PackedByteArray _stage_data;

	void _setup_compute();
	void _teardown_compute();
	void _dispatch_compute(double p_delta);
	void _resize_compute_buffers(int p_count);
};

} // namespace godot

#endif // MULTI_NODE_SUB_H
