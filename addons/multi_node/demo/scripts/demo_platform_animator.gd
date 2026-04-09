extends MultiNodeAnimator
## Animates kinematic platforms with a vertical bob using MultiAnimation keys.
## Tests additive transform properties — each platform bobs from its own base position.

func _ready() -> void:
	var ma: MultiAnimation = MultiAnimation.new()
	ma.play_mode = MultiAnimation.PLAY_LOOP
	ma.default_duration = 1.5
	ma.default_trans_type = MultiAnimation.TRANS_CUBIC
	ma.default_easing = MultiAnimation.EASE_IN_OUT

	# Four position keys forming a bob cycle.
	ma.add_key("center")
	ma.set_key_property(0, &"position", Vector3(0.0, 0.0, 0.0))

	ma.add_key("up")
	ma.set_key_property(1, &"position", Vector3(0.0, 8.0, 0.0))

	ma.add_key("center2")
	ma.set_key_property(2, &"position", Vector3(0.0, 0.0, 0.0))

	ma.add_key("down")
	ma.set_key_property(3, &"position", Vector3(0.0, -4.0, 0.0))

	# AUTO transitions: 0→1→2→3→0 (loop chain).
	ma.add_transition(0, 1)  # center → up
	ma.add_transition(1, 2)  # up → center2
	ma.add_transition(2, 3)  # center2 → down
	ma.add_transition(3, 0)  # down → center (loop closure)

	animation_resource = ma
