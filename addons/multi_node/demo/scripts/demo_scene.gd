extends Node3D
## Test scene root: handles camera, UI, hover detection, and displays
## raycast/area debug info from the new MultiNode sub-nodes.

@onready var multi_node: MultiNode = $MultiNode
@onready var mesh_node: MultiNodeMesh = $MultiNode/MultiNodeMesh
@onready var collider_node: MultiNodeCollider = $MultiNode/MultiNodeCollider
@onready var raycast_node: MultiNodeRaycast = $MultiNode/MultiNodeRaycast
@onready var area_node: MultiNodeArea = $MultiNode/MultiNodeArea
@onready var camera: Camera3D = $Camera3D
@onready var label: Label = $UI/Label
var _hovered_index: int = -1


func _ready() -> void:
	DebugMenu.style = DebugMenu.Style.VISIBLE_DETAILED
	label.text = "MultiNode: %d instances" % multi_node.instance_count


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_accept"):
		_hovered_index = -1
		multi_node.clear_instances()
		multi_node._ready()
		for i: int in range(multi_node.instance_count):
			mesh_node.set_instance_color(i, Color.WHITE)


func _physics_process(_delta: float) -> void:
	# --- Hover detection via mouse raycast ---
	var mouse_pos: Vector2 = get_viewport().get_mouse_position()
	var from: Vector3 = camera.project_ray_origin(mouse_pos)
	var to: Vector3 = from + camera.project_ray_normal(mouse_pos) * 1000.0

	var space_state: PhysicsDirectSpaceState3D = get_world_3d().direct_space_state
	var query: PhysicsRayQueryParameters3D = PhysicsRayQueryParameters3D.create(from, to)
	var result: Dictionary = space_state.intersect_ray(query)

	if _hovered_index >= 0:
		mesh_node.set_instance_color(_hovered_index, Color.WHITE)
		_hovered_index = -1

	if result:
		var body_rid: RID = result.get("rid", RID())
		var index: int = collider_node.get_instance_from_body(body_rid)
		if index >= 0:
			mesh_node.set_instance_color(index, Color.RED)
			_hovered_index = index

	# --- Build info text ---
	var lines: PackedStringArray = PackedStringArray()
	lines.push_back("MultiNode: %d instances" % multi_node.instance_count)

	# Hover info.
	if _hovered_index >= 0:
		var idx: int = _hovered_index
		lines.push_back("Hovered: Instance %d" % idx)

		# Show raycast ground distance if this instance has one.
		if raycast_node and raycast_node.is_instance_colliding(idx):
			var dist: float = raycast_node.get_instance_hit_distance(idx)
			lines.push_back("  Ground distance: %.1f m" % dist)
		elif raycast_node and raycast_node.is_instance_active(idx):
			lines.push_back("  Ground: no hit")

	# Count grounded instances from raycast.
	if raycast_node and raycast_node.get_active_count() > 0:
		var grounded: int = 0
		var total_active: int = 0
		for i: int in range(multi_node.instance_count):
			if raycast_node.is_instance_active(i):
				total_active += 1
				if raycast_node.is_instance_colliding(i):
					grounded += 1
		lines.push_back("Raycast: %d/%d grounded" % [grounded, total_active])

	# Area info — count how many areas are active.
	if area_node:
		lines.push_back("Areas: %d active" % area_node.get_active_count())

	label.text = "\n".join(lines)
