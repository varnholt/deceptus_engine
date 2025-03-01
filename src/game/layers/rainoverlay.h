#pragma once

#include "constants.h"
#include "weatheroverlay.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <SFML/Graphics.hpp>

class RainOverlay : public WeatherOverlay
{
public:
   struct RainSettings
   {
      bool _collide = true;
      int32_t _drop_count = 500;
      int32_t _fall_through_rate = 0;
   };

   struct RainDrop
   {
      void reset(const sf::FloatRect& rect);

      sf::Vector2f _origin_px;
      sf::Vector2f _pos_px;
      sf::Vector2f _dir_px;
      float _length = 0.0f;
      float _age_s = 0.0f;
      sf::Sprite _sprite;
      std::vector<float> _intersections;
   };

   struct DropHit
   {
      sf::Vector2f _pos_px;
      float _age_s = 0.0f;
      std::unique_ptr<sf::Sprite> _sprite;
   };

   struct Edge
   {
      sf::Vector2f _p1_px;
      sf::Vector2f _p2_px;
   };

   RainOverlay();

   void draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/) override;
   void update(const sf::Time& dt) override;

   void setSettings(const RainSettings& newSettings);

private:
   void determineRainSurfaces();

   bool _initialized = false;
   uint8_t _refresh_surface_counter = 0;

   sf::FloatRect _screen;
   sf::FloatRect _clip_rect;

   std::vector<RainDrop> _drops;
   std::shared_ptr<sf::Texture> _texture;
   std::vector<Edge> _edges;

   std::vector<DropHit> _hits;
   Winding _winding = Winding::Clockwise;

   RainSettings _settings;
};
