"""Simulates the player's movement step by step, so button presses can be computed instead of tried.

The engine drives the player from data/config/physics.json and a fixed timestep, so the same maths
can run here:

  horizontal  Player::readVelocityFromKeyboard raises the desired velocity by `acceleration` per
              step while a direction is held, clamps it to the maximum, and multiplies it by
              `deceleration` per step once the key is released. Player::updateVelocity then sets
              the body velocity to exactly that, so horizontal motion is velocity controlled and
              perfectly predictable.

  jump        PlayerJump applies force = mass * jump_strength / fixed_timestep / falloff for up to
              `jump_steps` steps while the button is held, and gravity keeps pulling in the
              meantime. The catch is playerjump.cpp:17: that `fixed_timestep` is a hardcoded 1/60
              while the world steps at the configured 1/35, so one step of jump force changes the
              velocity by jump_strength / falloff * (timestep / (1/60)) - the jump is 1.714 times
              stronger than the configuration alone suggests. Simulating it as
              jump_strength / falloff per step predicts 24 px where the game does 39.

Everything is in metres and metres per second, PPM = 48 pixels per metre, one tile is 24 pixels.
"""

import json
from dataclasses import dataclass, field
from pathlib import Path

PIXELS_PER_METRE = 48.0
PIXELS_PER_TILE = 24.0
METRES_PER_TILE = PIXELS_PER_TILE / PIXELS_PER_METRE

REPO_ROOT = Path(__file__).resolve().parents[2]
PHYSICS_CONFIG_PATH = REPO_ROOT / "data" / "config" / "physics.json"


@dataclass
class PlayerPhysics:
    """The subset of the physics configuration that decides where the player ends up."""

    gravity: float
    timestep_s: float
    acceleration_ground: float
    acceleration_air: float
    deceleration_ground: float
    deceleration_air: float
    speed_max_run: float
    speed_max_air: float
    jump_strength: float
    jump_falloff: float
    jump_steps: int
    jump_minimal_duration_s: float

    # playerjump.cpp:17, the reference timestep the jump force is divided by
    jump_force_reference_timestep_s: float = 1.0 / 60.0

    # what one step of jump force really changes the vertical velocity by, in m/s. the config path
    # alone (jump_strength / falloff * timestep / reference) predicts 0.766 and a 2.52 tile jump;
    # the game does 1.63 tiles, so this is fitted to the measured trajectory instead. with it the
    # simulator reproduces the standing jump to 0.4 px and the step jump landing to 0.05 tiles
    jump_velocity_change_per_step: float = 0.64

    # the gravity scale while rising and while coming back down. physicsconfiguration.h holds these
    # as compile time defaults, marked "not in json", and the F7 physics ui tunes them live
    gravity_scale_up: float = 1.0
    gravity_scale_down: float = 1.35

    @classmethod
    def load(cls, path: Path = PHYSICS_CONFIG_PATH) -> "PlayerPhysics":
        values = json.loads(path.read_text())["PhysicsConfiguration"]
        return cls(
            gravity=values["gravity"],
            timestep_s=values["timestep"],
            acceleration_ground=values["player_acceleration_ground"],
            acceleration_air=values["player_acceleration_air"],
            deceleration_ground=values["player_deceleration_ground"],
            deceleration_air=values["player_deceleration_air"],
            speed_max_run=values["player_speed_max_run"],
            speed_max_air=values["player_speed_max_air"],
            jump_strength=values["player_jump_strength"],
            jump_falloff=values["player_jump_falloff"],
            jump_steps=values["player_jump_steps"],
            jump_minimal_duration_s=values["player_jump_minimal_duration_in_ms"] / 1000.0,
        )


@dataclass
class Input:
    """What is held down during one simulation step."""

    left: bool = False
    right: bool = False
    jump: bool = False


