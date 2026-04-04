#ifndef MULTI_NODE_COLLIDER_H
#define MULTI_NODE_COLLIDER_H

#include "multi_node_3d.h"
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hash_map.hpp>

namespace godot {

/// Creates and manages physics bodies for each active instance in the parent MultiNode.
/// Uses PhysicsServer3D directly. Reads rigid body transforms back each physics frame,
/// skipping sleeping bodies. Inactive instances have their bodies removed from the space.
class MultiNodeCollider : public MultiNode3D {
	GDCLASS(MultiNodeCollider, MultiNode3D)

public:
	enum BodyType {
		BODY_TYPE_STATIC = 0,
		BODY_TYPE_KINEMATIC = 1,
		BODY_TYPE_RIGID = 2,
	};

protected:
	static void _bind_methods();
	void _notification(int p_what);

	void _on_sub_entered() override;
	void _on_sub_exiting() override;
	void _on_parent_sync() override;
	void _on_active_changed() override;

public:
	MultiNodeCollider();
	~MultiNodeCollider();

	PackedStringArray _get_configuration_warnings() const override;

	void set_shape(const Ref<Shape3D> &p_shape);
	Ref<Shape3D> get_shape() const;

	void set_body_type(int p_type);
	int get_body_type() const;

	void set_collision_layer(uint32_t p_layer);
	uint32_t get_collision_layer() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_self_collision(bool p_enable);
	bool get_self_collision() const;

	int get_instance_from_body(const RID &p_body_rid) const;
	RID get_body_rid(int p_index) const;
	bool is_instance_in_space(int p_index) const;

	void _physics_process(double p_delta);

private:
	Ref<Shape3D> _shape;
	int _body_type = BODY_TYPE_STATIC;
	uint32_t _collision_layer = 1;
	uint32_t _collision_mask = 1;
	bool _self_collision = true;

	Vector<RID> _bodies;
	Vector<Transform3D> _last_world_transforms; // Cached for sleep detection.
	PackedByteArray _in_space; // Tracks which bodies are currently in the physics space.
	HashMap<RID, int> _body_to_index;
	bool _in_readback = false;
	Transform3D _cached_global_inv; // Cached inverse of global transform.
	bool _global_inv_dirty = true;

	void _rebuild();
	void _clear_bodies();
	void _sync_bodies();
	void _compute_effective_layers(uint32_t &r_layer, uint32_t &r_mask) const;
	void _update_all_layers();
};

} // namespace godot

VARIANT_ENUM_CAST(MultiNodeCollider::BodyType);

#endif // MULTI_NODE_COLLIDER_H
