#include "physicsconfigurationui.h"
#include "game/debug/settingsui.h"
#include "physicsconfiguration.h"

#pragma warning(push, 0)
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"
#pragma warning(pop)

#include <iostream>
#include <sstream>

PhysicsConfigurationUi::PhysicsConfigurationUi()
#ifdef DECEPTUS_VRSFML
    : _render_window(std::make_unique<sf::RenderWindow>(
         sf::RenderWindow::create(sf::WindowSettings{.size = {800u, 800u}, .title = "deceptus physics configuration"}).value()
      ))
#else
    : _render_window(std::make_unique<sf::RenderWindow>(sf::VideoMode({800, 800}), "deceptus physics configuration"))
#endif
{
   if (!ImGui::SFML::Init(*_render_window.get()))
   {
      std::cout << "could not create render window" << std::endl;
   }
}

void PhysicsConfigurationUi::processEvents()
{
   while (const auto event = _render_window->pollEvent())
   {
      ImGui::SFML::ProcessEvent(*_render_window.get(), event.value());

      if (event->is<sf::Event::Closed>())
      {
#ifndef DECEPTUS_VRSFML
         _render_window->close();
#endif
      }
   }
}

void PhysicsConfigurationUi::draw()
{
   auto& config = PhysicsConfiguration::getInstance();

   ImGui::SFML::Update(*_render_window.get(), _clock.restart());

   if (ImGui::BeginMainMenuBar())
   {
      if (ImGui::BeginMenu("File", true))
      {
         if (ImGui::MenuItem("Save Physics"))
         {
            std::cout << "save physics" << std::endl;
            config.serializeToFile();
         }

         if (ImGui::MenuItem("Reload Physics"))
         {
            std::cout << "reload physics" << std::endl;
            config.deserializeFromFile();
         }

         ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
   }

   const auto* viewport = ImGui::GetMainViewport();
   ImGui::SetNextWindowPos(viewport->WorkPos);
   ImGui::SetNextWindowSize(viewport->WorkSize);
   ImGui::Begin(
      "physics", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar
   );
   ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);
   ImGuiTreeNodeFlags header_flags = ImGuiTreeNodeFlags_DefaultOpen;

   SettingsUi::beginSettings();

   if (ImGui::CollapsingHeader("gravity", header_flags))
   {
      SettingsUi::drawFloat(
         "global gravity (requires level reload)",
         &config._gravity,
         -50.0f,
         50.0f,
         "Downward acceleration for the whole box2d world, in box2d units.",
         "The world is built with it, so nothing changes until the level is reloaded. Everything else "
         "under gravity is a per body scale on top of this one number."
      );
      SettingsUi::drawFloat(
         "gravity scale default",
         &config._gravity_scale_default,
         0.1f,
         3.0f,
         "How much of the world's gravity the player feels normally.",
         "NOT SAVED - it is missing from the json, so it returns to 1 on the next reload. This is also "
         "what the player is put back to on leaving water and on landing, so the other two scales are "
         "departures from it."
      );
      SettingsUi::drawFloat(
         "gravity scale water",
         &config._gravity_scale_water,
         0.1f,
         1.0f,
         "How much gravity the player feels while in water, so how quickly they sink.",
         "NOT SAVED, like the other two scales. Works against the buoyancy force under swimming, which "
         "pushes the other way: lowering this and raising that both make the player float, but only "
         "buoyancy also carries them upward."
      );
      SettingsUi::drawFloat(
         "gravity scale jump downward",
         &config._gravity_scale_jump_downward,
         0.5f,
         3.0f,
         "How much gravity the player feels once a jump has used up its upward frames, so how sharply "
         "the arc turns over into the fall.",
         "NOT SAVED, like the other two scales. Above 1 is the point - the fall is faster than the "
         "rise, which is what stops a jump feeling floaty at the top. Reset to the default scale on "
         "landing."
      );
   }

   if (ImGui::CollapsingHeader("player velocity", header_flags))
   {
      SettingsUi::drawFloat(
         "speed max walk",
         &config._player_speed_max_walk,
         0.1f,
         20.0f,
         "Top horizontal speed on the ground, in box2d units.",
         "Acceleration below only decides how quickly this is reached, never how fast the player ends "
         "up going. Jump height also reads this as its reference speed."
      );
      SettingsUi::drawFloat(
         "speed max run",
         &config._player_speed_max_run,
         0.1f,
         20.0f,
         "Top horizontal speed while the run key is held.",
         "Running was never wired up: Player::getMaxVelocity has that branch commented out. The value "
         "survives only as the reference the jump speed bonus is scaled against, and even that path is "
         "marked as probably dead."
      );
      SettingsUi::drawFloat(
         "speed max water",
         &config._player_speed_max_water,
         0.1f,
         20.0f,
         "Top horizontal speed while in water.",
         "Replaces the walk and air caps entirely as soon as the player is in water."
      );
      SettingsUi::drawFloat(
         "speed max air",
         &config._player_speed_max_air,
         0.1f,
         20.0f,
         "Top horizontal speed while airborne.",
         "Above the walk speed it would let the player gain ground by jumping. It currently ships just "
         "below it, 2.4 against 2.5, so a jump costs a sliver of speed rather than buying any."
      );
      SettingsUi::drawFloat(
         "friction",
         &config._player_friction,
         0.0f,
         1.0f,
         "Surface friction on the player's own collision shapes, handed straight to box2d.",
         "0 by default, because horizontal movement is driven by setting velocity rather than by "
         "friction. Raising it fights that and mostly makes the player catch on geometry."
      );
      SettingsUi::drawFloat(
         "jump strength",
         &config._player_jump_strength,
         0.1f,
         20.0f,
         "How hard the jump pushes upward. The force is this times the player's mass, spread over the "
         "jump's frames.",
         "Height comes out of this together with jump frame count and jump falloff, so all three move it."
      );
      SettingsUi::drawFloat(
         "acceleration ground",
         &config._player_acceleration_ground,
         0.01f,
         1.0f,
         "How much speed a held direction key adds each simulation step on the ground.",
         "Per step, and the simulation runs 60 of them a second, so the shipped 0.5 reaches the walk "
         "speed of 2.5 in five steps, under a tenth of a second. Capped by speed max walk."
      );
      SettingsUi::drawFloat(
         "deceleration ground",
         &config._player_deceleration_ground,
         0.01f,
         1.0f,
         "How much of the current speed is kept each step once no direction is held, on the ground.",
         "A survival fraction, not a subtraction. The shipped 0.1 keeps a tenth of the speed per step, "
         "so the player stops within a few frames; 0.95 would slide. Also applied when they turn "
         "against their own momentum."
      );
      SettingsUi::drawFloat(
         "acceleration air",
         &config._player_acceleration_air,
         0.01f,
         1.0f,
         "How much speed a held direction key adds each step while airborne.",
         "Currently the same as the ground value, so the arc can be steered as freely as a walk. "
         "Lowering it below the ground value is what would give a jump commitment."
      );
      SettingsUi::drawFloat(
         "deceleration air",
         &config._player_deceleration_air,
         0.01f,
         1.0f,
         "How much of the current speed is kept each step with no direction held, while airborne.",
         "Same survival fraction as on the ground, and shipped lower - 0.05 against 0.1 - so letting "
         "go mid-air kills horizontal speed faster than it does on the ground. Raise it to keep the "
         "jump's momentum."
      );
      SettingsUi::drawFloat(
         "deceleration sword attack",
         &config._player_deceleration_sword_attack,
         0.5f,
         1.0f,
         "How much of the current horizontal speed is kept each step while a sword attack is running.",
         "Below 1 roots the player into the swing. It is applied on top of the ordinary deceleration "
         "for wherever they happen to be standing."
      );
      SettingsUi::drawFloat(
         "acceleration water",
         &config._player_acceleration_water,
         0.01f,
         1.0f,
         "How much speed a held direction key adds each step in water.",
         "Together with the water deceleration below this is what makes swimming feel heavy rather "
         "than merely slow."
      );
      SettingsUi::drawFloat(
         "deceleration water",
         &config._player_deceleration_water,
         0.01f,
         1.0f,
         "How much of the current speed is kept each step with no direction held, in water.",
         "High values glide. Note this is a survival fraction like the others, so higher means less "
         "damping, which is the opposite of what the name suggests."
      );
      SettingsUi::drawFloat(
         "cap velocity horizontal",
         &config._player_max_velocity_horizontal,
         0.1f,
         30.0f,
         "Hard ceiling on horizontal speed, applied after everything else has had its say.",
         "A backstop, not a tuning value: it is what stops a dash or a conveyor from launching the "
         "player. Well above the ordinary speeds by design."
      );
      SettingsUi::drawFloat(
         "cap velocity up",
         &config._player_max_velocity_up,
         0.1f,
         30.0f,
         "Hard ceiling on upward speed.",
         "Set below the speed a jump or wall jump can reach and it clips the top off them, which reads "
         "as the jump losing height for no visible reason."
      );
      SettingsUi::drawFloat(
         "cap velocity down",
         &config._player_max_velocity_down,
         0.1f,
         30.0f,
         "Hard ceiling on falling speed, i.e. terminal velocity.",
         "Also decides how hard the worst possible landing is, so it interacts with the hard landing "
         "damage factor further down."
      );
   }

   if (ImGui::CollapsingHeader("player jump", header_flags))
   {
      const auto jump_frame_count_min = 1;
      const auto jump_frame_count_max = 50;
      const auto jump_frame_count_min_min = 0;
      auto jump_frame_count_min_max = config._player_jump_frame_count - 1;

      if (jump_frame_count_min_max < jump_frame_count_min_min)
      {
         jump_frame_count_min_max = jump_frame_count_min_min;
      }

      SettingsUi::drawInt(
         "jump frame count",
         &config._player_jump_frame_count,
         jump_frame_count_min,
         jump_frame_count_max,
         "For how many simulation steps the jump keeps pushing upward while the button is held.",
         "This is what makes the jump variable: releasing early ends the push early. Steps, not "
         "milliseconds, so at 60 steps a second 9 frames is about 150 ms."
      );
      SettingsUi::drawInt(
         "jump frame count minimum",
         &config._player_jump_frame_count_minimum,
         jump_frame_count_min_min,
         jump_frame_count_min_max,
         "How many of those steps run even if the button is released immediately, so a tap still "
         "produces a real hop.",
         "Capped at one below the jump frame count, since it cannot outlast the jump it belongs to."
      );
      SettingsUi::drawInt(
         "jump after contact lost [ms]",
         &config._player_jump_after_contact_lost_ms,
         jump_frame_count_min_min,
         200,
         "How long after walking off an edge a jump is still accepted. Coyote time.",
         "Forgives being a frame or two late off a ledge. Too generous and the player can jump out of "
         "thin air well after leaving the ground."
      );
      SettingsUi::drawInt(
         "jump buffer [ms]",
         &config._player_jump_buffer_ms,
         10,
         200,
         "How long before landing a jump press is remembered, so it fires on touchdown instead of "
         "being dropped.",
         "The mirror of coyote time: that one forgives pressing late, this one forgives pressing early."
      );
      SettingsUi::drawInt(
         "jump minimal duration [ms]",
         &config._player_jump_minimal_duration_ms,
         20,
         200,
         "The jump keeps pushing for at least this long no matter what the button does.",
         "Overlaps with jump frame count minimum, which does the same job counted in steps. Whichever "
         "is the more generous of the two is the one that decides a tap."
      );
      SettingsUi::drawFloat(
         "jump falloff",
         &config._player_jump_falloff,
         1.0f,
         20.0f,
         "Divides the jump force, so it trades directly against jump strength.",
         "A divisor: higher means a weaker jump. Strength and falloff together set the height, which "
         "is why neither on its own reads as the obvious height knob."
      );
      SettingsUi::drawFloat(
         "jump speed factor",
         &config._player_jump_speed_factor,
         0.0f,
         0.5f,
         "How much extra height a jump gains for already moving faster than walking speed.",
         "Only bites while the player is above walk speed, which on the ground they cannot be, so this "
         "mostly does nothing today. Scaled against speed max run."
      );
      SettingsUi::drawFloat(
         "jump impulse factor",
         &config._player_jump_impulse_factor,
         1.0f,
         20.0f,
         "The one-off upward kick used where a jump has to start instantly rather than build up: this "
         "times the player's mass.",
         "NOT SAVED. It is missing from the json, so anything set here is gone on the next reload or "
         "restart."
      );
      SettingsUi::drawFloat(
         "minimum jump interval [ms]",
         &config._player_minimum_jump_interval_ms,
         100.0f,
         200.0f,
         "The shortest time allowed between two jumps, which is what stops jump spam.",
         "NOT SAVED. Missing from the json as well, so it goes back to its built-in value on reload."
      );
   }

   if (ImGui::CollapsingHeader("player attack dash", header_flags))
   {
      SettingsUi::drawInt(
         "attack dash frame count",
         &config._player_attack_dash_frame_count,
         1,
         100,
         "For how many simulation steps the lunge that comes with an attack pushes the player forward.",
         "Its force is built from the dash vector down under player dash, so that value moves both."
      );
      SettingsUi::drawFloat(
         "attack dash multiplier",
         &config._player_attack_dash_multiplier,
         1.0f,
         100.0f,
         "How hard the attack lunge starts out.",
         "It only ever falls from here, by the decrement below, so this is the peak of the lunge."
      );
      SettingsUi::drawFloat(
         "attack dash multiplier dec. per frame",
         &config._player_attack_dash_multiplier_decrement_per_frame,
         0.1f,
         10.0f,
         "How much of the lunge's strength is taken away each step.",
         "Subtracted, so larger values make a shorter, sharper lunge. Set it high enough to reach zero "
         "before the frame count runs out and the lunge simply stops early."
      );
      SettingsUi::drawFloat(
         "attack dash multiplier scale per frame",
         &config._player_attack_dash_multiplier_scale_per_frame,
         0.1f,
         5.0f,
         "A plain multiplier on the lunge force.",
         "Despite the name it is applied to the force rather than accumulated into the strength, so it "
         "scales the whole lunge evenly instead of compounding over its frames."
      );
   }

   if (ImGui::CollapsingHeader("player dash", header_flags))
   {
      SettingsUi::drawInt(
         "dash frame count",
         &config._player_dash_frame_count,
         1,
         100,
         "For how many simulation steps a dash pushes the player sideways.",
         "Gravity is switched off for the whole dash, so this is also how long the player hangs in the "
         "air. A dash needs the Dash skill, costs stamina, and cannot restart within a second."
      );
      SettingsUi::drawFloat(
         "dash multiplier",
         &config._player_dash_multiplier,
         1.0f,
         100.0f,
         "How hard the dash starts out.",
         "The two values below change it as the dash runs, so this is only its opening strength."
      );
      SettingsUi::drawFloat(
         "dash multiplier inc. per frame",
         &config._player_dash_multiplier_increment_per_frame,
         -10.0f,
         -0.1f,
         "How much the dash's strength changes each step. Negative, so it fades.",
         "If it reaches zero before the frame count is up, the rest of the dash pushes backwards. The "
         "shipped values are tuned to land on zero exactly as the dash ends: 40 strength losing 1 over "
         "40 frames."
      );
      SettingsUi::drawFloat(
         "dash multiplier scale per frame",
         &config._player_dash_multiplier_scale_per_frame,
         0.1f,
         5.0f,
         "A multiplier applied to the dash's strength every step.",
         "This one really does compound, because it is applied to the stored strength: anything above "
         "1 grows the dash exponentially over its frames. 1 leaves it alone."
      );
      SettingsUi::drawFloat(
         "dash vector",
         &config._player_dash_vector,
         0.1f,
         50.0f,
         "The force one unit of dash strength is worth, times the player's mass.",
         "Shared with the attack lunge above, so changing it retunes both."
      );
   }

   if (ImGui::CollapsingHeader("wall slide", header_flags))
   {
      SettingsUi::drawFloat(
         "wall slide friction",
         &config._player_wall_slide_friction,
         0.0f,
         1.0f,
         "How strongly sliding down a wall is braked. The force opposes the player's own velocity, "
         "scaled by this.",
         "0 is a free fall down the wall, 1 nearly pins them to it. Wall sliding is also the state a "
         "wall jump has to be launched from."
      );
   }

   if (ImGui::CollapsingHeader("wall jump", header_flags))
   {
      SettingsUi::drawInt(
         "wall jump frame count",
         &config._player_wall_jump_frame_count,
         1,
         50,
         "For how many simulation steps the wall jump keeps pushing away from the wall.",
         "Runs alongside the key lock further down, which is what stops the player steering straight "
         "back into the wall they just left."
      );
      SettingsUi::drawFloat(
         "wall jump vector x",
         &config._player_wall_jump_vector_x,
         0.0f,
         20.0f,
         "How hard the wall jump throws the player away from the wall, times their mass.",
         "This against the y value below is the angle of the launch. The player's velocity is zeroed "
         "first, so the wall jump does not inherit the fall."
      );
      SettingsUi::drawFloat(
         "wall jump vector y",
         &config._player_wall_jump_vector_y,
         0.0f,
         20.0f,
         "How hard the wall jump throws the player upward, times their mass.",
         "Small against the x value gives a long flat leap, large gives a climb up the wall."
      );
      SettingsUi::drawFloat(
         "wall jump multiplier",
         &config._player_wall_jump_multiplier,
         1.0f,
         50.0f,
         "How hard the wall jump's push starts out, before the two values below wear it down.",
         "Mirrors the dash: a starting strength that is changed every step while the jump runs."
      );
      SettingsUi::drawFloat(
         "wall jump multiplier inc. per frame",
         &config._player_wall_jump_multiplier_increment_per_frame,
         -10.0f,
         -0.1f,
         "How much the wall jump's strength changes each step. Negative, so it fades.",
         "As with the dash, letting it cross zero inside the frame count turns the push around, and the "
         "shipped values land on zero exactly as the jump ends: 20 losing 1 over 20 frames."
      );
      SettingsUi::drawFloat(
         "wall jump multiplier scale per frame",
         &config._player_wall_jump_multiplier_scale_per_frame,
         0.1f,
         5.0f,
         "A multiplier applied to the wall jump's strength every step.",
         "1 leaves it alone. Above 1 compounds over the jump's frames."
      );
      SettingsUi::drawFloat(
         "wall jump extra force",
         &config._player_wall_jump_extra_force,
         0.0f,
         5.0f,
         "Extra push given to a jump that has to overcome an existing fall, scaled by how fast the "
         "player is already falling.",
         "Used by the wall jump and the double jump, not by an ordinary jump, so that neither is eaten "
         "by the fall it starts from."
      );
      SettingsUi::drawInt(
         "wall jump lock key duration [ms]",
         &config._player_wall_jump_lock_key_duration_ms,
         0,
         2000,
         "How long the direction keys are forced after a wall jump: away from the wall held, back "
         "towards it ignored.",
         "Too short and holding towards the wall cancels the jump on the spot; too long and the player "
         "cannot steer at all after one."
      );
   }

   if (ImGui::CollapsingHeader("double jump", header_flags))
   {
      SettingsUi::drawFloat(
         "double jump factor",
         &config._player_double_jump_factor,
         1.0f,
         20.0f,
         "The upward kick of the second jump: this times the player's mass, applied in one go.",
         "A single impulse rather than a push over several frames, so unlike the first jump its height "
         "does not depend on how long the button is held."
      );
   }

   if (ImGui::CollapsingHeader("hard landing", header_flags))
   {
      SettingsUi::drawBool(
         "hard landing damage enabled",
         &config._player_hard_landing_damage_enabled,
         "Whether landing hard enough costs health.",
         "Off still leaves the landing itself: the player is stunned for the delay below either way."
      );
      SettingsUi::drawFloat(
         "hard landing damage factor",
         &config._player_hard_landing_damage_factor,
         0.0f,
         50.0f,
         "How much health a hard landing costs, per unit of landing impulse above the threshold.",
         "Measured against the impulse, so the ceiling on falling speed up under player velocity sets "
         "the worst case this can produce."
      );
      SettingsUi::drawFloat(
         "hard landing delay [s]",
         &config._player_hard_landing_delay_s,
         0.0f,
         5.0f,
         "How long the player is stuck in the landing before they can move again.",
         "Applies whether or not the damage above is enabled, so this is the knob for how punishing a "
         "long fall feels."
      );
   }

   if (ImGui::CollapsingHeader("swimming", header_flags))
   {
      SettingsUi::drawFloat(
         "in water force jump button",
         &config._player_in_water_force_jump_button,
         -10.0f,
         0.0f,
         "The upward force holding the jump button gives while in water, i.e. how the player swims up.",
         "Negative is upward here, because box2d's y axis points down. Only applied after the delay "
         "below has passed."
      );
      SettingsUi::drawInt(
         "in water time to allow jump button [ms]",
         &config._player_in_water_time_to_allow_jump_button_ms,
         0,
         2000,
         "How long after entering water the jump button starts pushing upward instead of jumping.",
         "It exists so that diving in does not immediately bounce the player back out on a button they "
         "were already holding."
      );
      SettingsUi::drawFloat(
         "in water linear velocity y clamp min",
         &config._player_in_water_linear_velocity_y_clamp_min,
         -5.0f,
         0.0f,
         "Fastest the player may rise in water. Negative is upward.",
         "The pair of clamps is what keeps swimming feeling like water rather than like flight, "
         "whatever buoyancy and the jump force add up to."
      );
      SettingsUi::drawFloat(
         "in water linear velocity y clamp max",
         &config._player_in_water_linear_velocity_y_clamp_max,
         0.0f,
         5.0f,
         "Fastest the player may sink in water.",
         "Independent of the ordinary falling ceiling, so entering water always slows the fall."
      );
      SettingsUi::drawFloat(
         "in water buoyancy force",
         &config._in_water_buoyancy_force,
         0.0f,
         0.2f,
         "Constant upward force on a submerged player, as a fraction of the world's gravity.",
         "Above the water gravity scale the player floats up on their own. Together with that scale it "
         "decides whether water is something to sink in or something to bob in."
      );
   }

   SettingsUi::endSettings();

   ImGui::End();

   _render_window->clear();
   ImGui::SFML::Render(*_render_window.get());
   _render_window->display();
}

void PhysicsConfigurationUi::close()
{
   ImGui::SFML::Shutdown(*_render_window.get());
}
