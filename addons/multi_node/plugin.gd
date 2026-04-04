@tool
extends EditorPlugin

# ---------------------------------------------------------------------------
# State
# ---------------------------------------------------------------------------
var _inspector_plugin: EditorInspectorPlugin
var _active_multi_node: Node = null

# Highlight colors.
var _group_color: Color = Color(1.0, 0.7, 0.3, 1.0)    # Light orange — all instances.
var _selected_color: Color = Color(0.6, 0.3, 1.0, 1.0)  # Purple — selected instance.
var _last_highlighted: int = -1
var _group_highlighted: bool = false
var _colors_were_enabled: bool = false

# ---------------------------------------------------------------------------
# Lifecycle
# ---------------------------------------------------------------------------
func _enter_tree() -> void:
	_inspector_plugin = _InspPlugin.new()
	add_inspector_plugin(_inspector_plugin)
	set_process(true)

func _exit_tree() -> void:
	_clear_all_highlights()
	remove_inspector_plugin(_inspector_plugin)
	_inspector_plugin = null

# ---------------------------------------------------------------------------
# Plugin activation — which objects we handle
# ---------------------------------------------------------------------------
func _handles(object: Object) -> bool:
	if not object is Node:
		return false
	var node: Node = object as Node
	# Handle MultiNode directly.
	if node.is_class("MultiNode"):
		return true
	# Handle direct children of MultiNode (Mesh, Collider, InstanceGen, etc.)
	var parent: Node = node.get_parent()
	if parent and parent.is_class("MultiNode"):
		return true
	return false

func _edit(object: Object) -> void:
	_clear_all_highlights()
	if not object or not (object is Node):
		_active_multi_node = null
		return
	var node: Node = object as Node
	if node.is_class("MultiNode"):
		_active_multi_node = node
	else:
		var parent: Node = node.get_parent()
		if parent and parent.is_class("MultiNode"):
			_active_multi_node = parent
		else:
			_active_multi_node = null
			return
	# Defer highlight to next frame so the editor has time to set things up.
	call_deferred("_apply_group_highlight")

func _make_visible(visible: bool) -> void:
	if not visible:
		_clear_all_highlights()
		_active_multi_node = null

# ---------------------------------------------------------------------------
# Helpers — find children by class
# ---------------------------------------------------------------------------
func _find_mesh_child() -> Node:
	if not _active_multi_node or not is_instance_valid(_active_multi_node):
		return null
	for c: int in range(_active_multi_node.get_child_count()):
		var child: Node = _active_multi_node.get_child(c)
		if child.is_class("MultiNodeMesh"):
			return child
	return null

func _find_manual_gen() -> Node:
	if not _active_multi_node or not is_instance_valid(_active_multi_node):
		return null
	for c: int in range(_active_multi_node.get_child_count()):
		var child: Node = _active_multi_node.get_child(c)
		if child.is_class("MultiNodeInstancer"):
			if int(child.get("gen_mode")) == 2:  # GEN_MANUAL
				return child
	return null

# ---------------------------------------------------------------------------
# Color management — enable instance colors for highlighting
# ---------------------------------------------------------------------------
func _ensure_colors_enabled() -> void:
	var mesh_child: Node = _find_mesh_child()
	if not mesh_child:
		return
	_colors_were_enabled = bool(mesh_child.get("use_instance_colors"))
	if not _colors_were_enabled:
		# This triggers _rebuild() internally which re-allocates with color
		# storage and does a full buffer upload.
		mesh_child.set("use_instance_colors", true)

func _restore_colors_state() -> void:
	if _colors_were_enabled:
		return
	var mesh_child: Node = _find_mesh_child()
	if mesh_child:
		mesh_child.set("use_instance_colors", false)

# ---------------------------------------------------------------------------
# Group highlight — light orange for all instances
# ---------------------------------------------------------------------------
func _apply_group_highlight() -> void:
	if not _active_multi_node or not is_instance_valid(_active_multi_node):
		return
	var count: int = int(_active_multi_node.get("instance_count"))
	if count == 0:
		return
	_ensure_colors_enabled()
	var mesh_child: Node = _find_mesh_child()
	if not mesh_child:
		return
	for i: int in range(count):
		mesh_child.call("set_instance_color", i, _group_color)
	_group_highlighted = true

func _clear_all_highlights() -> void:
	if not _active_multi_node or not is_instance_valid(_active_multi_node):
		_group_highlighted = false
		_last_highlighted = -1
		return
	var mesh_child: Node = _find_mesh_child()
	if mesh_child:
		var count: int = int(_active_multi_node.get("instance_count"))
		for i: int in range(count):
			mesh_child.call("set_instance_color", i, Color.WHITE)
	_restore_colors_state()
	_group_highlighted = false
	_last_highlighted = -1

