# Player Jump Physics

<br>

# Regular Jump

## Initialization
```cpp
jump_frame_count = json_player_jump_steps
```

## For Each Jump Step (`player_jump_steps`)
```cpp
fixed_timestep = 1.0 / 60.0
force_applied_to_body = factor * body_mass * json_player_jump_strength / fixed_timestep
force_applied_to_body /= json_player_jump_falloff
```

<br><br>

# Wall Jump

## Initialization
```cpp
impulse_x =   body_mass * json_player_wall_jump_vector_x
impulse_y = - body_mass * json_player_wall_jump_vector_y

walljump_frame_count = json_player_wall_jump_frame_count
walljump_multiplier  = json_player_wall_jump_multiplier

walljump_direction = vec2(jump_right ? impulse_x : -impulse_x, impulse_y)
```

## For Each Wall Jump Step (`walljump_frame_count`)

```cpp
walljump_multiplier *= json_player_wall_jump_multiplier_scale_per_frame
walljump_multiplier += json_player_wall_jump_multiplier_increment_per_frame

force_applied_to_body = walljump_multiplier * walljump_direction
```

<br><br>

# Double Jump

```cpp
impulse_applied_once_to_body = vec2(0.0, body_mass * json_player_double_jump_factor)
```

<br><br>

# Dash

## Initialization
```cpp
mDashFrameCount = json_player_dash_frame_count
mDashMultiplier = json_player_dash_multiplier
```

## For Each Dash Step (`player_dash_frame_count`)
```cpp
dash_multiplier += json_player_dash_multiplier_increment_per_frame
dash_multiplier *= json_player_dash_multiplier_scale_per_frame

auto dash_vector = dash_multiplier * body_mass * json_player_dash_vector
auto impulse = (left) ? -dash_vector : dash_vector

force_applied_to_body = vec2(impulse, 0.0)
```
