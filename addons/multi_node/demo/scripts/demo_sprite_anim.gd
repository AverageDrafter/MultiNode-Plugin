extends MultiNodeSprite
## Sprite walk cycle animation driven by cube rotational speed.
## Uses instance_data channel 0 as per-instance animation accumulator.
## Reads angular velocity from the parent's velocity bus — works across collider swaps.

const BASE_FRAME_TIME: float = 0.12
const MAX_SPEED_MULT: float = 2.0
const ANGULAR_SPEED_REF: float = 8.0
const DEAD_ZONE: float = 0.5  ## Ignore angular velocity below this (rad/s).

var _parent: MultiNode = null


func _ready() -> void:
	_parent = get_multi_node_parent()
	if not _parent:
		return

	if _parent.instance_count > 0:
		_stagger_frames()
	else:
		_parent.instances_changed.connect(_stagger_frames, CONNECT_ONE_SHOT)


func _stagger_frames() -> void:
	if _parent:
		for i: int in range(_parent.instance_count):
			set_instance_frame(i, i % 8)


func _process(delta: float) -> void:
	if not _parent or get_active_count() == 0:
		return

	var count: int = _parent.instance_count
	for i: int in range(count):
		if not is_instance_active(i):
			continue

		var angular_vel: Vector3 = _parent.get_instance_angular_velocity(i)
		var spin_speed: float = angular_vel.length()

		# Dead zone: ignore tiny residual spin from pre-sleep settling.
		if spin_speed < DEAD_ZONE:
			continue

		var speed_mult: float = clampf(spin_speed / ANGULAR_SPEED_REF, 0.0, 1.0) * MAX_SPEED_MULT

		var accum: float = get_instance_data(i, 0) + delta * speed_mult
		if accum >= BASE_FRAME_TIME:
			accum -= BASE_FRAME_TIME
			var frame: int = (get_instance_frame(i) + 1) % 8
			set_instance_frame(i, frame)
		set_instance_data(i, 0, accum)
