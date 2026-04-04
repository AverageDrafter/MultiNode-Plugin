extends MultiNodeLabel
## Sets instance text to the instance index number.
## Uses a one-shot connection so texts are applied once after instances exist.

func _ready() -> void:
	var parent: MultiNode = get_multi_node_parent()
	if not parent:
		return
	if parent.instance_count > 0:
		_apply_texts()
	else:
		parent.instances_changed.connect(_apply_texts, CONNECT_ONE_SHOT)


func _apply_texts() -> void:
	var parent: MultiNode = get_multi_node_parent()
	if not parent:
		return
	for i: int in range(parent.instance_count):
		set_instance_text(i, str(i))
