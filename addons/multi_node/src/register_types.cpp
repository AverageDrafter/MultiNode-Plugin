#include "register_types.h"

#include "multi_node.h"
#include "multi_node_sub.h"
#include "multi_node_3d.h"
#include "multi_node_visual_3d.h"
#include "multi_node_mesh.h"
#include "multi_node_collider.h"
#include "multi_node_label.h"
#include "multi_node_animator.h"
#include "multi_node_audio.h"
#include "multi_node_area.h"
#include "multi_node_raycast.h"
#include "multi_node_particle.h"
#include "multi_node_sprite.h"
#include "multi_node_decal.h"
#include "multi_node_light.h"
#include "multi_node_instancer.h"
#include "multi_node_marker.h"
#include "multi_node_instance_data.h"
#include "multi_animation.h"
#include "multi_animation_player.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_multi_node_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(MultiNode);
	GDREGISTER_CLASS(MultiNodeSub);
	GDREGISTER_CLASS(MultiNode3D);
	GDREGISTER_CLASS(MultiNodeVisual3D);
	GDREGISTER_CLASS(MultiNodeMesh);
	GDREGISTER_CLASS(MultiNodeCollider);
	GDREGISTER_CLASS(MultiNodeLabel);
	GDREGISTER_CLASS(MultiNodeAnimator);
	GDREGISTER_CLASS(MultiNodeAudio);
	GDREGISTER_CLASS(MultiNodeArea);
	GDREGISTER_CLASS(MultiNodeRaycast);
	GDREGISTER_CLASS(MultiNodeParticle);
	GDREGISTER_CLASS(MultiNodeSprite);
	GDREGISTER_CLASS(MultiNodeDecal);
	GDREGISTER_CLASS(MultiNodeLight);
	GDREGISTER_CLASS(MultiNodeInstancer);
	GDREGISTER_CLASS(MultiNodeMarker);
	GDREGISTER_CLASS(MultiNodeInstanceData);
	GDREGISTER_CLASS(MultiAnimation);
	GDREGISTER_CLASS(MultiAnimationPlayer);
}

void uninitialize_multi_node_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C" {
GDExtensionBool GDE_EXPORT multi_node_library_init(
		GDExtensionInterfaceGetProcAddress p_get_proc_address,
		const GDExtensionClassLibraryPtr p_library,
		GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_multi_node_module);
	init_obj.register_terminator(uninitialize_multi_node_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
