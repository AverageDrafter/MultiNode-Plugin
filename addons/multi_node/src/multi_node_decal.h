#ifndef MULTI_NODE_DECAL_H
#define MULTI_NODE_DECAL_H

#include "multi_node_visual_3d.h"
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

/// Projected decals for each instance in the parent MultiNode.
/// Each active instance gets a RenderingServer decal projected downward.
///
/// Use cases: shadow blobs, tire marks, damage indicators, selection rings.
class MultiNodeDecal : public MultiNodeVisual3D {
	GDCLASS(MultiNodeDecal, MultiNodeVisual3D)

protected:
	static void _bind_methods();

	void _on_sub_entered() override;
	void _on_sub_exiting() override;
	void _on_parent_sync() override;
	void _on_active_changed() override;
	RID _get_visual_instance_rid() const override;

	void _on_instance_color_changed(int p_index, const Color &p_color) override;
	void _on_colors_changed() override;

public:
	MultiNodeDecal();
	~MultiNodeDecal();

	void set_texture_albedo(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_texture_albedo() const;

	void set_decal_size(const Vector3 &p_size);
	Vector3 get_decal_size() const;

	void set_albedo_mix(float p_mix);
	float get_albedo_mix() const;

	void set_normal_fade(float p_fade);
	float get_normal_fade() const;

	void set_upper_fade(float p_fade);
	float get_upper_fade() const;

	void set_lower_fade(float p_fade);
	float get_lower_fade() const;

private:
	Ref<Texture2D> _texture_albedo;
	Vector3 _decal_size = Vector3(2, 2, 2);
	float _albedo_mix = 1.0f;
	float _normal_fade = 0.3f;
	float _upper_fade = 0.3f;
	float _lower_fade = 0.3f;

	struct DecalInstance {
		RID decal;
		RID instance;
	};

	Vector<DecalInstance> _decals;

	void _rebuild();
	void _clear_decals();
	void _sync_decals();
};

} // namespace godot

#endif
