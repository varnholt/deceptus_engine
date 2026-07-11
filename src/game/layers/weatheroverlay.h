#pragma once

#include <SFML/Graphics.hpp>

/// \brief polymorphic interface for weather effects rendered on top of the level.
class WeatherOverlay
{
public:
   /// \brief destroys the weather overlay instance.
   virtual ~WeatherOverlay() = default;

   /// \brief draws the weather effect into color and optionally normal render targets.
   /// \param target primary color render target for weather output.
   /// \param normal normal-map render target for weather effects that contribute normals.
   virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal) = 0;

   /// \brief draws the weather effect with explicit render states (used in WASM to carry the level view).
   /// \param target primary color render target for weather output.
   /// \param normal normal-map render target for weather effects that contribute normals.
   /// \param states render states to apply; weather overlays are screen-space and may ignore the view.
   virtual void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& /*states*/)
   {
      draw(target, normal);
   }

   /// \brief advances weather simulation state for the current frame.
   /// \param dt elapsed frame time since the previous update.
   virtual void update(const sf::Time& dt) = 0;
};
