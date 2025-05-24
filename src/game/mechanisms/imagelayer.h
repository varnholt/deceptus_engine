#pragma once

#include "game/io/lazytexture.h"
#include "game/layers/parallaxsettings.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <optional>

struct TmxElement;

class ImageLayer : public GameMechanism, public GameNode
{
public:
   ImageLayer(GameNode* parent = nullptr);

   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   void update(const sf::Time& dt) override;

   void updateView(float level_view_x, float level_view_y, float view_width, float view_height);
   void resetView(float view_width, float view_height);

   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   static std::shared_ptr<ImageLayer> deserialize(const std::shared_ptr<TmxElement>& element, const std::filesystem::path& level_path);

private:
   std::unique_ptr<sf::Sprite> _sprite;
   std::shared_ptr<LazyTexture> _texture;
   sf::BlendMode _blend_mode = sf::BlendAlpha;
   sf::Vector2f _position;
   sf::Color _color;

   sf::View _parallax_view;
   std::optional<ParallaxSettings> _parallax_settings;
   std::vector<std::string> _restrict_to_rooms;
   std::filesystem::path _texture_path;
};
