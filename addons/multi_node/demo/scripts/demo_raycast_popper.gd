extends MultiNodeRaycast
## Raycast popper — instances that detect a surface below them pop upward.
## Uses instance_data channel 0 as cooldown timer.
## Fires the notification sound on each pop.
## Throttled to check every 2 frames — pops still feel responsive.

@export var pop_force: float = 20.0
@export var cooldown_time: float = 3.0
@export var check_interval: int = 2

var _parent: MultiNode = null
var _audio_notify: MultiNodeAudio = null
var _frame_counter: int = 0
var _accumulated_delta: float = 0.0


func _ready() -> void:
	_parent = get_multi_node_parent()
	if not _parent:
		return
	# Find the notification audio sibling.
	var notify_node: Node = _parent.get_node_or_null("MultiNodeAudioNotify")
	if notify_node is MultiNodeAudio:
		_audio_notify = notify_node as MultiNodeAudio


func _process(delta: float) -> void:
	if not _parent or get_active_count() == 0:
		return

	_accumulated_delta += delta
	_frame_counter += 1
	if _frame_counter < check_interval:
		return
	_frame_counter = 0
	var dt: float = _accumulated_delta
	_accumulated_delta = 0.0

	var pop_dir: Vector3 = -cast_direction.normalized()
	var count: int = _parent.instance_count

	for i: int in range(count):
		if not is_instance_active(i):
			continue

		var cd: float = get_instance_data(i, 0)
		if cd > 0.0:
			set_instance_data(i, 0, cd - dt)
			continue

		if is_instance_colliding(i):
			var dir: Vector3 = pop_dir
			if use_instance_basis:
				dir = _parent.get_instance_transform(i).basis * pop_dir
			_parent.set_linear_velocity(i, dir * pop_force)
			set_instance_data(i, 0, cooldown_time)
			# Fire notification sound for this instance.
			if _audio_notify:
				_audio_notify.play_instance(i)
