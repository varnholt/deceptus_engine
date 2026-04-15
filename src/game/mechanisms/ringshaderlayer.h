#ifndef RINGSHADERLAYER_H
#define RINGSHADERLAYER_H

#include "game/mechanisms/shaderlayer.h"

/// \brief specializes shaderlayer with ring-specific enable and timing behavior.
class RingShaderLayer : public ShaderLayer
{
public:
   /// \brief creates a ring shader layer instance.
   /// \param parent owning game node in the scene graph.
   RingShaderLayer(GameNode* parent = nullptr);

   /// \brief advances base shader timing state.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override final;

   /// \brief enables or disables the layer and records disable time.
   /// \param enabled true to render the effect, false to stop it.
   void setEnabled(bool enabled) override final;

   /// \brief triggers a colour flash that fades out over the given duration.
   /// \param red red component 0-1.
   /// \param green green component 0-1.
   /// \param blue blue component 0-1.
   /// \param duration_s fade-out duration in seconds.
   void flash(float red, float green, float blue, float duration_s);

private:
   using HighResDuration = std::chrono::high_resolution_clock::duration;
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

   HighResTimePoint _disable_time{};

   float _flash_duration = 0.0f; //!< total fade-out duration in seconds; 0 means no active flash
   float _flash_elapsed  = 0.0f; //!< time elapsed since flash was triggered
};

#endif  // RINGSHADERLAYER_H
