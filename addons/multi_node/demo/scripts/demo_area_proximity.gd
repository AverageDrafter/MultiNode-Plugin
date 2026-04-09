extends MultiNodeArea
## Area proximity demo — color-only isolation build (collider/mesh switching disabled).
## Area source instances turn green.
## Nearby instances turn purple.
## Everything else stays white.
## Collider switching is commented out to isolate whether perf issues come from area or collider swaps.

@export var proximity_radius: float = 5.0
@export var check_interval: int = 4

var _parent: MultiNode = null
var _mesh_box: MultiNodeMesh = null
#var _mesh_sphere: MultiNodeMesh = null
#var _mesh_capsule: MultiNodeMesh = null
#var _col_box: MultiNodeCollider = null
#var _col_sphere: MultiNodeCollider = null
#var _col_capsule: MultiNodeCollider = null

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
				#&"MultiNodeMeshSphere": _mesh_sphere = child as MultiNodeMesh
				#&"MultiNodeMeshCapsule": _mesh_capsule = child as MultiNodeMesh
		#if child is MultiNodeCollider:
		#	match child.name:
		#		&"MultiNodeCollider": _col_box = child as MultiNodeCollider
		#		&"MultiNodeColliderSphere": _col_sphere = child as MultiNodeCollider
		#		&"MultiNodeColliderCapsule": _col_capsule = child as MultiNodeCollider


func _process(_delta: float) -> void:
	# Press P to dump performance stats.
	if Input.is_key_pressed(KEY_P) and Engine.get_process_frames() % 60 == 0:
		print("=== PERF DUMP ===")
		print("  FPS: ", Engine.get_frames_per_second())
		print("  Physics objects: ", Performance.get_monitor(Performance.PHYSICS_3D_ACTIVE_OBJECTS))
		print("  Physics islands: ", Performance.get_monitor(Performance.PHYSICS_3D_ISLAND_COUNT))
		print("  Collision pairs: ", Performance.get_monitor(Performance.PHYSICS_3D_COLLISION_PAIRS))
		print("  Draw calls: ", Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME))
		print("  Objects drawn: ", Performance.get_monitor(Performance.RENDER_TOTAL_OBJECTS_IN_FRAME))
		print("  Primitives: ", Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME))
		print("  VRAM used: ", Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED) / 1048576, " MB")
		print("  Process time: ", Performance.get_monitor(Performance.TIME_PROCESS), " ms")
		print("  Physics time: ", Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS), " ms")
		print("  Areas active: ", get_active_count())
		print("=================")

	if not _parent:
		return

	# When area is off, revert any active swaps and stop.
	if get_active_count() == 0:
		if _sphere_set.size() > 0 or _capsule_set.size() > 0:
			print("[AreaScript] Reverting: spheres=", _sphere_set.size(), " capsules=", _capsule_set.size())
		_revert_all()
		return

	_frame_counter += 1
	if _frame_counter < check_interval:
		return
	_frame_counter = 0

	if not _mesh_box:
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

	# Collect area source positions and color them green.
	var area_positions: PackedVector3Array = PackedVector3Array()
	for i: int in range(count):
		if is_instance_active(i):
			area_positions.push_back(all_positions[i])
			_activate_sphere(i)
			_mesh_box.set_instance_color(i, source_color)

	var area_count: int = area_positions.size()
	if area_count == 0:
		return

	# Check proximity for remaining instances and color them purple.
	for i: int in range(count):
		if is_instance_active(i):
			continue
		var pos: Vector3 = all_positions[i]
		for j: int in range(area_count):
			if pos.distance_squared_to(area_positions[j]) < radius_sq:
				_activate_capsule(i)
				_mesh_box.set_instance_color(i, affect_color)
				break


func _activate_sphere(idx: int) -> void:
	# Mesh/collider switching disabled — tracking index only for revert.
	_sphere_set.push_back(idx)


func _activate_capsule(idx: int) -> void:
	# Mesh/collider switching disabled — tracking index only for revert.
	_capsule_set.push_back(idx)


func _revert_all() -> void:
	for i: int in range(_sphere_set.size()):
		_mesh_box.set_instance_color(_sphere_set[i], Color.WHITE)
	_sphere_set.clear()

	for i: int in range(_capsule_set.size()):
		_mesh_box.set_instance_color(_capsule_set[i], Color.WHITE)
	_capsule_set.clear()