# ---------------------------------------------------------------------------
# Selected instance highlight — purple for specific manual instance
# ---------------------------------------------------------------------------
func _update_selected_highlight() -> void:
	var gen: Node = _find_manual_gen()
	if not gen:
		if _last_highlighted >= 0:
			_restore_instance_to_group(_last_highlighted)
			_last_highlighted = -1
		return
	var idx: int = int(gen.call("get_editor_selected_instance"))
	if idx < 0:
		if _last_highlighted >= 0:
			_restore_instance_to_group(_last_highlighted)
			_last_highlighted = -1
		return
	var count: int = int(_active_multi_node.get("instance_count"))
	if idx >= count:
		return
	var mesh_child: Node = _find_mesh_child()
	if not mesh_child:
		return
	if _last_highlighted >= 0 and _last_highlighted != idx and _last_highlighted < count:
		mesh_child.call("set_instance_color", _last_highlighted, _group_color)
	mesh_child.call("set_instance_color", idx, _selected_color)
	_last_highlighted = idx

func _restore_instance_to_group(idx: int) -> void:
	var mesh_child: Node = _find_mesh_child()
	if not mesh_child:
		return
	var count: int = int(_active_multi_node.get("instance_count"))
	if idx >= 0 and idx < count:
		mesh_child.call("set_instance_color", idx, _group_color)

# ---------------------------------------------------------------------------
# Process — maintain highlights + overlays
# ---------------------------------------------------------------------------
func _process(_delta: float) -> void:
	if not _active_multi_node or not is_instance_valid(_active_multi_node):
		return
	# Re-apply group highlight if a sync wiped our colors.
	if _group_highlighted:
		var mesh_child: Node = _find_mesh_child()
		if mesh_child:
			var count: int = int(_active_multi_node.get("instance_count"))
			if count > 0:
				var c: Color = mesh_child.call("get_instance_color", 0)
				if c != _group_color and c != _selected_color:
					_apply_group_highlight()
					if _last_highlighted >= 0:
						_update_selected_highlight()
	# Track manual selection changes.
	var gen: Node = _find_manual_gen()
	if gen:
		var idx: int = int(gen.call("get_editor_selected_instance"))
		if idx != _last_highlighted:
			_update_selected_highlight()

# ---------------------------------------------------------------------------
# 3D input — click to pick instance in manual mode
# ---------------------------------------------------------------------------
func _forward_3d_gui_input(camera: Camera3D, event: InputEvent) -> int:
	if not _active_multi_node or not is_instance_valid(_active_multi_node):
		return EditorPlugin.AFTER_GUI_INPUT_PASS
	if not (event is InputEventMouseButton):
		return EditorPlugin.AFTER_GUI_INPUT_PASS
	var mb: InputEventMouseButton = event as InputEventMouseButton
	if mb.button_index != MOUSE_BUTTON_LEFT or not mb.pressed:
		return EditorPlugin.AFTER_GUI_INPUT_PASS

	var gen: Node = _find_manual_gen()
	if not gen:
		return EditorPlugin.AFTER_GUI_INPUT_PASS

	var count: int = int(_active_multi_node.get("instance_count"))
	if count == 0:
		return EditorPlugin.AFTER_GUI_INPUT_PASS

	var positions: PackedVector3Array = _active_multi_node.call("get_all_positions")
	var global_xform: Transform3D = _active_multi_node.global_transform
	var click: Vector2 = mb.position
	var best_idx: int = -1
	var best_dist_sq: float = 40.0 * 40.0

	for i: int in range(count):
		var world_pos: Vector3 = global_xform * positions[i]
		if camera.is_position_behind(world_pos):
			continue
		var screen_pos: Vector2 = camera.unproject_position(world_pos)
		var d_sq: float = click.distance_squared_to(screen_pos)
		if d_sq < best_dist_sq:
			best_dist_sq = d_sq
			best_idx = i

	if best_idx >= 0:
		gen.call("set_editor_selected_instance", best_idx)
		_update_selected_highlight()
		return EditorPlugin.AFTER_GUI_INPUT_STOP
	return EditorPlugin.AFTER_GUI_INPUT_PASS