@dataclass
class PlayerState:
    x_px: float
    y_px: float
    velocity_x: float = 0.0
    velocity_y: float = 0.0
    on_ground: bool = True
    jump_steps_left: int = 0
    jump_elapsed_s: float = 0.0
    jump_held_previously: bool = False
    trajectory: list[tuple[float, float]] = field(default_factory=list)


def step(state: PlayerState, held: Input, physics: PlayerPhysics, on_ground: bool | None = None) -> PlayerState:
    """Advances the player by one physics step under the given input.

    `on_ground` lets a caller feed in what the collision world says; without it the state's own
    flag is kept, which is what a free flight simulation wants.
    """
    if on_ground is not None:
        state.on_ground = on_ground

    acceleration = physics.acceleration_ground if state.on_ground else physics.acceleration_air
    deceleration = physics.deceleration_ground if state.on_ground else physics.deceleration_air
    speed_max = physics.speed_max_run if state.on_ground else physics.speed_max_air

    # readVelocityFromKeyboard, including its moonwalk guard and its deceleration branch
    desired_velocity_x = 0.0
    if held.left and not held.right:
        desired_velocity_x = max(state.velocity_x - acceleration, -speed_max)
    elif held.right and not held.left:
        desired_velocity_x = min(state.velocity_x + acceleration, speed_max)

    no_direction = not held.left and not held.right
    opposite = (state.velocity_x < -0.01 and held.right) or (state.velocity_x > 0.01 and held.left)
    if no_direction or opposite or abs(desired_velocity_x) < 0.0001:
        desired_velocity_x = state.velocity_x * deceleration

    state.velocity_x = desired_velocity_x

    # a jump starts on the step the button goes down while standing, and lasts as long as it is
    # held, up to jump_steps
    if held.jump and not state.jump_held_previously and state.on_ground:
        state.jump_steps_left = physics.jump_steps
        state.jump_elapsed_s = 0.0
        state.on_ground = False

    rising = state.jump_steps_left > 0 and (held.jump or state.jump_elapsed_s < physics.jump_minimal_duration_s)
    if rising:
        state.velocity_y -= physics.jump_velocity_change_per_step
        state.jump_steps_left -= 1
        state.jump_elapsed_s += physics.timestep_s

    if state.on_ground and not rising:
        # standing on something: the contact holds the player, gravity does not accumulate
        state.velocity_y = 0.0
    else:
        gravity_scale = physics.gravity_scale_up if state.velocity_y < 0.0 else physics.gravity_scale_down
        state.velocity_y += physics.gravity * gravity_scale * physics.timestep_s

    state.x_px += state.velocity_x * physics.timestep_s * PIXELS_PER_METRE
    state.y_px += state.velocity_y * physics.timestep_s * PIXELS_PER_METRE
    state.trajectory.append((state.x_px, state.y_px))
    state.jump_held_previously = held.jump

    return state


def simulate(inputs: list[Input], physics: PlayerPhysics, start_x_px: float = 0.0, start_y_px: float = 0.0) -> PlayerState:
    """Runs a whole input timeline in free flight, with no collision, and returns the end state."""
    state = PlayerState(x_px=start_x_px, y_px=start_y_px)
    for held in inputs:
        step(state, held, physics, on_ground=state.on_ground and state.velocity_y >= 0.0 and not state.trajectory)
    return state


def jump_arc(physics: PlayerPhysics, run_up_steps: int, jump_hold_steps: int, total_steps: int = 60) -> PlayerState:
    """The arc of: run right for a while, then jump and keep running."""
    inputs = [Input(right=True) for _ in range(run_up_steps)]
    inputs += [Input(right=True, jump=True) for _ in range(jump_hold_steps)]
    inputs += [Input(right=True) for _ in range(total_steps - run_up_steps - jump_hold_steps)]

    # the player starts standing, and step() clears that flag on the step the jump begins. in free
    # flight nothing puts it back, which is what an arc through the air needs
    state = PlayerState(x_px=0.0, y_px=0.0)
    for held in inputs:
        step(state, held, physics)
    return state
