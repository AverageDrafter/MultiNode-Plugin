#ifndef MULTI_NODE_AREA_H
#define MULTI_NODE_AREA_H

#include "multi_node_3d.h"
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>

namespace godot {

/// Detection zones for each instance in the parent MultiNode.
/// Manages a pool of PhysicsServer3D areas — one per active instance.
/// Reports overlapping bodies per instance.
///
/// Use cases: trigger zones, pickup detection, proximity alerts, checkpoints.
class MultiNodeArea : public MultiNode3D {
	GDCLASS(MultiNodeArea, MultiNode3D)

protected:
	static void _bind_methods();

	void _on_sub_entered() override;
	void _on_sub_exiting() override;
	void _on_parent_sync() override;
	void _on_active_changed() override;

public:
	MultiNodeArea();
	~MultiNodeArea();

	void set_shape(const Ref<Shape3D> &p_shape);
	Ref<Shape3D> get_shape() const;

	void set_collision_layer(uint32_t p_layer);
	uint32_t get_collision_layer() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_monitorable(bool p_monitorable);
	bool get_monitorable() const;

	/// Get the area RID for a specific instance (for custom queries).
	RID get_instance_area_rid(int p_index) const;

	/// Map an area RID back to an instance index. Returns -1 if not found.
	int get_instance_from_area(const RID &p_area_rid) const;

private:
	Ref<Shape3D> _shape;
	uint32_t _collision_layer = 1;
	uint32_t _collision_mask = 1;
	bool _monitorable = false;

	Vector<RID> _areas;
	PackedByteArray _in_space; // 1 = area is currently in the physics space.
	Vector<Transform3D> _last_transforms; // Cached for dirty-skip (like collider).
	HashMap<RID, int> _area_to_index;

	void _rebuild();
	void _clear_areas();
	void _sync_areas();
};

} // namespace godot

#endif // MULTI_NODE_AREA_H
