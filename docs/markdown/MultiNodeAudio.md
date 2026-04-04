# MultiNodeAudio

**Inherits:** [MultiNode3D](MultiNode3D.md)

Software-mixed spatial audio for all instances through a single audio channel.

## Description

Decodes an AudioStreamWAV resource to raw PCM samples, then software-mixes all playing instances into a single AudioStreamGenerator output. Each playing instance contributes to the mix with per-instance distance attenuation, stereo panning, pitch scaling, and optional distance-based low-pass filtering. All instances share one audio channel -- there is no voice limit.

The source audio must be an **AudioStreamWAV** in 16-bit or 8-bit format. When importing WAV files, set **Compress Mode** to **Disabled** in the Godot import dock. Compressed formats (IMA ADPCM, QOA) are not supported. The stream is decoded once at setup time and stored as raw float samples for efficient mixing.

Spatial audio is computed relative to the current camera position. Distance attenuation follows an inverse-distance model: full volume at `reference_distance`, falling off to silence at `max_distance`. Stereo panning is based on the horizontal angle between the camera and each instance. When `distance_filter_enabled` is true, a single-pole low-pass filter attenuates high frequencies at greater distances, simulating natural sound propagation.

Per-instance playback is fully independent. Each instance can be playing, paused, or stopped with its own playback position, volume, and pitch. The `autoplay` property starts all active instances when the node enters the tree.

When `auto_pause` is enabled (default), the audio output pauses automatically if no instances are currently playing, avoiding unnecessary mix processing.

## Properties

