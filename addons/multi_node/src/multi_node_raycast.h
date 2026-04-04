#ifndef MULTI_NODE_RAYCAST_H
#define MULTI_NODE_RAYCAST_H

#include "multi_node_3d.h"
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

class MultiNodeCollider;

/// Per-instance raycasting for MultiNode.
/// Each active instance casts a ray from its position (+ offset) in a
/// configurable direction. Results are stored per-instance and queryable.
///
/// Use cases: ground detection (vehicles), line of sight (AI),
/// terrain following, wall detection, surface probing.
class MultiNodeRaycast : public MultiNode3D {
	GDCLASS(MultiNodeRaycast, MultiNode3D)

protected:
	static void _bind_methods();
	void _notification(int p_what);

	void _on_sub_entered() override;
	void _on_sub_exiting() override;
	void _on_parent_sync() override;

public:
	MultiNodeRaycast();
	~MultiNodeRaycast();

	void set_cast_direction(const Vector3 &p_dir);
	Vector3 get_cast_direction() const;

	void set_cast_length(float p_length);
	float get_cast_length() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_hit_from_inside(bool p_enable);
	bool get_hit_from_inside() const;

	/// Query results for a specific instance.
	bool is_instance_colliding(int p_index) const;
	Vector3 get_instance_hit_point(int p_index) const;
	Vector3 get_instance_hit_normal(int p_index) const;
	float get_instance_hit_distance(int p_index) const;
	RID get_instance_hit_rid(int p_index) const;

private:
	Vector3 _cast_direction = Vector3(0, -1, 0); // Down by default.
	float _cast_length = 10.0f;
	uint32_t _collision_mask = 1;
	bool _hit_from_inside = false;

	// Per-instance results.
	struct RayResult {
		bool hit = false;
		Vector3 point;
		Vector3 normal;
		float distance = 0.0f;
		RID collider_rid;
	};

	Vector<RayResult> _results;
	uint64_t _cached_collider_id = 0;

	MultiNodeCollider *_get_cached_collider() const;

	void _cast_all();
	void _resize_results(int p_count);
};

} // namespace godot

#endif // MULTI_NODE_RAYCAST_H
