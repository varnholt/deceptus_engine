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

   /// \brief detects ring-specific uniforms in addition to the base set.
   /// \param shader_path file path to the fragment shader source.
   void checkUniforms(const std::string& shader_path) override;

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
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   HighResTimePoint _disable_time{};

   // ring-specific uniforms
   bool _has_u_ring_scale     = false;
   bool _has_u_pixel_size     = false;
   bool _has_u_flash_color    = false;
   bool _has_u_flash_intensity = false;

   float          _ring_scale    = 1.0f / 3.0f; //!< ring size relative to the quad; TMX property "ring_scale"
   float          _pixel_size    = 1.0f;         //!< pixel block size in screen pixels; TMX property "pixel_size"
   sf::Glsl::Vec3 _flash_color   {0.0f, 0.0f, 0.0f}; //!< flash tint color, set programmatically
   float          _flash_intensity = 0.0f;       //!< flash blend factor 0-1, animated over time

   float _flash_duration = 0.0f; //!< total fade-out duration in seconds; 0 means no active flash
   float _flash_elapsed  = 0.0f; //!< time elapsed since flash was triggered
};
