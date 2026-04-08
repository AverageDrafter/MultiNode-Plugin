extends Node3D
## Baseline test scene — MultiNode with mesh + collider only.
## Perf overlay and hover detection. No raycast/area/audio dependencies.

@onready var multi_node: MultiNode = $MultiNode
@onready var mesh_node: MultiNodeMesh = $MultiNode/MultiNodeMesh
@onready var collider_node: MultiNodeCollider = $MultiNode/MultiNodeCollider
@onready var camera: Camera3D = $Camera3D
@onready var label: Label = $UI/Label
var _hovered_index: int = -1


func _ready() -> void:
	if Engine.has_singleton("DebugMenu"):
		Engine.get_singleton("DebugMenu").style = 2
	label.text = "MultiNode Baseline: %d instances (mesh + collider only)" % multi_node.instance_count


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_accept"):
		multi_node.clear_instances()
		multi_node._ready()
		label.text = "MultiNode Baseline: %d instances (mesh + collider only)" % multi_node.instance_count


func _physics_process(_delta: float) -> void:
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

	var lines: PackedStringArray = PackedStringArray()
	lines.push_back("MultiNode Baseline: %d instances" % multi_node.instance_count)
	if _hovered_index >= 0:
		lines.push_back("Hovered: Instance %d" % _hovered_index)
	label.text = "\n".join(lines)
