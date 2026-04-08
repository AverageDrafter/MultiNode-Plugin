extends Camera3D
## Simple fly camera: WASD + QE for up/down, mouse look with right-click held.

@export var move_speed: float = 20.0
@export var look_sensitivity: float = 0.003
@export var sprint_multiplier: float = 3.0

var _looking: bool = false


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		var mb: InputEventMouseButton = event as InputEventMouseButton
		if mb.button_index == MOUSE_BUTTON_RIGHT:
			_looking = mb.pressed
			Input.mouse_mode = Input.MOUSE_MODE_CAPTURED if _looking else Input.MOUSE_MODE_VISIBLE

	if event is InputEventMouseMotion and _looking:
		var mm: InputEventMouseMotion = event as InputEventMouseMotion
		rotate_y(-mm.relative.x * look_sensitivity)
		rotate_object_local(Vector3.RIGHT, -mm.relative.y * look_sensitivity)


func _process(delta: float) -> void:
	var speed: float = move_speed
	if Input.is_key_pressed(KEY_SHIFT):
		speed *= sprint_multiplier

	var dir: Vector3 = Vector3.ZERO
	if Input.is_key_pressed(KEY_W):
		dir -= transform.basis.z
	if Input.is_key_pressed(KEY_S):
		dir += transform.basis.z
	if Input.is_key_pressed(KEY_A):
		dir -= transform.basis.x
	if Input.is_key_pressed(KEY_D):
		dir += transform.basis.x
	if Input.is_key_pressed(KEY_E):
		dir += Vector3.UP
	if Input.is_key_pressed(KEY_Q):
		dir += Vector3.DOWN

	if dir.length_squared() > 0.0:
		position += dir.normalized() * speed * delta