# ===========================================================================
# Inspector plugin — quick-add toolbar on MultiNode, collider creator on Mesh
# ===========================================================================
class _InspPlugin extends EditorInspectorPlugin:

	func _can_handle(object: Object) -> bool:
		if object.is_class("MultiNode"):
			return true
		if object.is_class("MultiNodeMesh"):
			return true
		return false

	func _parse_end(object: Object) -> void:
		if object.is_class("MultiNode"):
			add_custom_control(_QuickAdd.new(object))
		if object.is_class("MultiNodeMesh"):
			add_custom_control(_CreateCollider.new(object))

	# -------------------------------------------------------------------
	# Quick-add toolbar on MultiNode parent
	# -------------------------------------------------------------------
	class _QuickAdd extends VBoxContainer:
		var _node: Object

		func _init(node: Object) -> void:
			_node = node

		func _ready() -> void:
			add_child(HSeparator.new())
			var header: Label = Label.new()
			header.text = "Quick Add Sub-Node"
			header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			header.add_theme_font_size_override("font_size", 12)
			add_child(header)

			var row1: HBoxContainer = HBoxContainer.new()
			add_child(row1)
			for type_name: String in ["Instancer", "Mesh", "Collider"]:
				var btn: Button = Button.new()
				btn.text = type_name
				btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
				btn.pressed.connect(_add_child_node.bind("MultiNode" + type_name))
				row1.add_child(btn)

			var row2: HBoxContainer = HBoxContainer.new()
			add_child(row2)
			for type_name: String in ["Area", "Audio", "Raycast"]:
				var btn: Button = Button.new()
				btn.text = type_name
				btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
				btn.pressed.connect(_add_child_node.bind("MultiNode" + type_name))
				row2.add_child(btn)

			var row3: HBoxContainer = HBoxContainer.new()
			add_child(row3)
			for type_name: String in ["Animator", "Label", "Particle"]:
				var btn: Button = Button.new()
				btn.text = type_name
				btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
				btn.pressed.connect(_add_child_node.bind("MultiNode" + type_name))
				row3.add_child(btn)

			var row4: HBoxContainer = HBoxContainer.new()
			add_child(row4)
			for type_name: String in ["Sprite", "Decal", "Light"]:
				var btn: Button = Button.new()
				btn.text = type_name
				btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
				btn.pressed.connect(_add_child_node.bind("MultiNode" + type_name))
				row4.add_child(btn)

		func _add_child_node(class_name_str: String) -> void:
			if not is_instance_valid(_node):
				return
			var parent: Node = _node as Node
			var child: Node = ClassDB.instantiate(class_name_str)
			if not child:
				push_warning("Could not instantiate " + class_name_str)
				return
			child.name = class_name_str.replace("MultiNode", "")

			var ur: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
			ur.create_action("Add " + class_name_str)
			ur.add_do_method(parent, "add_child", child)
			ur.add_do_property(child, "owner", parent.owner if parent.owner else parent)
			ur.add_do_reference(child)
			ur.add_undo_method(parent, "remove_child", child)
			ur.commit_action()

	# -------------------------------------------------------------------
	# Create Collider from Mesh
	# -------------------------------------------------------------------
	class _CreateCollider extends VBoxContainer:
		var _node: Object

		func _init(node: Object) -> void:
			_node = node

		func _ready() -> void:
			add_child(HSeparator.new())
			var header: Label = Label.new()
			header.text = "Create Collider"
			header.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			header.add_theme_font_size_override("font_size", 12)
			add_child(header)

			var btn_row: HBoxContainer = HBoxContainer.new()
			add_child(btn_row)
			for mode: String in ["Convex", "Trimesh", "Box"]:
				var btn: Button = Button.new()
				btn.text = mode
				btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
				btn.pressed.connect(_create.bind(mode.to_lower()))
				btn_row.add_child(btn)

		func _create(mode: String) -> void:
			if not is_instance_valid(_node): return
			var mesh_node: Node = _node as Node
			var mesh: Variant = mesh_node.get("mesh")
			if not mesh is Mesh:
				push_warning("No mesh assigned.")
				return
			var m: Mesh = mesh as Mesh

			var shape: Shape3D
			match mode:
				"convex": shape = m.create_convex_shape()
				"trimesh": shape = m.create_trimesh_shape()
				"box":
					var box: BoxShape3D = BoxShape3D.new()
					box.size = m.get_aabb().size
					shape = box
			if not shape: return

			var parent: Node = mesh_node.get_parent()
			if not parent: return

			var collider: Node = ClassDB.instantiate("MultiNodeCollider")
			var base_name: String = mesh_node.name.replace("Mesh", "Collider")
			if base_name == mesh_node.name:
				base_name = mesh_node.name + "Collider"
			collider.name = base_name
			collider.set("shape", shape)

			var ur: EditorUndoRedoManager = EditorInterface.get_editor_undo_redo()
			ur.create_action("Create Collider from Mesh")
			ur.add_do_method(parent, "add_child", collider)
			ur.add_do_property(collider, "owner", mesh_node.owner)
			ur.add_do_reference(collider)
			ur.add_undo_method(parent, "remove_child", collider)
			ur.commit_action()
