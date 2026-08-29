#pragma once

#include "game/mechanisms/shaderlayer.h"

/// \brief specializes shaderlayer with ring-specific uniforms, enable/timing behavior, and flash support.
class RingShaderLayer : public ShaderLayer
{
public:
   /// \brief creates a ring shader layer instance.
   /// \param parent owning game node in the scene graph.
   RingShaderLayer(GameNode* parent = nullptr);

   /// \brief advances shader timing and animates flash fade-out.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override final;

   /// \brief enables or disables the layer and records disable time.
   /// \param enabled true to render the effect, false to stop it.
   void setEnabled(bool enabled) override final;

   /// \brief sets ring-specific uniforms then delegates quad drawing to the base.
   /// \param target render target.
   /// \param normal normal-map render target.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override final;

#ifdef DECEPTUS_VRSFML
   /// \brief sets ring-specific uniforms then delegates quad drawing to the base (states-carrying overload).
   /// \param target render target.
   /// \param normal normal-map render target.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states) override final;
#else
   /// \brief detects ring-specific uniforms in addition to the base set.
   /// \param shader_path file path to the fragment shader source.
   void checkUniforms(const std::string& shader_path) override;
#endif

   /// \brief reads ring-specific TMX properties (ring_scale, pixel_size).
   /// \param data deserialization data passed through from the factory.
   void readCustomProperties(const GameDeserializeData& data) override;

   /// \brief triggers a colour flash that fades out over the given duration.
   /// \param red red component 0-1.
   /// \param green green component 0-1.
   /// \param blue blue component 0-1.
   /// \param duration_s fade-out duration in seconds.
   void flash(float red, float green, float blue, float duration_s);

private:
   /// \brief returns the ring scale for this frame, including the beat and the power-down.
   /// \return value handed to the shader as u_ring_scale.
   float currentRingScale() const;

   /// \brief checks whether the player is pushing against the band and updates the dent.
   /// \param dt elapsed frame time.
   void updateTouch(const sf::Time& dt);

   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   HighResTimePoint _disable_time{};

#ifndef DECEPTUS_VRSFML
   // ring-specific uniforms
   bool _has_u_ring_scale = false;
   bool _has_u_pixel_size = false;
   bool _has_u_flash_color = false;
   bool _has_u_flash_intensity = false;
   bool _has_u_touch = false;
#endif

   float _ring_scale = 1.0f / 3.0f;                //!< ring size relative to the quad; TMX property "ring_scale"
   float _pixel_size = 1.0f;                       //!< pixel block size in screen pixels; TMX property "pixel_size"
   sf::Glsl::Vec3 _flash_color{0.0f, 0.0f, 0.0f};  //!< flash tint color, set programmatically
   float _flash_intensity = 0.0f;                  //!< flash blend factor 0-1, animated over time

   float _flash_duration = 0.0f;  //!< total fade-out duration in seconds; 0 means no active flash
   float _flash_elapsed = 0.0f;   //!< time elapsed since flash was triggered

   float _heartbeat_period_s = 2.0f;     //!< time from one beat to the next; TMX property "heartbeat_period_s"
   float _heartbeat_scale = 1.1f;        //!< ring size at the peak of a beat, 1.0 disables it; TMX property "heartbeat_scale"
   float _heartbeat_second_beat = 0.7f;  //!< strength of the weaker second beat; TMX property "heartbeat_second_beat"
   float _heartbeat_turbulence = 1.0f;   //!< extra churn speed at the peak of a beat; TMX property "heartbeat_turbulence"
   float _heartbeat_beat_width =
      0.035f;  //!< standard deviation of one beat as a fraction of the period; TMX property "heartbeat_beat_width"

   float _touch_depth = 0.22f;      //!< how far the band is dented where the player pushes; TMX property "touch_depth"
   float _touch_width = 0.55f;      //!< angular falloff of the dent in radians; TMX property "touch_width"
   float _touch_release_s = 0.35f;  //!< time the dent needs to smooth out again; TMX property "touch_release_s"
   float _touch_angle = 0.0f;       //!< angle the player is currently pressing against
   float _touch_intensity = 0.0f;   //!< current dent strength, 0 when nothing is touching
   bool _touched = false;           //!< whether the player was against the band last frame

   float _power_down_s = 0.35f;        //!< time the ring needs to die after being disabled; TMX property "power_down_s"
   float _power_down_progress = 0.0f;  //!< 0 while powered, 1 once the ring has gone out for good

   float _heartbeat_elapsed_s = 0.0f;  //!< time since the current beat cycle started
   float _heartbeat_pulse = 0.0f;      //!< current beat strength, 0 between beats and 1 at the peak of the first
};
