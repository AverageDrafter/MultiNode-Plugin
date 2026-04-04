extends MultiNodeAnimator
## Animates kinematic platforms with a position bob track.
## Tests additive transform tracks — each platform bobs from its own base position.

func _ready() -> void:
	var anim: Animation = Animation.new()
	anim.length = 6.0
	anim.loop_mode = Animation.LOOP_LINEAR

	# Position track — vertical bob. Applied additively to each instance's base position.
	var pos_track: int = anim.add_track(Animation.TYPE_POSITION_3D)
	anim.track_set_path(pos_track, ".")
	anim.track_set_interpolation_type(pos_track, Animation.INTERPOLATION_CUBIC)
	anim.position_track_insert_key(pos_track, 0.0, Vector3(0.0, 0.0, 0.0))
	anim.position_track_insert_key(pos_track, 1.5, Vector3(0.0, 8.0, 0.0))
	anim.position_track_insert_key(pos_track, 3.0, Vector3(0.0, 0.0, 0.0))
	anim.position_track_insert_key(pos_track, 4.5, Vector3(0.0, -4.0, 0.0))

	animation = anim
