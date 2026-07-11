#pragma once

#include "game/effects/lightsystem.h"
#include "game/items/item.h"

#include <SFML/Graphics.hpp>

/// \brief lantern item that renders a warm circular player light when equipped.
class ItemLantern : public Item
{
public:
   /// \brief initializes the lantern light shape and default radius.
   ItemLantern();

   /// \brief draws the lantern light circle when the item is enabled.
   /// \param target SFML render target that receives the light sprite.
   void draw(sf::RenderTarget& target) override;

   /// \brief updates the light position to follow the current player.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt) override;

   /// \brief enables lantern rendering and updates while equipped.
   void onEquipped() override;

   /// \brief disables lantern rendering and updates while unequipped.
   void onUnequipped() override;

   /// \brief gets the inventory display name for this item.
   /// \return the string "Lantern".
   std::string getName() const override;

private:
#ifdef __EMSCRIPTEN__
   sf::CircleShape _light_circle{sf::CircleShape::Data{.radius = 50.0f}};
#else
   sf::CircleShape _light_circle;
#endif
   float _light_radius{50.0f};
   bool _enabled{false};
   sf::Time _elapsed;
   std::shared_ptr<LightSystem::LightInstance> _player_light_left;
   std::shared_ptr<LightSystem::LightInstance> _player_light_right;
   std::optional<sf::Vector2f>
      _last_valid_eye_position;  //!< set once a valid eye position is received since the last onEquipped; empty until then
   std::shared_ptr<sf::Texture> _player_texture;
   std::unique_ptr<sf::Sprite> _helmet_sprite_r;
   std::unique_ptr<sf::Sprite> _helmet_sprite_l;
   std::shared_ptr<sf::Shader> _noise_shader;                      //!< shared noise shader applied to both lantern lights
#ifdef __EMSCRIPTEN__
   sf::base::Optional<sf::Shader::UniformLocation> _ul_time;            //!< cached uniform location for u_time
   sf::base::Optional<sf::Shader::UniformLocation> _ul_intensity;       //!< cached uniform location for u_intensity
   sf::base::Optional<sf::Shader::UniformLocation> _ul_flicker_speed;   //!< cached uniform location for u_flicker_speed
   sf::base::Optional<sf::Shader::UniformLocation> _ul_flicker_amount;  //!< cached uniform location for u_flicker_amount
   sf::base::Optional<sf::Shader::UniformLocation> _ul_layer_1_size;    //!< cached uniform location for u_layer_1_size
   sf::base::Optional<sf::Shader::UniformLocation> _ul_layer_1_speed;   //!< cached uniform location for u_layer_1_speed
   sf::base::Optional<sf::Shader::UniformLocation> _ul_layer_2_size;    //!< cached uniform location for u_layer_2_size
   sf::base::Optional<sf::Shader::UniformLocation> _ul_layer_2_speed;   //!< cached uniform location for u_layer_2_speed
   sf::base::Optional<sf::Shader::UniformLocation> _ul_sprite_pos_px;   //!< cached uniform location for u_sprite_pos_px
   sf::base::Optional<sf::Shader::UniformLocation> _ul_sprite_size_px;  //!< cached uniform location for u_sprite_size_px
#endif
   float _offset_left_x_m{-3.4f};                                  //!< x body offset when facing left, in box2d meters
   float _offset_right_x_m{1.9f};                                  //!< x body offset when facing right, in box2d meters
   float _offset_left_y_m{-1.0f};                                  //!< y body offset when facing left, in box2d meters
   float _offset_right_y_m{-1.0f};                                 //!< y body offset when facing right, in box2d meters

   float _flicker_speed{3.0f};    //!< noise frequency controlling the organic light flicker
   float _flicker_amount{0.12f};  //!< fractional brightness variation from flicker (0–1)

   bool _was_hard_landing{false};                 //!< previous-frame hard-landing state for rising-edge detection
   sf::Time _jitter_elapsed;                      //!< remaining jitter animation time
   sf::Time _jitter_duration{sf::seconds(0.4f)};  //!< total duration of the hard-landing jitter animation
   float _jitter_magnitude_m{0.12f};              //!< peak jitter displacement in box2d meters

   bool _was_on_ground{false};                          //!< previous-frame ground state for rising-edge detection
   sf::Time _landing_tilt_elapsed;                      //!< remaining landing-tilt animation time
   sf::Time _landing_tilt_duration{sf::seconds(0.8f)};  //!< total duration of the tilt ease-back animation
   float _landing_tilt_max_degrees{6.0f};               //!< peak downward beam tilt applied at the moment of landing

   sf::Time _dust_burst_elapsed;                      //!< remaining dust-burst animation time
   sf::Time _dust_burst_duration{sf::seconds(0.5f)};  //!< total duration of the dust burst on hard landing
   float _dust_burst_peak_multiplier{4.0f};           //!< peak dust intensity multiplier during the burst

   bool _was_eye_position_valid{false};            //!< previous-frame eye-position validity for rising-edge detection
   sf::Time _fade_in_elapsed;                      //!< remaining fade-in time after the player spawns visible
   sf::Time _fade_in_duration{sf::seconds(0.5f)};  //!< total duration of the appear fade-in
   uint8_t _target_alpha{80};                      //!< configured full-brightness alpha restored after fade-in completes
};
