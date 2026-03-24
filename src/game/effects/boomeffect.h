#pragma once

#include <SFML/Graphics.hpp>

#include <functional>
#include <memory>

#include "boomeffectenvelope.h"
#include "boomsettings.h"

/// \brief computes camera shake offsets using configurable envelope functions.
struct BoomEffect
{
   /// \brief starts a new shake if camera shaking is enabled and no shake is currently active.
   /// \param x horizontal shake factor multiplied by view width when offsets are computed.
   /// \param y vertical shake factor multiplied by view height when offsets are computed.
   /// \param boom_settings amplitude, duration, and envelope type for the shake.
   void boom(float x, float y, const BoomSettings& boom_settings = _default_boom_settings);
   /// \brief advances the current shake and writes the resulting per-axis offsets.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt);

   /// \brief retrieves remaining shake time in seconds relative to the global clock.
   /// \return seconds until shake end, which can be negative after expiry.
   float getRemainingTime() const;
   /// \brief retrieves normalized shake progress from start to end.
   /// \return normalized time where 0.0 is shake start and 1.0 is configured duration.
   float getRemainingTimeNormalized() const;

   sf::Time _boom_time_end;
   float _boom_offset_x = 0.0f;
   float _boom_offset_y = 0.0f;
   float _boom_duration_s = 1.0f;
   float _factor_x = 0.0f;
   float _factor_y = 0.0f;

   std::shared_ptr<BoomEffectEnvelope> _envelope;

   using ShakeFunction = std::function<float(float)>;
   ShakeFunction _shake_function;

   static BoomSettings _default_boom_settings;
};
