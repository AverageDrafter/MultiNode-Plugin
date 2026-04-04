extends PanelContainer
## Bottom control panel — SpinBox for each sub-node's instance_every
## and the parent's instance count. Built dynamically from the scene tree.
## Wraps into multiple rows. Enter releases focus back to the scene.

var _multi_node: MultiNode = null
var _count_spin: SpinBox = null
var _node_spins: Array[Dictionary] = []  # [{node: MultiNodeSub, spin: SpinBox}]
var _flow: GridContainer = null


func _ready() -> void:
	_multi_node = get_node_or_null("/root/TestScene/MultiNode") as MultiNode
	if not _multi_node:
		return

	# Panel itself should never grab focus.
	focus_mode = Control.FOCUS_NONE
	mouse_filter = Control.MOUSE_FILTER_PASS

	# Style the panel.
	var style: StyleBoxFlat = StyleBoxFlat.new()
	style.bg_color = Color(0.08, 0.08, 0.1, 0.85)
	style.content_margin_left = 8.0
	style.content_margin_right = 8.0
	style.content_margin_top = 4.0
	style.content_margin_bottom = 4.0
	add_theme_stylebox_override("panel", style)

	# Use a GridContainer — calculate columns based on viewport width.
	_flow = GridContainer.new()
	_flow.add_theme_constant_override("h_separation", 10)
	_flow.add_theme_constant_override("v_separation", 4)
	add_child(_flow)

	# We'll set column count after adding all controls.
	var controls: Array[VBoxContainer] = []

	# Instance count control.
	controls.append(_make_control("Count", 0, 10000, _multi_node.instance_count, _on_count_changed))

	# One SpinBox per sub-node.
	for i: int in range(_multi_node.get_child_count()):
		var child: Node = _multi_node.get_child(i)
		if child is MultiNodeSub:
			var sub: MultiNodeSub = child as MultiNodeSub
			var short_name: String = child.name.replace("MultiNode", "")
			if short_name.is_empty():
				short_name = child.name
			controls.append(_make_control(short_name, -1000, 1000, sub.instance_every, _on_nth_changed, sub))

	# Reset button.
	controls.append(_make_reset_button())

	# Aim for roughly 8 per row, adjust to fit.
	var cols: int = mini(controls.size(), 9)
	_flow.columns = cols

	for ctrl: VBoxContainer in controls:
		_flow.add_child(ctrl)


func _make_control(p_label: String, p_min: int, p_max: int, p_value: int, p_callback: Callable, p_sub: MultiNodeSub = null) -> VBoxContainer:
	var vbox: VBoxContainer = VBoxContainer.new()

	var lbl: Label = Label.new()
	lbl.text = p_label
	lbl.add_theme_font_size_override("font_size", 11)
	lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vbox.add_child(lbl)

	var spin: SpinBox = SpinBox.new()
	spin.min_value = p_min
	spin.max_value = p_max
	spin.value = p_value
	spin.step = 1
	spin.custom_minimum_size.x = 75.0
	spin.add_theme_font_size_override("font_size", 12)
	spin.focus_mode = Control.FOCUS_NONE
	var line_edit: LineEdit = spin.get_line_edit()
	line_edit.focus_mode = Control.FOCUS_CLICK  # Only focus when clicked, not tabbed to.
	line_edit.gui_input.connect(_on_spin_input.bind(spin))
	# Block the up/down arrow buttons from grabbing focus too.
	for child_node: Node in spin.get_children():
		if child_node is BaseButton:
			(child_node as BaseButton).focus_mode = Control.FOCUS_NONE
	vbox.add_child(spin)

	if p_label == "Count":
		_count_spin = spin
		spin.value_changed.connect(_on_count_changed)
	else:
		spin.value_changed.connect(_on_nth_changed.bind(p_sub))
		_node_spins.append({"node": p_sub, "spin": spin})

	return vbox


func _make_reset_button() -> VBoxContainer:
	var vbox: VBoxContainer = VBoxContainer.new()

	var lbl: Label = Label.new()
	lbl.text = "Reset"
	lbl.add_theme_font_size_override("font_size", 11)
	lbl.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vbox.add_child(lbl)

	var btn: Button = Button.new()
	btn.text = "Reset"
	btn.custom_minimum_size.x = 75.0
	btn.add_theme_font_size_override("font_size", 12)
	btn.focus_mode = Control.FOCUS_NONE
	btn.pressed.connect(_on_reset_pressed)
	vbox.add_child(btn)

	return vbox


func _on_spin_input(event: InputEvent, spin: SpinBox) -> void:
	if event is InputEventKey:
		var key_event: InputEventKey = event as InputEventKey
		if key_event.pressed:
			# Enter, Tab, Space, Escape — all release focus back to scene.
			if key_event.keycode in [KEY_ENTER, KEY_TAB, KEY_SPACE, KEY_ESCAPE]:
				spin.get_line_edit().release_focus()
				get_viewport().set_input_as_handled()


func _on_count_changed(value: float) -> void:
	if not _multi_node:
		return
	_multi_node.set("count", int(value))
	_multi_node.clear_instances()
	_multi_node._ready()


func _on_nth_changed(value: float, sub: MultiNodeSub) -> void:
	if sub:
		sub.instance_every = int(value)


func _on_reset_pressed() -> void:
	if not _multi_node:
		return
	_multi_node.clear_instances()
	_multi_node._ready()
	# Sync the count spinbox.
	if _count_spin:
		_count_spin.set_value_no_signal(_multi_node.instance_count)
