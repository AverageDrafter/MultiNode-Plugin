#ifndef MULTI_NODE_VISUAL_3D_H
#define MULTI_NODE_VISUAL_3D_H

#include "multi_node_3d.h"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

namespace godot {

/// Base class for visual MultiNode children (Mesh, Label, Particle, etc.).
/// Manages shared rendering properties that apply to the RenderingServer
/// visual instance: shadow casting, render layers, visibility range.
/// Also owns the per-instance color tint array used by all visual sub-nodes.
///
/// Subclasses must call _apply_visual_properties() after creating their
/// instance RID, and provide it via _get_visual_instance_rid().
class MultiNodeVisual3D : public MultiNode3D {
	GDCLASS(MultiNodeVisual3D, MultiNode3D)

public:
	enum ShadowCastingSetting {
		SHADOW_CASTING_OFF = 0,
		SHADOW_CASTING_ON = 1,
		SHADOW_CASTING_DOUBLE_SIDED = 2,
		SHADOW_CASTING_SHADOWS_ONLY = 3,
	};

protected:
	static void _bind_methods();

	/// Subclasses must override to return their visual instance RID.
	virtual RID _get_visual_instance_rid() const;

	/// Call after creating/recreating the visual instance RID.
	void _apply_visual_properties();

	/// Called when a single instance color changes. Override to push to backend.
	virtual void _on_instance_color_changed(int p_index, const Color &p_color);

	/// Called when the entire color array changes (set_all_colors / resize).
	/// Override to push a bulk update to backend.
	virtual void _on_colors_changed();

	/// Per-instance color tint array. Accessible to subclasses.
	PackedColorArray _colors;

public:
	MultiNodeVisual3D();
	~MultiNodeVisual3D();

	void set_cast_shadow(ShadowCastingSetting p_mode);
	ShadowCastingSetting get_cast_shadow() const;

	void set_render_layers(uint32_t p_layers);
	uint32_t get_render_layers() const;

	void set_visibility_range_begin(float p_distance);
	float get_visibility_range_begin() const;

	void set_visibility_range_end(float p_distance);
	float get_visibility_range_end() const;

	void set_tint(const Color &p_color);
	Color get_tint() const;

	// --- Per-instance color tint ---
	void set_instance_color(int p_index, const Color &p_color);
	Color get_instance_color(int p_index) const;
	void set_all_colors(const PackedColorArray &p_colors);

	/// Resize the color array to match instance count, filling new entries with white.
	void resize_colors(int p_count);

	/// Get the per-instance color multiplied by this node's tint.
	Color get_tinted_color(int p_index) const;

private:
	Color _tint = Color(1, 1, 1, 1);
	ShadowCastingSetting _cast_shadow = SHADOW_CASTING_ON;
	uint32_t _render_layers = 1;
	float _visibility_range_begin = 0.0f;
	float _visibility_range_end = 0.0f;
};

} // namespace godot

VARIANT_ENUM_CAST(MultiNodeVisual3D::ShadowCastingSetting);

#endif // MULTI_NODE_VISUAL_3D_H
