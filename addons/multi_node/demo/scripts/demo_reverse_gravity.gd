extends MultiNodeSub
## Reverse gravity via compute shader.
## Reads force vectors from instance data (written by the GPU shader)
## and applies them as impulses via MultiNode.apply_impulse — works
## regardless of which collider is currently active for each instance.

var _parent: MultiNode = null


func _ready() -> void:
	_parent = get_multi_node_parent()


func _process(delta: float) -> void:
	if not compute_enabled or not _parent or get_active_count() == 0:
		return

	var count: int = _parent.instance_count
	for i: int in range(count):
		if not is_instance_active(i):
			continue
		var force_y: float = get_instance_data(i, 1)
		if absf(force_y) < 0.001:
			continue
		_parent.apply_impulse(i, Vector3(0.0, force_y * delta, 0.0))
