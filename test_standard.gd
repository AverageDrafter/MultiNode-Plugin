extends Node3D
## Standard Godot comparison scene.
## Spawns individual RigidBody3D + MeshInstance3D + CollisionShape3D nodes
## and a MultiMeshInstance3D for rendering comparison.
## Same spawn pattern as test_multi_node.gd for fair benchmarking.

@export var count: int = 500
@export var use_multimesh: bool = true  ## Use MultiMesh for rendering (true) or individual MeshInstance3D (false).

@onready var label: Label = $UI/Label
var _bodies: Array[RigidBody3D] = []
var _multimesh_instance: MultiMeshInstance3D = null


func _ready() -> void:
	DebugMenu.style = DebugMenu.Style.VISIBLE_DETAILED
	seed(Time.get_ticks_msec())

	var box_mesh: BoxMesh = BoxMesh.new()
	var mat: StandardMaterial3D = StandardMaterial3D.new()
	mat.vertex_color_use_as_albedo = true
	box_mesh.material = mat

	var box_shape: BoxShape3D = BoxShape3D.new()

	# --- Spawn rigid bodies ---
	for i: int in range(count):
		var height: float = 5.0 + randf() * 60.0
		var spread_x: float = (randf() - 0.5) * 8.0
		var spread_z: float = (randf() - 0.5) * 8.0
		var pos: Vector3 = Vector3(spread_x, height, spread_z)

		var rot: Basis = Basis()
		rot = rot.rotated(Vector3.RIGHT, randf() * TAU)
		rot = rot.rotated(Vector3.UP, randf() * TAU)

		var body: RigidBody3D = RigidBody3D.new()
		body.transform = Transform3D(rot, pos)

		var col: CollisionShape3D = CollisionShape3D.new()
		col.shape = box_shape
		body.add_child(col)

		if not use_multimesh:
			# Individual MeshInstance3D per body — maximum node count.
			var mesh_inst: MeshInstance3D = MeshInstance3D.new()
			mesh_inst.mesh = box_mesh
			body.add_child(mesh_inst)

		add_child(body)
		_bodies.append(body)

	# --- MultiMesh for rendering (if enabled) ---
	if use_multimesh:
		var mm: MultiMesh = MultiMesh.new()
		mm.transform_format = MultiMesh.TRANSFORM_3D
		mm.instance_count = count
		mm.mesh = box_mesh

		for i: int in range(count):
			mm.set_instance_transform(i, _bodies[i].transform)

		_multimesh_instance = MultiMeshInstance3D.new()
		_multimesh_instance.multimesh = mm
		add_child(_multimesh_instance)

	var node_count: int = _count_nodes(self)
	label.text = "Standard Godot: %d instances\n%d scene tree nodes" % [count, node_count]


func _physics_process(_delta: float) -> void:
	# Sync MultiMesh transforms from rigid bodies.
	if _multimesh_instance and _multimesh_instance.multimesh:
		var mm: MultiMesh = _multimesh_instance.multimesh
		for i: int in range(_bodies.size()):
			mm.set_instance_transform(i, _bodies[i].transform)


func _process(delta: float) -> void:
	# Reset on Enter.
	if Input.is_action_just_pressed("ui_accept"):
		get_tree().reload_current_scene()



func _count_nodes(p_node: Node) -> int:
	var total: int = 1
	for i: int in range(p_node.get_child_count()):
		total += _count_nodes(p_node.get_child(i))
	return total
