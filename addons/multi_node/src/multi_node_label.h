#ifndef MULTI_NODE_LABEL_H
#define MULTI_NODE_LABEL_H

#include "multi_node_visual_3d.h"
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/templates/hash_map.hpp>

namespace godot {

/// Renders text labels above instances using GPU-instanced billboard quads.
/// All labels share a single text atlas texture and one draw call via MultiMesh.
///
/// Per-instance text is set via set_instance_text(). Empty string = no label.
/// The atlas is rebuilt automatically when texts change.
class MultiNodeLabel : public MultiNodeVisual3D {
	GDCLASS(MultiNodeLabel, MultiNodeVisual3D)

protected:
	static void _bind_methods();
	void _notification(int p_what);

	void _on_sub_entered() override;
	void _on_sub_exiting() override;
	void _on_parent_sync() override;
	void _on_active_changed() override;
	RID _get_visual_instance_rid() const override;

	void _on_colors_changed() override;

public:
	MultiNodeLabel();
	~MultiNodeLabel();

	// --- Properties ---
	void set_font(const Ref<Font> &p_font);
	Ref<Font> get_font() const;

	void set_font_size(int p_size);
	int get_font_size() const;

	void set_text_color(const Color &p_color);
	Color get_text_color() const;

	void set_outline_color(const Color &p_color);
	Color get_outline_color() const;

	void set_outline_size(int p_size);
	int get_outline_size() const;

	void set_label_height(float p_height);
	float get_label_height() const;

	void set_cell_padding(int p_padding);
	int get_cell_padding() const;

	void set_render_scale(float p_scale);
	float get_render_scale() const;

	void set_max_label_distance(float p_dist);
	float get_max_label_distance() const;

	// --- Per-instance text ---
	void set_instance_text(int p_index, const String &p_text);
	String get_instance_text(int p_index) const;

	// --- Bulk text API ---
	void set_all_texts(const String &p_text);
	void set_texts(const PackedStringArray &p_texts);

	// --- Batch text API (suppress atlas rebuild until end) ---
	void begin_text_batch();
	void end_text_batch();

private:
	Ref<Font> _font;
	int _font_size = 32;
	Color _text_color = Color(1, 1, 1, 1);
	Color _outline_color = Color(0, 0, 0, 1);
	int _outline_size = 4;
	float _label_height = 0.5f;
	int _cell_padding = 8;
	float _render_scale = 2.0f;
	float _max_label_distance = 100.0f;

	PackedStringArray _texts;
	bool _text_batch_mode = false;

	// Atlas state.
	Ref<ImageTexture> _atlas_texture;
	int _atlas_columns = 1;
	int _atlas_rows = 1;
	HashMap<String, int> _text_to_cell;
	HashMap<String, float> _text_to_aspect; // Per-text quad aspect ratio (width/height).

	// Atlas build state machine.
	SubViewport *_atlas_viewport = nullptr;
	RID _atlas_canvas_item;
	int _atlas_wait_frames = 0;
	bool _needs_atlas_rebuild = false;

	// Rendering.
	RID _multimesh;
	RID _instance;
	Ref<ShaderMaterial> _shader_material;
	Ref<Shader> _shader;
	Ref<QuadMesh> _quad_mesh; // Kept alive so RS doesn't lose the mesh reference.
	int _current_count = 0;

	void _rebuild();
	void _cleanup_rids();
	void _sync_positions();

	void _start_atlas_build();
	void _finish_atlas_build();
	void _create_shader();

	Ref<Font> _resolve_font() const;

	static const char *BILLBOARD_SHADER_CODE;
};

} // namespace godot

#endif // MULTI_NODE_LABEL_H
