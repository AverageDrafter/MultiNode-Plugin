#ifndef MULTI_NODE_PARTICLE_H
#define MULTI_NODE_PARTICLE_H

#include "multi_node_visual_3d.h"
#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

/// GPU particle emitters for each active instance in the parent MultiNode.
/// Creates GPUParticles3D child nodes — one per active instance.
/// All share the same process material and draw mesh.
///
/// Inherits cast_shadow and render_layers from MultiNodeVisual3D.
class MultiNodeParticle : public MultiNodeVisual3D {
	GDCLASS(MultiNodeParticle, MultiNodeVisual3D)

protected:
	static void _bind_methods();

	void _on_sub_entered() override;
	void _on_sub_exiting() override;
	void _on_parent_sync() override;
	void _on_active_changed() override;
	RID _get_visual_instance_rid() const override;

	void _on_colors_changed() override;
	void _on_instance_color_changed(int p_index, const Color &p_color) override;

public:
	MultiNodeParticle();
	~MultiNodeParticle();

	// --- Core properties ---
	void set_process_material(const Ref<Material> &p_material);
	Ref<Material> get_process_material() const;

	void set_draw_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_draw_mesh() const;

	void set_amount(int p_amount);
	int get_amount() const;

	void set_lifetime(float p_lifetime);
	float get_lifetime() const;

	void set_emitting(bool p_emitting);
	bool get_emitting() const;

	// --- Simulation properties ---
	void set_speed_scale(float p_scale);
	float get_speed_scale() const;

	void set_explosiveness(float p_ratio);
	float get_explosiveness() const;

	void set_randomness(float p_ratio);
	float get_randomness() const;

	void set_one_shot(bool p_one_shot);
	bool get_one_shot() const;

	void set_pre_process_time(float p_time);
	float get_pre_process_time() const;

	void set_fixed_fps(int p_fps);
	int get_fixed_fps() const;

	// --- Per-instance ---
	void set_instance_emitting(int p_index, bool p_emitting);
	bool get_instance_emitting(int p_index) const;

	void restart_instance(int p_index);
	void restart_all();

private:
	Ref<Material> _process_material;
	Ref<Mesh> _draw_mesh;
	int _amount = 16;
	float _lifetime = 1.0f;
	bool _emitting = true;
	float _speed_scale = 1.0f;
	float _explosiveness = 0.0f;
	float _randomness = 0.0f;
	bool _one_shot = false;
	float _pre_process_time = 0.0f;
	int _fixed_fps = 0;

	Vector<GPUParticles3D *> _emitters;
	PackedByteArray _inst_emitting;

	void _rebuild();
	void _clear_emitters();
	void _sync_emitters();
	void _resize_state(int p_count);
	void _configure_emitter(GPUParticles3D *p_emitter);
	void _apply_visual_to_emitter(GPUParticles3D *p_emitter);
};

} // namespace godot

#endif // MULTI_NODE_PARTICLE_H
