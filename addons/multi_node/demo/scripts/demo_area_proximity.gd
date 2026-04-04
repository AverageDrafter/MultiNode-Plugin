extends MultiNodeArea
## Area proximity demo — swaps mesh and collider shapes at runtime.
## Area source instances become green spheres.
## Nearby instances become purple capsules.
## Everything else stays as the default box.

@export var proximity_radius: float = 5.0
@export var check_interval: int = 4

var _parent: MultiNode = null
var _mesh_box: MultiNodeMesh = null
var _mesh_sphere: MultiNodeMesh = null
var _mesh_capsule: MultiNodeMesh = null
var _col_box: MultiNodeCollider = null
var _col_sphere: MultiNodeCollider = null
var _col_capsule: MultiNodeCollider = null

var _frame_counter: int = 0
# Track which instances are currently non-default so we only revert those.
var _sphere_set: PackedInt32Array = PackedInt32Array()
var _capsule_set: PackedInt32Array = PackedInt32Array()


func _ready() -> void:
	_parent = get_multi_node_parent()
	if not _parent:
		return

	for c: int in range(_parent.get_child_count()):
		var child: Node = _parent.get_child(c)
		if child is MultiNodeMesh:
			match child.name:
				&"MultiNodeMesh": _mesh_box = child as MultiNodeMesh
				&"MultiNodeMeshSphere": _mesh_sphere = child as MultiNodeMesh
				&"MultiNodeMeshCapsule": _mesh_capsule = child as MultiNodeMesh
		if child is MultiNodeCollider:
			match child.name:
				&"MultiNodeCollider": _col_box = child as MultiNodeCollider
				&"MultiNodeColliderSphere": _col_sphere = child as MultiNodeCollider
				&"MultiNodeColliderCapsule": _col_capsule = child as MultiNodeCollider


func _process(_delta: float) -> void:
	if not _parent:
		return

	# When area is off, revert any active swaps and stop.
	if get_active_count() == 0:
		_revert_all()
		return

	_frame_counter += 1
	if _frame_counter < check_interval:
		return
	_frame_counter = 0

	if not _mesh_box or not _mesh_sphere or not _mesh_capsule:
		return
	if not _col_box or not _col_sphere or not _col_capsule:
		return

	var count: int = _parent.instance_count
	if count == 0:
		return

	# Step 1: Revert previous frame's swaps back to box.
	_revert_all()

	# Step 2: Apply new swaps based on current positions.
	var radius_sq: float = proximity_radius * proximity_radius
	var pulse: float = (sin(Engine.get_process_frames() * 0.1) + 1.0) * 0.5
	var source_color: Color = Color(0.0, 0.8 + pulse * 0.4, 0.0, 1.0)
	var affect_color: Color = Color(0.6 + pulse * 0.2, 0.0, 0.8 + pulse * 0.2, 1.0)

	var all_positions: PackedVector3Array = _parent.get_all_positions()

	# Collect area source positions and swap sources to sphere.
	var area_positions: PackedVector3Array = PackedVector3Array()
	for i: int in range(count):
		if is_instance_active(i):
			area_positions.push_back(all_positions[i])
			_activate_sphere(i)
			_mesh_sphere.set_instance_color(i, source_color)

	var area_count: int = area_positions.size()
	if area_count == 0:
		return

	# Check proximity for remaining instances.
	for i: int in range(count):
		if is_instance_active(i):
			continue
		var pos: Vector3 = all_positions[i]
		for j: int in range(area_count):
			if pos.distance_squared_to(area_positions[j]) < radius_sq:
				_activate_capsule(i)
				_mesh_capsule.set_instance_color(i, affect_color)
				break


func _activate_sphere(idx: int) -> void:
	_mesh_box.set_instance_active(idx, false)
	_col_box.set_instance_active(idx, false)
	_mesh_sphere.set_instance_active(idx, true)
	_col_sphere.set_instance_active(idx, true)
	_sphere_set.push_back(idx)


func _activate_capsule(idx: int) -> void:
	_mesh_box.set_instance_active(idx, false)
	_col_box.set_instance_active(idx, false)
	_mesh_capsule.set_instance_active(idx, true)
	_col_capsule.set_instance_active(idx, true)
	_capsule_set.push_back(idx)


func _revert_all() -> void:
	for i: int in range(_sphere_set.size()):
		var idx: int = _sphere_set[i]
		_mesh_sphere.set_instance_active(idx, false)
		_col_sphere.set_instance_active(idx, false)
		_mesh_box.set_instance_active(idx, true)
		_col_box.set_instance_active(idx, true)
	_sphere_set.clear()

	for i: int in range(_capsule_set.size()):
		var idx: int = _capsule_set[i]
		_mesh_capsule.set_instance_active(idx, false)
		_col_capsule.set_instance_active(idx, false)
		_mesh_box.set_instance_active(idx, true)
		_col_box.set_instance_active(idx, true)
	_capsule_set.clear()
