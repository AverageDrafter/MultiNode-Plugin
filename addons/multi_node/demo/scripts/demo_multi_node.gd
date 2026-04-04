extends MultiNode
## MultiNode root — spawns instances and manages world bounds.
## All node-specific behavior lives on each sub-node's own .gd script.

@export var count: int = 500

const WORLD_MIN: Vector3 = Vector3(-200.0, -50.0, -200.0)
const WORLD_MAX: Vector3 = Vector3(200.0, 200.0, 200.0)

var _colliders: Array = []


func _ready() -> void:
	seed(Time.get_ticks_msec())
	for child: Node in get_children():
		if child is MultiNodeCollider:
			_colliders.append(child)

	begin_batch()
	for i: int in range(count):
		var height: float = 5.0 + randf() * 60.0
		var spread_x: float = (randf() - 0.5) * 8.0
		var spread_z: float = (randf() - 0.5) * 8.0
		var pos: Vector3 = Vector3(spread_x, height, spread_z)
		var rot: Basis = Basis()
		rot = rot.rotated(Vector3.RIGHT, randf() * TAU)
		rot = rot.rotated(Vector3.UP, randf() * TAU)
		add_instance(Transform3D(rot, pos))
	end_batch()


func _physics_process(_delta: float) -> void:
	if instance_count == 0:
		return
	var positions: PackedVector3Array = get_all_positions()
	for i: int in range(instance_count):
		var pos: Vector3 = positions[i]
		var clamped: Vector3 = pos.clamp(WORLD_MIN, WORLD_MAX)
		if clamped != pos:
			var xform: Transform3D = Transform3D(get_instance_transform(i).basis, clamped)
			set_instance_transform(i, xform)
			var world_xform: Transform3D = global_transform * xform
			for collider: MultiNodeCollider in _colliders:
				var body_rid: RID = collider.get_body_rid(i)
				if body_rid.is_valid():
					PhysicsServer3D.body_set_state(body_rid, PhysicsServer3D.BODY_STATE_TRANSFORM, world_xform)
					PhysicsServer3D.body_set_state(body_rid, PhysicsServer3D.BODY_STATE_LINEAR_VELOCITY, Vector3.ZERO)
					PhysicsServer3D.body_set_state(body_rid, PhysicsServer3D.BODY_STATE_ANGULAR_VELOCITY, Vector3.ZERO)
