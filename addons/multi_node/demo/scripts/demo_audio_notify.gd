extends MultiNodeAudio
## Notification sound — per-instance pitch variety.

func _ready() -> void:
	var parent: MultiNode = get_multi_node_parent()
	if not parent:
		return
	if parent.instance_count > 0:
		_apply_pitches()
	else:
		parent.instances_changed.connect(_apply_pitches, CONNECT_ONE_SHOT)


func _apply_pitches() -> void:
	var parent: MultiNode = get_multi_node_parent()
	if not parent:
		return
	for i: int in range(parent.instance_count):
		set_instance_pitch(i, randf_range(0.8, 1.3))
