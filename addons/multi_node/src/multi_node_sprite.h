#ifndef MULTI_NODE_SPRITE_H
#define MULTI_NODE_SPRITE_H

#include "multi_node_visual_3d.h"
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {

/// Billboard sprites with sprite sheet animation for MultiNode instances.
/// One draw call for all sprites via MultiMesh. Supports:
/// - Sprite sheet: hframes/vframes grid, per-instance frame index
/// - Per-instance color modulation
/// - Billboard (Y-locked) or flat orientation
/// - Animator integration: set_instance_frame(i, frame) for walk cycles etc.
class MultiNodeSprite : public MultiNodeVisual3D {
	GDCLASS(MultiNodeSprite, MultiNodeVisual3D)

protected:
	static void _bind_methods();
	void _notification(int p_what);

	void _on_sub_entered() override;
	void _on_sub_exiting() override;
	void _on_parent_sync() override;
	void _on_active_changed() override;
	RID _get_visual_instance_rid() const override;

	void _on_instance_color_changed(int p_index, const Color &p_color) override;

public:
	MultiNodeSprite();
	~MultiNodeSprite();

	void set_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_texture() const;

	void set_pixel_size(float p_size);
	float get_pixel_size() const;

	void set_hframes(int p_hframes);
	int get_hframes() const;

	void set_vframes(int p_vframes);
	int get_vframes() const;

	void set_billboard(bool p_billboard);
	bool get_billboard() const;

	void set_pixel_art(bool p_pixel_art);
	bool get_pixel_art() const;

	// Per-instance.
	void set_instance_frame(int p_index, int p_frame);
	int get_instance_frame(int p_index) const;

private:
	Ref<Texture2D> _texture;
	float _pixel_size = 0.01f;
	int _hframes = 1;
	int _vframes = 1;
	bool _billboard = true;
	bool _pixel_art = true;

	// Track which settings the current shader was built with.
	bool _shader_billboard = false;
	bool _shader_pixel_art = false;

	RID _multimesh;
	RID _instance;
	Ref<ShaderMaterial> _shader_material;
	Ref<Shader> _shader;
	Ref<QuadMesh> _quad_mesh;
	PackedFloat32Array _frames; // Per-instance frame index.
	int _current_count = 0;

	void _rebuild();
	void _cleanup_rids();
	void _sync_transforms();
	void _create_shader();

	String _build_shader_code() const;
};

} // namespace godot

#endif