| Name | Type | Default |
|------|------|---------|
| [stream](#stream) | AudioStream | `null` |
| [volume_db](#volume_db) | float | `0.0` |
| [amplify_db](#amplify_db) | float | `0.0` |
| [bus](#bus) | StringName | `"Master"` |
| [loop](#loop) | bool | `false` |
| [autoplay](#autoplay) | bool | `false` |
| [max_distance](#max_distance) | float | `50.0` |
| [reference_distance](#reference_distance) | float | `1.0` |
| [auto_pause](#auto_pause) | bool | `true` |
| [pitch_randomization](#pitch_randomization) | float | `0.0` |
| [volume_randomization](#volume_randomization) | float | `0.0` |
| [distance_filter_enabled](#distance_filter_enabled) | bool | `true` |
| [distance_filter_strength](#distance_filter_strength) | float | `0.5` |

## Property Descriptions

### stream

```
AudioStream stream = null
```

- *Setter:* `set_stream(value: AudioStream)`
- *Getter:* `get_stream() -> AudioStream`

The audio stream to play. Must be an AudioStreamWAV resource with 16-bit or 8-bit sample format. The stream is decoded to raw PCM on assignment. Changing the stream re-decodes and stops all playback.

Import settings: set **Compress Mode** to **Disabled** in the Godot import dock. Compressed or Vorbis streams are not supported.

---

### volume_db

```
float volume_db = 0.0
```

- *Setter:* `set_volume_db(value: float)`
- *Getter:* `get_volume_db() -> float`

Base volume in decibels applied to all instances. `0.0` is full volume, `-80.0` is effectively silent. This is a global volume control -- per-instance volume is applied on top of this.

Range: -80.0 to 24.0 dB.

---

### amplify_db

```
float amplify_db = 0.0
```

- *Setter:* `set_amplify_db(value: float)`
- *Getter:* `get_amplify_db() -> float`

Additional gain in decibels applied to the final mix output. Use this to boost quiet source audio without modifying the original file. Applied after all per-instance mixing.

Range: -20.0 to 40.0 dB.

---

### bus

```
StringName bus = "Master"
```

- *Setter:* `set_bus(value: StringName)`
- *Getter:* `get_bus() -> StringName`

The audio bus to route the mixed output to. Must match a bus name defined in your project's Audio Bus Layout. Defaults to `"Master"`.

---

### loop

```
bool loop = false
```

- *Setter:* `set_loop(value: bool)`
- *Getter:* `get_loop() -> bool`

If `true`, playback loops from the beginning when it reaches the end of the stream. Applies to all instances -- each instance loops independently based on its own playback position.

---

### autoplay

```
bool autoplay = false
```

- *Setter:* `set_autoplay(value: bool)`
- *Getter:* `get_autoplay() -> bool`

If `true`, all active instances begin playing automatically when the node enters the scene tree. Equivalent to calling `play_all()` during `_ready()`.

---

### max_distance

```
float max_distance = 50.0
```

- *Setter:* `set_max_distance(value: float)`
- *Getter:* `get_max_distance() -> float`

The maximum distance from the camera at which instances are audible. Beyond this distance, an instance's contribution to the mix is zero. Measured in world units.

Range: 1.0 to 1000.0 meters.

---

### reference_distance

```
float reference_distance = 1.0
```

- *Setter:* `set_reference_distance(value: float)`
- *Getter:* `get_reference_distance() -> float`

The distance at which the sound is at full volume (no attenuation). Between `reference_distance` and `max_distance`, volume falls off using an inverse-distance model.

Range: 0.1 to 100.0 meters.

---

### auto_pause

```
bool auto_pause = true
```

- *Setter:* `set_auto_pause(value: bool)`
- *Getter:* `get_auto_pause() -> bool`

If `true`, the internal AudioStreamPlayer pauses when no instances are currently playing. This avoids filling the audio buffer with silence and saves processing time. The player resumes automatically when any instance starts playing.

---

### pitch_randomization

```
float pitch_randomization = 0.0
```

- *Setter:* `set_pitch_randomization(value: float)`
- *Getter:* `get_pitch_randomization() -> float`

Random pitch variation applied when an instance starts playing. A value of `0.1` means the pitch can vary by up to 10% above or below the base pitch. Applied per-instance at play time -- the randomized pitch persists for the duration of playback.

Range: 0.0 to 1.0.

---

### volume_randomization

```
float volume_randomization = 0.0
```

- *Setter:* `set_volume_randomization(value: float)`
- *Getter:* `get_volume_randomization() -> float`

Random volume variation in decibels applied when an instance starts playing. A value of `3.0` means volume can vary by up to 3 dB above or below the base volume. Applied per-instance at play time.

Range: 0.0 to 20.0 dB.

---

### distance_filter_enabled

```
bool distance_filter_enabled = true
```

- *Setter:* `set_distance_filter_enabled(value: bool)`
- *Getter:* `get_distance_filter_enabled() -> bool`

If `true`, a single-pole low-pass filter is applied per instance based on distance from the camera. Distant sounds lose high-frequency content, simulating natural sound propagation through air.

---

### distance_filter_strength

```
float distance_filter_strength = 0.5
```

- *Setter:* `set_distance_filter_strength(value: float)`
- *Getter:* `get_distance_filter_strength() -> float`

Controls the intensity of the distance-based low-pass filter. Higher values produce a stronger muffling effect at distance. `0.0` applies no filtering, `1.0` applies maximum filtering.

Range: 0.0 to 1.0.

## Methods

| Return | Name |
|--------|------|
| void | [play_instance](#play_instance) |
| void | [stop_instance](#stop_instance) |
| void | [play_all](#play_all) |
| void | [stop_all](#stop_all) |
| void | [set_instance_playing](#set_instance_playing) |
| bool | [get_instance_playing](#get_instance_playing) |
| void | [set_instance_volume](#set_instance_volume) |
| float | [get_instance_volume](#get_instance_volume) |
| void | [set_instance_pitch](#set_instance_pitch) |
| float | [get_instance_pitch](#get_instance_pitch) |
| float | [get_instance_playback_position](#get_instance_playback_position) |

## Method Descriptions

### play_instance

```
void play_instance(index: int, from_seconds: float = 0.0)
```

Starts playback for a single instance at the given time offset in seconds. If the instance is already playing, playback restarts from `from_seconds`. Pitch and volume randomization (if configured) are re-applied each time this is called.

---

### stop_instance

```
void stop_instance(index: int)
```

Stops playback for a single instance. The playback position is reset. The instance will not contribute to the audio mix until `play_instance()` or `set_instance_playing(index, true)` is called again.

---

### play_all

```
void play_all()
```

Starts playback for all active instances simultaneously. Each instance begins from the start of the stream. Pitch and volume randomization are applied independently to each instance.

---

### stop_all

```
void stop_all()
```

Stops playback for all instances. All playback positions are reset.

---

### set_instance_playing

```
void set_instance_playing(index: int, playing: bool)
```

Sets the playing state of a single instance. When set to `true`, the instance resumes from its current playback position. When set to `false`, the instance is paused (position is preserved, unlike `stop_instance()` which resets it).

---

### get_instance_playing

```
bool get_instance_playing(index: int)
```

Returns `true` if the given instance is currently playing audio. Returns `false` if stopped, paused, or the index is out of range.

---

### set_instance_volume

```
void set_instance_volume(index: int, db: float)
```

Sets the volume offset in decibels for a single instance. This is added to the global `volume_db`. Use to make specific instances louder or quieter than the base level.

---

### get_instance_volume

```
float get_instance_volume(index: int)
```

Returns the per-instance volume offset in decibels. Default is `0.0` (no offset from the global volume).

---

### set_instance_pitch

```
void set_instance_pitch(index: int, scale: float)
```

Sets the pitch scale for a single instance. `1.0` is normal pitch, `2.0` is one octave up, `0.5` is one octave down. Affects playback speed -- higher pitch means faster playback.

---

### get_instance_pitch

```
float get_instance_pitch(index: int)
```

Returns the pitch scale for the given instance. Default is `1.0`.

---

### get_instance_playback_position

```
float get_instance_playback_position(index: int)
```

Returns the current playback position in seconds for the given instance. Returns `0.0` if the instance is not playing or the index is out of range.
