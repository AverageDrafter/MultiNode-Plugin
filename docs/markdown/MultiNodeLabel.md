# MultiNodeLabel

**Inherits:** [MultiNodeVisual3D](MultiNodeVisual3D.md)

Billboard text labels rendered above instances using a shared text atlas.

## Description

Renders text labels using GPU-instanced billboard quads. All labels share a single text atlas texture generated via an internal SubViewport, drawn in one MultiMesh draw call. This makes it possible to display unique text over thousands of instances with minimal draw call overhead.

Per-instance text is set via `set_instance_text()`. Setting an empty string hides the label for that instance. The atlas is rebuilt automatically when any text changes. For bulk text updates, use `begin_text_batch()` and `end_text_batch()` to suppress atlas rebuilds until all changes are applied.

Labels use a Y-locked billboard shader -- they always face the camera horizontally but remain upright (no tilt). The label quad is positioned at each instance's world position with the MultiNode3D offset applied. The `label_height` property controls the world-space height of the label quad; width scales automatically based on text length.

Text rendering uses the assigned Font resource (or the default system font if none is set). Outline rendering provides readability against any background. The atlas is rendered at `font_size * render_scale` resolution for crisp text at typical viewing distances.

Labels beyond `max_label_distance` from the camera are hidden to avoid rendering illegible text and to save GPU fill rate.

Inherits per-instance color tinting from MultiNodeVisual3D. The `tint` property and per-instance colors multiply the `text_color`.

## Properties

| Name | Type | Default |
|------|------|---------|
| [font](#font) | Font | `null` |
| [font_size](#font_size) | int | `32` |
| [text_color](#text_color) | Color | `Color(1, 1, 1, 1)` |
| [outline_color](#outline_color) | Color | `Color(0, 0, 0, 1)` |
| [outline_size](#outline_size) | int | `4` |
| [label_height](#label_height) | float | `0.5` |
| [cell_padding](#cell_padding) | int | `8` |
| [render_scale](#render_scale) | float | `2.0` |
| [max_label_distance](#max_label_distance) | float | `100.0` |

## Property Descriptions

### font

```
Font font = null
```

- *Setter:* `set_font(value: Font)`
- *Getter:* `get_font() -> Font`

The font used for rendering label text. Accepts any Godot Font resource: SystemFont, FontFile (TTF/OTF), or BitmapFont. If `null`, the default theme font is used.

Changing the font triggers an atlas rebuild.

---

### font_size

```
int font_size = 32
```

- *Setter:* `set_font_size(value: int)`
- *Getter:* `get_font_size() -> int`

The font size in pixels used when rendering text to the atlas. Larger values produce sharper text but increase atlas texture size. The final rendering resolution is `font_size * render_scale`.

---

### text_color

```
Color text_color = Color(1, 1, 1, 1)
```

- *Setter:* `set_text_color(value: Color)`
- *Getter:* `get_text_color() -> Color`

The base color of the label text. This is baked into the atlas texture. Per-instance color tinting (from MultiNodeVisual3D) is multiplied on top.

Changing this triggers an atlas rebuild.

---

### outline_color

```
Color outline_color = Color(0, 0, 0, 1)
```

- *Setter:* `set_outline_color(value: Color)`
- *Getter:* `get_outline_color() -> Color`

The color of the text outline. A dark outline improves readability against bright or varied backgrounds. Set alpha to `0.0` to disable the outline visually.

Changing this triggers an atlas rebuild.

---

### outline_size

```
int outline_size = 4
```

- *Setter:* `set_outline_size(value: int)`
- *Getter:* `get_outline_size() -> int`

The thickness of the text outline in pixels. Larger values produce a thicker border around each character. Set to `0` to disable outlines entirely.

Changing this triggers an atlas rebuild.

---

### label_height

```
float label_height = 0.5
```

- *Setter:* `set_label_height(value: float)`
- *Getter:* `get_label_height() -> float`

The world-space height of each label quad in meters. The width scales automatically based on the text's aspect ratio (longer text produces wider quads). Adjust this to control how large labels appear in the scene.

---

### cell_padding

```
int cell_padding = 8
```

- *Setter:* `set_cell_padding(value: int)`
- *Getter:* `get_cell_padding() -> int`

Padding in pixels around each text cell in the atlas. Prevents texture bleeding between adjacent cells when using mipmaps or linear filtering. Increase if you see artifacts at the edges of labels.

---

### render_scale

```
float render_scale = 2.0
```

- *Setter:* `set_render_scale(value: float)`
- *Getter:* `get_render_scale() -> float`

Multiplier applied to `font_size` when rendering text to the atlas. A value of `2.0` renders at double resolution, producing sharper text when viewed up close. Higher values increase atlas texture memory. Reduce to `1.0` for distant labels or large instance counts to save memory.

---

### max_label_distance

```
float max_label_distance = 100.0
```

- *Setter:* `set_max_label_distance(value: float)`
- *Getter:* `get_max_label_distance() -> float`

The maximum distance from the camera at which labels are visible. Labels beyond this distance are hidden by setting their transform to zero scale. This avoids rendering tiny, illegible text and reduces GPU fill rate.

## Methods

| Return | Name |
|--------|------|
| void | [set_instance_text](#set_instance_text) |
| String | [get_instance_text](#get_instance_text) |
| void | [set_all_texts](#set_all_texts) |
| void | [set_texts](#set_texts) |
| void | [begin_text_batch](#begin_text_batch) |
| void | [end_text_batch](#end_text_batch) |

## Method Descriptions

### set_instance_text

```
void set_instance_text(index: int, text: String)
```

Sets the text label for a single instance. The atlas is rebuilt to include the new text (unless inside a `begin_text_batch()` / `end_text_batch()` block). Setting an empty string `""` hides the label for that instance.

Duplicate strings across instances share the same atlas cell, so assigning the same text to many instances is efficient.

---

### get_instance_text

```
String get_instance_text(index: int)
```

Returns the current text label for the given instance. Returns an empty string if no text is set or the index is out of range.

---

### set_all_texts

```
void set_all_texts(text: String)
```

Sets every instance's label to the same text string. Efficient for uniform labels (e.g., all instances showing "Enemy" or a shared status). Triggers a single atlas rebuild.

---

### set_texts

```
void set_texts(texts: PackedStringArray)
```

Sets text labels for all instances from a PackedStringArray. The array length should match the instance count. If shorter, remaining instances keep their existing text. If longer, extra entries are ignored. Triggers a single atlas rebuild.

---

### begin_text_batch

```
void begin_text_batch()
```

Begins a text batch operation. While a batch is active, calls to `set_instance_text()` do not trigger atlas rebuilds. This is critical for performance when updating many instance texts at once -- without batching, each `set_instance_text()` call would rebuild the atlas individually.

Must be paired with `end_text_batch()`.

---

### end_text_batch

```
void end_text_batch()
```

Ends a text batch operation and triggers a single atlas rebuild incorporating all text changes made since `begin_text_batch()` was called.
