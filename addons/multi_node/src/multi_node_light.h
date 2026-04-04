#ifndef MULTI_NODE_LIGHT_H
#define MULTI_NODE_LIGHT_H

#include "multi_node_visual_3d.h"
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {

/// Dynamic lights for each instance in the parent MultiNode.
/// Each active instance gets a RenderingServer omni or spot light.
///
/// Use cases: headlights, warning beacons, task lights, streetlamps.
/// Light count should be kept reasonable (use instance_every for culling).
class MultiNodeLight : public MultiNodeVisual3D {
	GDCLASS(MultiNodeLight, MultiNodeVisual3D)

public:
	enum LightType {
		LIGHT_OMNI = 0,
		LIGHT_SPOT = 1,
	};

protected:
	static void _bind_methods();

	void _on_sub_entered() override;
	void _on_sub_exiting() override;
	void _on_parent_sync() override;
	void _on_active_changed() override;
	RID _get_visual_instance_rid() const override;

	void _on_instance_color_changed(int p_index, const Color &p_color) override;

public:
	MultiNodeLight();
	~MultiNodeLight();

	void set_light_type(int p_type);
	int get_light_type() const;

	void set_light_color(const Color &p_color);
	Color get_light_color() const;

	void set_light_energy(float p_energy);
	float get_light_energy() const;

	void set_light_range(float p_range);
	float get_light_range() const;

	void set_spot_angle(float p_angle);
	float get_spot_angle() const;

	void set_shadow_enabled(bool p_enable);
	bool get_shadow_enabled() const;

	// Per-instance.
	void set_instance_energy(int p_index, float p_energy);

private:
	int _light_type = LIGHT_OMNI;
	Color _light_color = Color(1, 1, 1, 1);
	float _light_energy = 1.0f;
	float _light_range = 10.0f;
	float _spot_angle = 45.0f;
	bool _shadow_enabled = false;

	struct LightInstance {
		RID light;
		RID instance;
	};

	Vector<LightInstance> _lights;

	void _rebuild();
	void _clear_lights();
	void _sync_lights();
	void _configure_light(RID p_light);
};

} // namespace godot

VARIANT_ENUM_CAST(MultiNodeLight::LightType);

#endif
